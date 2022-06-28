"""
Module to register the Django application.
"""

from django.apps import AppConfig


class DuoUniversalAuthConfig(AppConfig):
    """
    The specific AppConfig class to register for the Duo Universal
    Authentication application.
    """
    name = 'duo_universal_auth'
