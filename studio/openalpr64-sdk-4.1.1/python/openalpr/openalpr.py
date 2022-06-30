import ctypes
import json
import os
import platform
from threading import Lock
import numpy as np
import numpy.ctypeslib as npct

mutex = Lock()

# We need to do things slightly differently for Python 2 vs. 3
# ... because the way str/unicode have changed to bytes/str
if platform.python_version_tuple()[0] == '2':
    # Using Python 2
    bytes = str
    _PYTHON_3 = False
else:
    # Assume using Python 3+
    unicode = str
    _PYTHON_3 = True


def _convert_to_charp(string):
    # Prepares function input for use in c-functions as char*
    if type(string) == unicode:
        return string.encode("UTF-8")
    elif type(string) == bytes:
        return string
    else:
        raise TypeError("Expected unicode string values or ascii/bytes values. Got: %r" % type(string))


def _convert_from_charp(charp):
    # Prepares char* output from c-functions into Python strings
    if _PYTHON_3 and type(charp) == bytes:
        return charp.decode("UTF-8")
    else:
        return charp


class AlprCRegionOfInterest(ctypes.Structure):
    _fields_ = [("x",  ctypes.c_int),
                ("y", ctypes.c_int),
                ("width", ctypes.c_int),
                ("height", ctypes.c_int)]


