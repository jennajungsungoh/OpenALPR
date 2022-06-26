import sys as _sys

name='alprstream'

if _sys.version_info.major >= 3:
    from .alprstream import AlprStream
else:
    from alprstream import AlprStream
