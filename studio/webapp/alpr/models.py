from django.db import models
from django.contrib.auth.models import User
import os


# Create your models here.
def get_upload_path(instance, filename):
    return 'media/{0}/{1}'.format(instance.user.username, filename)

class Document(models.Model): 
    # uploadedFile        = models.FileField(upload_to = "media/")
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    uploadedFile        = models.FileField(upload_to = get_upload_path)
    dateTimeOfUpload    = models.DateTimeField(auto_now = True)

    def filename(self):
        return os.path.basename(self.uploadedFile.name)

class Vehicle(models.Model):
    filename        = models.CharField(max_length=256, null=False)
    plate_number    = models.CharField(max_length=30, null=False)
    confidence      = models.FloatField(null=False)
    frame_no        = models.IntegerField(null=False)
    detected_at     = models.DateTimeField(auto_now_add=True)
    session_key     = models.CharField(max_length=256, null=False)
    user            = models.ForeignKey(User, on_delete=models.CASCADE)

    class Meta:
        ordering = ['frame_no']
