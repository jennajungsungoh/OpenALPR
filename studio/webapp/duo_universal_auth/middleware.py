"""
Module containing the DuoUniversalAuthMiddleware class.
"""

import logging

from django.conf import settings
from django.urls import reverse
from django.shortcuts import redirect
from django.contrib.auth import BACKEND_SESSION_KEY, logout
from django.utils.module_loading import import_string
from duo_universal.client import Client, DuoException


DUO_CLIENTS = {}
LOGGER = logging.getLogger(__name__)

class DuoUniversalAuthMiddleware:
    """
    Middleware to check if the user is authenticated with Duo per request.
    """

    def __init__(self, get_response):
        """
        Initialize the middleware with the Duo client based on app settings.
        """
        self.get_response = get_response


    @staticmethod
    def get_duo_username(app, request):
        """
        Return the Duo username for the current user formatted using the app's
        username remapping function (if applicable).
        """
        fn_get_username = lambda r: r.user.username
        if 'USERNAME_REMAP_FUNCTION' in app:
            app_fn = app['USERNAME_REMAP_FUNCTION']
            fn_get_username = app_fn if callable(app_fn) else import_string(app_fn)
        logging.debug('Invoking username mapping function: %s', fn_get_username)
        return fn_get_username(request)


    @staticmethod
    def create_duo_client(app, request):
        """
        Create a Duo client for a specific auth backend based on the app settings.
        """
        try:
            return Client(
                client_id = app['CLIENT_ID'],
                client_secret = app['CLIENT_SECRET'],
                host = app['DUO_HOST'],
                #redirect_uri = request.build_absolute_uri(
                #    reverse('duo_universal_auth:duo_callback')
                #),
                redirect_uri = app['REDIRECT_URI'],
                duo_certs = app.get('DUO_CERTS', None),
            )
        except DuoException as e:
            LOGGER.error("*** DuoException (this is most likely a configuration issue): %s ***", e)
            raise e


    @staticmethod
    def get_duo_client(app, request):
        """
        Return the Duo client for the current auth backend, creating if necessary.
        """
        auth_backend = request.session.get(BACKEND_SESSION_KEY)
        if auth_backend not in DUO_CLIENTS:
            DUO_CLIENTS[auth_backend] = DuoUniversalAuthMiddleware.create_duo_client(app, request)
        return DUO_CLIENTS[auth_backend]


    @staticmethod
    def get_app_settings(request):
        """
        Return the app settings for the current auth backend.
        """
        auth_backend = request.session.get(BACKEND_SESSION_KEY)
        for app_name, app_settings in settings.DUO_UNIVERSAL_AUTH.items():
            if auth_backend in app_settings['AUTH_BACKENDS']:
                return app_name, app_settings
        return None, None


    def get_duo_auth_url(self, request):
        """
        Create and return the redirect to the Duo authentication page.
        Returns a redirect if a valid Duo URL was created or if the
        authentication cycle was closed, otherwise returns None.
        """
        # Capture the current path so that it can be passed to the Duo callback.
        _, app_settings = DuoUniversalAuthMiddleware.get_app_settings(request)

        # Get the Duo app settings for the current auth backend (if any).
        if not app_settings:
            # The current backend is not configured for Duo, skip it.
            request.session['DUO_STATUS'] = 'SKIPPED'
            return None

        # Get/Create the Duo client and redirect to the Duo authentication page.
        duo_client = self.get_duo_client(app_settings, request)

        # Validate the status of Duo's server
        try:
            duo_client.health_check()
        except DuoException as e:
            LOGGER.error(e)
            if app_settings.get('FAIL_ACTION', 'CLOSED').upper() == 'CLOSED':
                # If Duo is not available, close the session and redirect to the login page.
                request.session['DUO_STATUS'] = 'CLOSED'
                logout(request)
                return settings.LOGIN_URL
            else:
                request.session['DUO_STATUS'] = 'SKIPPED'
                return None

        username = DuoUniversalAuthMiddleware.get_duo_username(app_settings, request)

        state = duo_client.generate_state()
        request.session['DUO_STATE'] = state
        request.session['DUO_USERNAME'] = username
        return duo_client.create_auth_url(username, state)


    def __call__(self, request):
        """
        Code to be executed for each request to check for a valid Duo session
        before the view (or any other registered middleware) is called.
        """
        if request.path.startswith(settings.STATIC_URL):
            # Serve static files regardless of authentication status.
            return self.get_response(request)

        elif request.user.is_authenticated and 'DUO_STATUS' not in request.session:
            # If the user is authenticated but not authenticated with Duo,
            # redirect to the Duo 2FA page.
            request.session['DUO_STATUS'] = 'IN_PROGRESS'
            request.session['DUO_NEXT_URL'] = request.path
            auth_url = self.get_duo_auth_url(request)
            return redirect(auth_url) if auth_url else self.get_response(request)

        elif (request.user.is_authenticated
                and request.session.get('DUO_STATUS', 'IN_PROGRESS') == 'IN_PROGRESS'
                and reverse('duo_universal_auth:duo_callback') not in request.path):
            # If the user is partially authenticated with Duo and a request is made
            # pointing elsewhere, log out and return to login page.
            logout(request)
            return self.get_response(request)
        else:
            # If the user is authenticated with Duo, or is not authenticated at all,
            # continue with the initial request.
            return self.get_response(request)
