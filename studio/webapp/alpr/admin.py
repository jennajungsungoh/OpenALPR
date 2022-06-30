from django.contrib import admin

# Register your models here.
from .models import Vehicle
from .models import Document
from .models import Status

admin.site.register(Vehicle)
admin.site.register(Document)
admin.site.register(Status)