class Alpr:
    """Initializes an OpenALPR instance in memory.

    :param str country: The default region for license plates. E.g., "us" or "eu"
    :param str config_file: The path to the OpenALPR config file
    :param str runtime_dir: The path to the OpenALPR runtime data directory
    :param str license_key: Commercial license
    :param bool use_gpu: Whether or not to use GPU acceleration
    :param int gpu_id: ID of the GPU to use
    :param int gpu_batch_size: Number of images to process simultaneously
    :return: An OpenALPR instance
    """

    def __init__(self, country, config_file, runtime_dir, license_key="", use_gpu=False, gpu_id=0, gpu_batch_size=10):
        # platform.system() calls popen which is not threadsafe on Python 2.x
        mutex.acquire()
        operating_system = platform.system()
        mutex.release()
        self.loaded = False

        if not os.path.isfile(config_file) and not (config_file == '' and operating_system.lower() == 'linux'):
            raise ValueError("Config file '{}' does not exist".format(config_file))
        if not os.path.isdir(runtime_dir) and not (runtime_dir == '' and operating_system.lower() == 'linux'):
            raise ValueError("Runtime directory '{}' does not exist".format(runtime_dir))
        country = _convert_to_charp(country)
        config_file = _convert_to_charp(config_file)
        runtime_dir = _convert_to_charp(runtime_dir)

        try:
            # Load the .dll for Windows and the .so for Unix-based
            if operating_system.lower().find("windows") != -1:
                self._openalprpy_lib = ctypes.cdll.LoadLibrary("libopenalpr.dll")
            elif operating_system.lower().find("darwin") != -1:
                self._openalprpy_lib = ctypes.cdll.LoadLibrary("libopenalpr.dylib")
            else:
                self._openalprpy_lib = ctypes.cdll.LoadLibrary("libopenalpr.so.2")
        except OSError as e:
            nex = OSError("Unable to locate the OpenALPR library. Please make sure that OpenALPR is properly "
                          "installed on your system and that the libraries are in the appropriate paths.")
            if _PYTHON_3:
                nex.__cause__ = e
            raise nex

        self.use_gpu = use_gpu
        self.gpu_batch_size = gpu_batch_size
        if self.use_gpu:
            self._initialize_func = self._openalprpy_lib.openalpr_init_gpu
            self._initialize_func.restype = ctypes.c_void_p
            self._initialize_func.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        else:
            self._initialize_func = self._openalprpy_lib.openalpr_init
            self._initialize_func.restype = ctypes.c_void_p
            self._initialize_func.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

        self._add_encoded_image_to_batch_func = self._openalprpy_lib.openalpr_add_encoded_image_to_batch
        self._add_encoded_image_to_batch_func.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_uint]

        self._add_image_to_batch_func = self._openalprpy_lib.openalpr_add_image_to_batch
        self._add_image_to_batch_func.argtypes = [
            ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint]

        self._create_image_batch_func = self._openalprpy_lib.openalpr_create_image_batch
        self._create_image_batch_func.restype = ctypes.c_void_p

        self._dispose_func = self._openalprpy_lib.openalpr_cleanup
        self._dispose_func.argtypes = [ctypes.c_void_p]

        self._free_json_mem_func = self._openalprpy_lib.openalpr_free_response_string

        self._set_top_n_func = self._openalprpy_lib.openalpr_set_topn
        self._set_top_n_func.argtypes = [ctypes.c_void_p, ctypes.c_int]

        self._get_version_func = self._openalprpy_lib.openalpr_get_version
        self._get_version_func.argtypes = [ctypes.c_void_p]
        self._get_version_func.restype = ctypes.c_void_p

        self._is_loaded_func = self._openalprpy_lib.openalpr_is_loaded
        self._is_loaded_func.argtypes = [ctypes.c_void_p]
        self._is_loaded_func.restype = ctypes.c_bool

        self._recognize_array_func = self._openalprpy_lib.openalpr_recognize_encodedimage
        self._recognize_array_func.restype = ctypes.c_void_p
        self._recognize_array_func.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_longlong, AlprCRegionOfInterest]

        self._recognize_batch_func = self._openalprpy_lib.openalpr_recognize_batch
        self._recognize_batch_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        self._recognize_batch_func.restype = ctypes.c_void_p

        self._recognize_file_func = self._openalprpy_lib.openalpr_recognize_imagefile
        self._recognize_file_func.restype = ctypes.c_void_p
        self._recognize_file_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        array_1_uint8 = npct.ndpointer(dtype=np.uint8, ndim=1, flags='CONTIGUOUS')
        self._recognize_raw_image_func = self._openalprpy_lib.openalpr_recognize_rawimage
        self._recognize_raw_image_func.restype = ctypes.c_void_p
        self._recognize_raw_image_func.argtypes = [
            ctypes.c_void_p, array_1_uint8, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, AlprCRegionOfInterest]

        self._release_image_batch_func = self._openalprpy_lib.openalpr_release_image_batch
        self._release_image_batch_func.argtypes = [ctypes.c_void_p]

        self._set_country_func = self._openalprpy_lib.openalpr_set_country
        self._set_country_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        self._set_default_region_func = self._openalprpy_lib.openalpr_set_default_region
        self._set_default_region_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        self._set_detect_region_func = self._openalprpy_lib.openalpr_set_detect_region
        self._set_detect_region_func.argtypes = [ctypes.c_void_p, ctypes.c_bool]

        self._set_detect_vehicles_func = self._openalprpy_lib.openalpr_set_detect_vehicles
        self._set_detect_vehicles_func.argtypes = [ctypes.c_void_p, ctypes.c_bool, ctypes.c_bool]

        self._vehiclesignature_rawimage_array_func = self._openalprpy_lib.openalpr_vehiclesignature_rawimage
        self._vehiclesignature_rawimage_array_func.restype = ctypes.c_void_p
        self._vehiclesignature_rawimage_array_func.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_longlong, AlprCRegionOfInterest, ctypes.c_bool]

        self._vehiclesignature_encodedimage_func = self._openalprpy_lib.openalpr_vehiclesignature_encodedimage
        self._vehiclesignature_encodedimage_func.restype = ctypes.c_void_p
        self._vehiclesignature_encodedimage_func.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_longlong, AlprCRegionOfInterest, ctypes.c_bool]

        self._vehiclesignature_similarity_func = self._openalprpy_lib.openalpr_vehiclesignature_similarity
        self._vehiclesignature_similarity_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
        self._vehiclesignature_similarity_func.restype = ctypes.c_double

        if self.use_gpu:
            self.alpr_pointer = self._initialize_func(country, config_file, runtime_dir, _convert_to_charp(license_key), 1, gpu_id, gpu_batch_size)
        else:
            self.alpr_pointer = self._initialize_func(country, config_file, runtime_dir, _convert_to_charp(license_key))

        self.loaded = True

    def __del__(self):
        if self.is_loaded():
            self.unload()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.is_loaded():
            self.unload()

    def _add_encoded_image_to_batch(self, byte_array, batch):
        """Add raw byte array representation of image to a batch.

        :param byte_array: This should be a string (Python 2) or a bytes
            object (Python 3). For an image located at ``img_path``, create
            this object using ``with open(img_path, 'rb') as f; byte_array =
            f.read()``.
        :param int batch: Identifier from ``self._create_image_batch``.
        :return: None
        """
        if type(byte_array) != bytes:
            raise TypeError("Expected a byte array (string in Python 2, bytes in Python 3)")
        pb = ctypes.cast(byte_array, ctypes.POINTER(ctypes.c_ubyte))
        self._add_encoded_image_to_batch_func(batch, pb, len(byte_array))

    def _add_image_to_batch(self, img, batch):
        """Add NumPy loaded image to a batch.

        :param np.ndarray img: 3 dimensional array of pixel data from a method
            like OpenCV's ``imread()``.
        :param int batch: Identifier from ``self._create_image_batch``.
        :return: None
        """
        height, width = img.shape[:2]
        bpp = img.shape[2] if len(img.shape) > 2 else 1
        self._add_image_to_batch_func(batch, img.ctypes.data_as(ctypes.c_void_p), bpp, width, height)

    def get_version(self):
        """Get software version of OpenALPR.

        :return str: Version information
        """
        ptr = self._get_version_func(self.alpr_pointer)
        version_number = ctypes.cast(ptr, ctypes.c_char_p).value
        version_number = _convert_from_charp(version_number)
        self._free_json_mem_func(ctypes.c_void_p(ptr))
        return version_number

    def is_loaded(self):
        """Checks if OpenALPR is loaded.

        :return bool: Representing if OpenALPR is loaded or not
        """
        if not self.loaded:
            return False

        return self._is_loaded_func(self.alpr_pointer)

    def recognize_array(self, byte_array,x,y,w,h):
        """Recognize an image passed in as a byte array.

        :param byte_array: This should be a string (Python 2) or a bytes object (Python 3)
        :return: An OpenALPR analysis in the form of a response dictionary
        """
        if type(byte_array) != bytes:
            raise TypeError("Expected a byte array (string in Python 2, bytes in Python 3)")
        pb = ctypes.cast(byte_array, ctypes.POINTER(ctypes.c_ubyte))
        roi = AlprCRegionOfInterest(x, y, w, h)
        ptr = self._recognize_array_func(self.alpr_pointer, pb, len(byte_array), roi)
        json_data = ctypes.cast(ptr, ctypes.c_char_p).value
        json_data = _convert_from_charp(json_data)
        response_obj = json.loads(json_data)
        self._free_json_mem_func(ctypes.c_void_p(ptr))
        return response_obj

    def vehicle_signature_from_array(self, byte_array, high_resolution=True):
        """Generates a vehicle signature an image passed in as a byte array.

        :param byte_array: This should be a string (Python 2) or a bytes object (Python 3)
        :return: A hash for the vehicle signature
        """
        if type(byte_array) != bytes:
            raise TypeError("Expected a byte array (string in Python 2, bytes in Python 3)")
        pb = ctypes.cast(byte_array, ctypes.POINTER(ctypes.c_ubyte))
        roi = AlprCRegionOfInterest(0, 0, 1000000, 1000000)
        high_res_int = 0
        if high_resolution:
            high_res_int = 1
        ptr = self._vehiclesignature_encodedimage_func(self.alpr_pointer, pb, len(byte_array), roi, high_res_int)
        json_data = ctypes.cast(ptr, ctypes.c_char_p).value
        json_data = _convert_from_charp(json_data)
        response_obj = json_data
        self._free_json_mem_func(ctypes.c_void_p(ptr))
        return response_obj

    def vehicle_signature_similarity(self, signature_a, signature_b):
        """Compares two string signatures and returns their similarity score (0 - 1.0)
        :param signature_a: The first signature
        :param signature_b: The second signature
        :return: A number between 0 and 1.0.  The higher this number the more similar the vehicles are to each other 
        """
        signature_a = _convert_to_charp(signature_a)
        signature_b = _convert_to_charp(signature_b)
        similarity_score = self._vehiclesignature_similarity_func(self.alpr_pointer, signature_a, signature_b)

        return similarity_score

    def recognize_batch(self, images):
        """Combined method to allocate batch, add images, and return JSON.

        If ``len(images)`` is greater than ``self.gpu_batch_size``, this
        method will be recursively called to handle all the images.

        :param list images: All elements must be the same type - either
            ``numpy.ndarray`` (loaded pixel data in OpenCV or similar
            framework) or ``bytes`` (raw JPEG bytes read from image files).
        :return [dict] batch_response: List of JSON data for each batch image.
        """
        # Validate inputs and allocate batch
        if not self.use_gpu:
            raise RuntimeError('\'use_gpu\' attribute must be true to run this method')
        types = [type(i) for i in images]
        if types[1:] != types[:-1]:
            raise ValueError('All batch elements must be the same type')
        batch = self._create_image_batch_func()
        batch_response = []

        # Recursively call method if user batch is larger than `self.batch_size`
        while len(images) > self.gpu_batch_size:
            current = images[:self.gpu_batch_size]
            images = images[self.gpu_batch_size:]
            batch_response += self.recognize_batch(current)

        # Populate batch using appropriate internal method
        if isinstance(images[0], bytes):
            for i in images:
                self._add_encoded_image_to_batch(i, batch)
        elif isinstance(images[0], np.ndarray):
            for i in images:
                self._add_image_to_batch(i, batch)
        else:
            raise ValueError('Unsupported batch element type {}'.format(type(images[0])))

        # Run batch, retrieve results, and free memory
        ptr = self._recognize_batch_func(self.alpr_pointer, batch)
        json_data = ctypes.cast(ptr, ctypes.c_char_p).value
        json_data = _convert_from_charp(json_data)
        batch_response += json.loads(json_data)
        self._free_json_mem_func(ctypes.c_void_p(ptr))
        self._release_image_batch_func(batch)
        return batch_response

    def recognize_file(self, file_path):
        """Recognize an image by opening a file on disk.

        :param file_path: The path to the image that will be analyzed
        :return: An OpenALPR analysis in the form of a response dictionary
        """
        file_path = _convert_to_charp(file_path)
        ptr = self._recognize_file_func(self.alpr_pointer, file_path)
        json_data = ctypes.cast(ptr, ctypes.c_char_p).value
        json_data = _convert_from_charp(json_data)
        response_obj = json.loads(json_data)
        self._free_json_mem_func(ctypes.c_void_p(ptr))
        return response_obj

    def recognize_ndarray(self, ndarray):
        """Recognize an image passed in as a numpy array.

        :param ndarray: numpy.array as used in cv2 module
        :return: An OpenALPR analysis in the form of a response dictionary
        """
        height, width = ndarray.shape[:2]
        bpp = ndarray.shape[2] if len(ndarray.shape) > 2 else 1
        roi = AlprCRegionOfInterest(0, 0, width, height)
        ptr = self._recognize_raw_image_func(self.alpr_pointer, ndarray.flatten(), bpp, width, height, roi)
        json_data = ctypes.cast(ptr, ctypes.c_char_p).value
        json_data = _convert_from_charp(json_data)
        response_obj = json.loads(json_data)
        self._free_json_mem_func(ctypes.c_void_p(ptr))
        return response_obj

    def set_top_n(self, topn):
        """Sets the number of returned results when analyzing an image.

        :param int topn: Desired number of returned results.
        :return: None
        """
        self._set_top_n_func(self.alpr_pointer, topn)
        # TODO when country = us, this cannot be set higher than 6...

    def set_default_region(self, region):
        """Set the default region for detecting license plates.

        :param region: A two character unicode/ascii string (Python 2/3) or
            bytes array (Python 3). For example, ``md`` for Maryland or ``fr``
            for France.
        :return: None
        """
        region = _convert_to_charp(region)
        self._set_default_region_func(self.alpr_pointer, region)

    def set_detect_vehicles(self, enabled, generate_tracking_signatures=True):
        """Allow OpenALPR to detect vehicles and generate tracking signatures.

        :param bool enabled: Whether or not vehicle bounding box detection is enabled. By
            default this is disabled for newly initialized instances.
        :param bool generate_tracking_signatures: Generate a signature used to track vehicles
            vehicle tracking must be enabled for this to work.
        :return: None
        """
        self._set_detect_vehicles_func(self.alpr_pointer, enabled, generate_tracking_signatures)

    def set_detect_region(self, enabled):
        """Allow OpenALPR to autodetect the region of a license plate.

        :param bool enabled: Whether or not auto-detection is enabled. By
            default this is disabled for newly initialized instances.
        :return: None
        """
        self._set_detect_region_func(self.alpr_pointer, enabled)

    def set_prewarp(self, prewarp):
        """Updates the prewarp configuration used to skew images before processing.

        :param prewarp: A unicode/ascii string (Python 2/3) or bytes array (Python 3)
        :return: None
        """
        prewarp = _convert_to_charp(prewarp)
        self._set_prewarp_func(self.alpr_pointer, prewarp)

    def set_country(self, country):
        """Set the country for detecting license plates.

        :param country: A unicode/ascii string (Python 2/3) or bytes array
            (Python 3). For example, ``us`` for United States or ``eu`` for
            Europe.
        :return: None
        """
        country = _convert_to_charp(country)
        self._set_country_func(self.alpr_pointer, country)

    def unload(self):
        """Unloads OpenALPR from memory."""
        if self.loaded:
            self.loaded = False
            self._dispose_func(self.alpr_pointer)
