"""
URLs to route for the Duo callback.
"""
from django.urls import path
from .views import DuoUniversalAuthCallback


app_name = "duo_universal_auth"

urlpatterns = [
    path('callback/', DuoUniversalAuthCallback.as_view(), name="duo_callback"),
]
