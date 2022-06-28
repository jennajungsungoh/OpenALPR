"""
Module containing Duo authentication callbacks using views.
"""
import logging

from django.conf import settings
from django.contrib.auth import logout
from django.contrib.auth.mixins import LoginRequiredMixin
from django.shortcuts import redirect
from django.views import View

from .middleware import DuoUniversalAuthMiddleware


LOGGER = logging.getLogger(__name__)

class DuoUniversalAuthCallback(LoginRequiredMixin, View):
    """
    View that handles the Duo Universal authentication callback.
    """

    def get(self, request):
        """
        Handle the GET request.
        """
        _, app_settings = DuoUniversalAuthMiddleware.get_app_settings(request)
        duo_client = DuoUniversalAuthMiddleware.get_duo_client(app_settings, request)
        next_url = request.session.get('DUO_NEXT_URL', settings.LOGIN_REDIRECT_URL)
        state = request.GET.get('state')
        code = request.GET.get('duo_code')

        if 'DUO_STATE' in request.session and 'DUO_USERNAME' in request.session:
            saved_state = request.session['DUO_STATE']
            username = request.session['DUO_USERNAME']
        else:
            logout(request)

        if state != saved_state:
            logout(request)

        decoded_token = duo_client.exchange_authorization_code_for_2fa_result(code, username)
        if decoded_token:
            request.session['DUO_STATUS'] = 'SUCCESS'
        else:
            logout(request)

        return redirect(next_url)
