from django.contrib import admin

# Register your models here.
from .models import Vehicle
from .models import Document

admin.site.register(Vehicle)
admin.site.register(Document)
