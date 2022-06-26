import sys as _sys

name='openalpr'

import sys as _sys

if _sys.version_info.major >= 3:
    from .openalpr import Alpr
    from .vehicleclassifier import VehicleClassifier
else:
    from openalpr import Alpr
    from vehicleclassifier import VehicleClassifier
