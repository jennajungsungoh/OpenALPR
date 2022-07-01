from django.db import models
from django.contrib.auth.models import User
import os


# Create your models here.
def get_upload_path(instance, filename):
    return 'media/{0}/{1}'.format(instance.user.username, filename)

def get_upload_tmp_path(instance, filename):
    return 'media/{0}/{1}/{2}'.format(instance.user.username, instance.filename.rsplit('.')[0], filename)

class Config(models.Model):
    lookup_server = models.CharField(max_length=256, null=False)

class Status(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    filename = models.CharField(max_length=256, null=False)
    status = models.BooleanField(null=False)

class Document(models.Model): 
    # uploadedFile        = models.FileField(upload_to = "media/")
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    uploadedFile        = models.FileField(upload_to=get_upload_path)
    dateTimeOfUpload    = models.DateTimeField(auto_now = True)

    def filename(self):
        return os.path.basename(self.uploadedFile.name)

class Vehicle(models.Model):
    filename        = models.CharField(max_length=256, null=False)
    confidence      = models.FloatField(null=False)
    frame_no        = models.IntegerField(null=False)
    detected_at     = models.DateTimeField(auto_now_add=True)
    session_key     = models.CharField(max_length=256, null=False)
    user            = models.ForeignKey(User, on_delete=models.CASCADE)
    captured_frame  = models.FileField(upload_to=get_upload_tmp_path)
    
    plate_number        = models.CharField(max_length=30, null=False)
    plate_number_likely  = models.CharField(max_length=30, null=True)

    status          = models.CharField(max_length=100, null=True)
    regist_exp      = models.CharField(max_length=30, null=True)
    owner           = models.CharField(max_length=100, null=True)
    owner_birth     = models.CharField(max_length=30, null=True)
    owner_addr      = models.CharField(max_length=256, null=True)
    owner_zip       = models.CharField(max_length=20, null=True)
    yom             = models.CharField(max_length=10, null=True)
    maker           = models.CharField(max_length=50, null=True)
    model           = models.CharField(max_length=50, null=True)
    color           = models.CharField(max_length=50, null=True)


    class Meta:
        ordering = ['frame_no']
