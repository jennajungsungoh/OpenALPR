import sys as _sys
import os as _os

_sys.stderr.write("This import is deprecated -- use 'from openalpr import VehicleClassifier' instead" + _os.linesep)

if _sys.version_info.major >= 3:
    from .vehicleclassifier import VehicleClassifier
else:
    from vehicleclassifier import VehicleClassifier
