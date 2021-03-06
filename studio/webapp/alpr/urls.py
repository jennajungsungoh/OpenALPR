from django.urls import path
from django.conf import settings
from django.conf.urls.static import static


from . import views

app_name='alpr'

urlpatterns = [
    path('playback', views.playback, name='playback'),
    path('webcam', views.webcam, name='webcam'),
    path('upload', views.upload, name='upload'),
    path('remove', views.remove, name='remove'),

    path('play', views.play, name='play'),
    path('stop', views.stop, name='stop'),
    path('get_vehicle', views.get_vehicle, name='get_vehicle'),
    path('remove_vehicle_history', views.remove_vehicle_history, name='remove_vehicle_history'),
    path('get_captured_plate', views.get_captured_plate, name='get_captured_plate'),
    path('get_captured_vehicle', views.get_captured_vehicle, name='get_captured_vehicle'),

    path('config', views.config, name='config'),
    
    path('', views.index, name='index'),
]

if settings: 
    urlpatterns += static(
        settings.MEDIA_URL, 
        document_root = settings.MEDIA_ROOT) 