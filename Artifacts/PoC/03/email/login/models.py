from django.db import models

# Create your models here.
class UserInfo(models.Model):
    username = models.CharField(max_length=200)
    password = models.CharField(max_length=200)

    def publish(self):
        self.save()



class EmailInfo(models.Model):
    subject = models.CharField(max_length=200)
    message = models.TextField()
    to = models.EmailField()
    from_email = models.EmailField()

    def publish(self):
        self.save()
