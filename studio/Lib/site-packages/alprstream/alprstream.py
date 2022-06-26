import ctypes
import json
import os
import platform
from threading import Lock
import numpy as np
import numpy.ctypeslib as npct

mutex = Lock()
if platform.python_version_tuple()[0] == "2":
    bytes = str
    _PYTHON_3 = False
else:
    unicode = str
    _PYTHON_3 = True


def _convert_bytes_to_json(bytes):
    """Convert bytes string to Python JSON.

    :param bytes: Bytes string from C code.
    :return: JSON formatted Python string.
    """
    return json.loads(bytes.decode("UTF-8"))


def _convert_to_charp(string):
    """Prepares function input for use in C-functions as c_char_p."""
    if type(string) == unicode:
        return string.encode("UTF-8")
    elif type(string) == bytes:
        return string


def _convert_from_charp(charp):
    """Prepares c_char_p output from C-functions into Python strings."""
    if _PYTHON_3 and type(charp) == bytes:
        return charp.decode("UTF-8")
    else:
        return charp


class AlprStreamRecognizedFrameC(ctypes.Structure):
    _fields_ = [("image_available",     ctypes.c_bool),
                ("jpeg_bytes",          ctypes.c_char_p),
                ("jpeg_bytes_size",     ctypes.c_longlong),
                ("frame_epoch_time_ms", ctypes.c_longlong),
                ("frame_number",        ctypes.c_longlong),
                ("results_str",         ctypes.c_char_p)]


class AlprStreamRecognizedBatchC(ctypes.Structure):
    _fields_ = [("results_size",  ctypes.c_int),
                ("results_array", ctypes.c_void_p),
                ("batch_results", ctypes.c_char_p)]


class AlprStream:
    """Manage a video file/stream and group similar plates as detected.

    :param int frame_queue_size: The size of the video buffer to be filled by incoming video frames
    :param bool use_motion_detection: Whether or not to enable motion detection on this stream
    """

    def __init__(self, frame_queue_size, use_motion_detection=1):
        # Load dynamic library based on operating system
        mutex.acquire()  # Python 2.x platform.system() is not thread-safe
        try:
            if platform.system().lower().find("windows") != -1:
                self._alprstreampy_lib = ctypes.cdll.LoadLibrary("libalprstream.dll")
            elif platform.system().lower().find("darwin") != -1:
                self._alprstreampy_lib = ctypes.cdll.LoadLibrary("libalprstream.dylib")
            else:
                self._alprstreampy_lib = ctypes.cdll.LoadLibrary("libalprstream.so.3")
        except OSError as e:
            nex = OSError("Unable to locate the ALPRStream library. Please make sure that ALPRStream is properly "
                          "installed on your system and that the libraries are in the appropriate paths.")
            if _PYTHON_3:
                nex.__cause__ = e
            raise nex
        finally:
            mutex.release()

        # Initialize the AlprStream object
        self._is_loaded = False
        self._initialize_func = self._alprstreampy_lib.alprstream_init
        self._initialize_func.restype = ctypes.c_void_p
        self._initialize_func.argtypes = [ctypes.c_uint, ctypes.c_uint]
        self._alprstream_pointer = self._initialize_func(frame_queue_size, use_motion_detection)
        self._is_loaded = True

        # Create ctypes bindings for remaining methods
        self._combine_grouping_func = self._alprstreampy_lib.alprstream_combine_grouping
        self._combine_grouping_func.restype = ctypes.c_void_p
        self._combine_grouping_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

        self._connect_video_file_func = self._alprstreampy_lib.alprstream_connect_video_file
        self._connect_video_file_func.restype = ctypes.c_void_p
        self._connect_video_file_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint]

        self._connect_video_stream_url_func = self._alprstreampy_lib.alprstream_connect_video_stream_url
        self._connect_video_stream_url_func.restype = ctypes.c_void_p
        self._connect_video_stream_url_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]

        self._disconnect_video_file_func = self._alprstreampy_lib.alprstream_disconnect_video_file
        self._disconnect_video_file_func.restype = ctypes.c_void_p
        self._disconnect_video_file_func.argtypes = [ctypes.c_void_p]

        self._disconnect_video_stream_func = self._alprstreampy_lib.alprstream_disconnect_video_stream
        self._disconnect_video_stream_func.restype = ctypes.c_void_p
        self._disconnect_video_stream_func.argtypes = [ctypes.c_void_p]

        self._dispose_func = self._alprstreampy_lib.alprstream_cleanup
        self._dispose_func.argtypes = [ctypes.c_void_p]
        self._dispose_func.restype = ctypes.c_bool

        self._free_batch_response_func = self._alprstreampy_lib.alprstream_free_batch_response
        self._free_batch_response_func.restype = ctypes.c_void_p
        self._free_batch_response_func.argtypes = [ctypes.POINTER(AlprStreamRecognizedBatchC)]

        self._free_frame_response_func = self._alprstreampy_lib.alprstream_free_frame_response
        self._free_frame_response_func.restype = ctypes.POINTER(AlprStreamRecognizedFrameC)
        self._free_frame_response_func.argtypes = [ctypes.c_void_p]

        self._free_response_string_func = self._alprstreampy_lib.alprstream_free_response_string
        self._free_response_string_func.argtypes = [ctypes.c_void_p]

        self._get_queue_size_func = self._alprstreampy_lib.alprstream_get_queue_size
        self._get_queue_size_func.restype = ctypes.c_uint
        self._get_queue_size_func.argtypes = [ctypes.c_void_p]

        self._get_video_file_fps_func = self._alprstreampy_lib.alprstream_get_video_file_fps
        self._get_video_file_fps_func.restype = ctypes.c_double
        self._get_video_file_fps_func.argtypes = [ctypes.c_void_p]

        self._peek_active_groups_func = self._alprstreampy_lib.alprstream_peek_active_groups
        self._peek_active_groups_func.restype = ctypes.c_char_p
        self._peek_active_groups_func.argtypes = [ctypes.c_void_p]

        self._pop_completed_groups_and_recognize_vehicle_func = self._alprstreampy_lib.alprstream_pop_completed_groups_and_recognize_vehicle
        self._pop_completed_groups_and_recognize_vehicle_func.restype = ctypes.c_char_p
        self._pop_completed_groups_and_recognize_vehicle_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]
            
        self._pop_completed_groups_and_recognize_vehicle_sig_func = self._alprstreampy_lib.alprstream_pop_completed_groups_and_recognize_vehicle_signature
        self._pop_completed_groups_and_recognize_vehicle_sig_func.restype = ctypes.c_char_p
        self._pop_completed_groups_and_recognize_vehicle_sig_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]

        self._pop_completed_groups_func = self._alprstreampy_lib.alprstream_pop_completed_groups
        self._pop_completed_groups_func.restype = ctypes.c_void_p
        self._pop_completed_groups_func.argtypes = [ctypes.c_void_p, ctypes.c_int]

        self._process_batch_func = self._alprstreampy_lib.alprstream_process_batch
        self._process_batch_func.restype = ctypes.POINTER(AlprStreamRecognizedBatchC)
        self._process_batch_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

        self._process_frame_func = self._alprstreampy_lib.alprstream_process_frame
        self._process_frame_func.restype = ctypes.POINTER(AlprStreamRecognizedFrameC)
        self._process_frame_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

        self._push_frame_encoded_func = self._alprstreampy_lib.alprstream_push_frame
        self._push_frame_encoded_func.restype = ctypes.c_uint
        self._push_frame_encoded_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_longlong, ctypes.c_longlong]

        self._push_frame_func = self._alprstreampy_lib.alprstream_push_frame
        c_uint8_p = npct.ndpointer(dtype=np.uint8, ndim=1, flags='CONTIGUOUS')
        self._push_frame_func.restype = ctypes.c_uint
        self._push_frame_func.argtypes = [ctypes.c_void_p, c_uint8_p, ctypes.c_uint, ctypes.c_uint, ctypes.c_longlong]

        self._set_encode_jpeg_func = self._alprstreampy_lib.alprstream_set_encode_jpeg
        self._set_encode_jpeg_func.restype = ctypes.c_void_p
        self._set_encode_jpeg_func.argtypes = [ctypes.c_void_p, ctypes.c_int]

        self._set_env_parameters_func = self._alprstreampy_lib.alprstream_set_env_parameters
        self._set_env_parameters_func.restype = ctypes.c_void_p
        self._set_env_parameters_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]

        self._set_detection_mask_encoded_func = self._alprstreampy_lib.alprstream_set_detection_mask
        self._set_detection_mask_encoded_func.restype = ctypes.c_void_p
        self._set_detection_mask_encoded_func.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_longlong]

        self._set_detection_mask_func = self._alprstreampy_lib.alprstream_set_detection_mask
        self._set_detection_mask_func.restype = ctypes.c_void_p
        self._set_detection_mask_func.argypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int, ctypes.c_int, ctypes.c_int]

        self._set_group_parameters_func = self._alprstreampy_lib.alprstream_set_group_parameters
        self._set_group_parameters_func.restype = ctypes.c_void_p
        self._set_group_parameters_func.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_int]

        self._set_jpeg_compression_func = self._alprstreampy_lib.alprstream_set_jpeg_compression
        self._set_jpeg_compression_func.restype = ctypes.c_void_p
        self._set_jpeg_compression_func.argtypes = [ctypes.c_void_p, ctypes.c_int]

        self._enable_overview_image_func = self._alprstreampy_lib.alprstream_enable_overview_jpeg
        self._enable_overview_image_func.restype = ctypes.c_void_p
        self._enable_overview_image_func.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]

        self._set_uuid_format_func = self._alprstreampy_lib.alprstream_set_uuid_format
        self._set_uuid_format_func.restype = ctypes.c_void_p
        self._set_uuid_format_func.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        self._video_file_active_func = self._alprstreampy_lib.alprstream_video_file_active
        self._video_file_active_func.restype = ctypes.c_uint
        self._video_file_active_func.argtypes = [ctypes.c_void_p]

        self._set_gpu_async_func = self._alprstreampy_lib.alprstream_set_gpu_async
        self._set_gpu_async_func.restype = ctypes.c_void_p
        self._set_gpu_async_func.argtypes = [ctypes.c_void_p, ctypes.c_int]

    def __del__(self):
        if self._is_loaded:
            self._is_loaded = False
            self._dispose_func(self._alprstream_pointer)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._is_loaded:
            self._is_loaded = False
            self._dispose_func(self._alprstream_pointer)

    def _convert_char_ptr_to_json(self, char_ptr):
        """Cast ctypes c_char_p to JSON and free the underlying memory.

        :param char_ptr: ctypes pointer to a character array.
        :return str response_obj: JSON formatted Python string.
        """
        json_data = ctypes.cast(char_ptr, ctypes.c_char_p).value
        json_data = _convert_from_charp(json_data)
        response_obj = json.loads(json_data)
        self._free_response_string_func(ctypes.c_void_p(char_ptr))
        return response_obj

    def combine_grouping(self, other_stream):
        """Combine plate grouping across cameras.

        This is useful if one or more cameras are looking at roughly the same
        area (i.e. from different angles), and you want to combine the group
        results.

        :param other_stream: Another AlprStream object.
        :return: None
        """
        self._combine_grouping_func(self._alprstream_pointer, other_stream)

    def connect_video_file(self, video_file_path, video_start_time):
        """Spawns thread connected to video file and fills processing queue.

        The thread will slow down to make sure that it does not overflow the
        queue The ``video_start_time`` is used to us with the epoch start time
        of of the video

        :param video_file_path: Location of video file on disk.
        :param video_start_time: Start time of the video (in ms). Used as an
            offset for identifying the epoch time for each frame in the video.
        :return: None
        """
        if not os.path.exists(video_file_path):
            raise ValueError('Video file \'{}\' does not exist'.format(video_file_path))
        video_file_path = _convert_to_charp(video_file_path)
        self._connect_video_file_func(self._alprstream_pointer, video_file_path, video_start_time)

    def connect_video_stream_url(self, url, gstreamer_pipeline_format=""):
        """Spawns thread connected to stream URL and fills processing queue.

        :param: url: the full URL to be used to connect to the video stream
        :param: gstreamer_pipeline_format: An optional override for the
            GStreamer format. Use {url} for a marker to substitute the url
            value.
        :return: None
        """
        url = _convert_to_charp(url)
        gstreamer_pipeline_format = _convert_to_charp(gstreamer_pipeline_format)
        self._connect_video_stream_url_func(self._alprstream_pointer, url, gstreamer_pipeline_format)

    def disconnect_video_file(self):
        """Stops video and removes it from the stream."""
        self._disconnect_video_file_func(self._alprstream_pointer)

    def disconnect_video_stream(self):
        """Disconnects video stream and stops pushing frames to buffer."""
        self._disconnect_video_stream_func(self._alprstream_pointer)

    def get_queue_size(self):
        """Check the size of the video buffer.

        :return size: Number of images waiting to be processed.
        """
        size = self._get_queue_size_func(self._alprstream_pointer)
        return size

    def get_stream_url(self):
        """Get the stream URL.

        :return url: The stream URL that is currently being used to stream.
        """
        url = self._get_stream_url_func(self._alprstream_pointer)
        url = _convert_to_charp(url)
        return url

    def get_video_file_fps(self):
        """Get the frames per second for the video file.

        :return float frames: The video file FPS value.
        """
        frames = self._get_video_file_fps_func(self._alprstream_pointer)
        return frames

    def is_loaded(self):
        """Check whether the AlprStream object is loaded into memory."""
        return self._is_loaded

    def peek_active_groups(self):
        """Check for active groups (but don't remove entries from the queue).

        :return list results: All currently active groups.
        """
        bytes = self._peek_active_groups_func(self._alprstream_pointer)
        results = _convert_bytes_to_json(bytes)
        return results

    def pop_completed_groups(self):
        """Get plate data for completed groups and remove from queue.

        :return list results: All completed plate groups, each as a JSON dict.
        """
        ptr = self._pop_completed_groups_func(self._alprstream_pointer, 1)
        results = self._convert_char_ptr_to_json(ptr)
        return results

    def pop_completed_groups_and_recognize_vehicle(self, vehicleclassifier_instance, alpr_instance=None):
        """Get plate/vehicle data for completed groups and remove from queue.

        :param vehicleclassifier_instance: Python VehicleClassifier instance
            you wish to use.
        :return list results: With each element containing a JSON dict.
        """
        if alpr_instance is None:
            bytes = self._pop_completed_groups_and_recognize_vehicle_func(
                self._alprstream_pointer, vehicleclassifier_instance.vehicleclassifier_pointer, 1)
        else:
            bytes = self._pop_completed_groups_and_recognize_vehicle_sig_func(
                self._alprstream_pointer, alpr_instance.alpr_pointer, vehicleclassifier_instance.vehicleclassifier_pointer, 1)

        results = _convert_bytes_to_json(bytes)
        return results

    def process_batch(self, alpr_instance):
        """Process the first image in the queue and return individual results.

        This function is most useful when using GPU acceleration.  Processing
        frames in a batch more efficiently uses GPU resources. You should
        make sure that the video buffer size for this AlprStream object is
        greater than or equal to the configured GPU batch size (in
        openalpr.conf).

        :param alpr_instance: Alpr object for processing images.
        :return list results: Array of results for all recognized frames, each
            as a JSON dict.
        """
        struct_response = self._process_batch_func(self._alprstream_pointer, alpr_instance.alpr_pointer)
        results = _convert_bytes_to_json(struct_response.contents.batch_results)
        self._free_batch_response_func(struct_response)
        return results

    def process_frame(self, alpr_instance):
        """Detect license plates from a single video frame.

        :param alpr_instance: Alpr object to process the frame.
        :return dict results: License plate, region, and confidence as JSON
            formatted Python dict.
        """
        struct_response = self._process_frame_func(self._alprstream_pointer, alpr_instance.alpr_pointer)
        results = _convert_bytes_to_json(struct_response.contents.results_str)
        self._free_frame_response_func(struct_response)
        return results

    def push_frame(self, pixels, bpp, width, height, frame_epoch_time=-1):
        """Push raw image data onto the video input buffer.

        :param numpy.ndarray pixels: Raw image bytes for BGR channels from
            ``cv2.imread()``.
        :param int bpp: Number of bytes per pixel.
        :param int width: Width of the image in pixels.
        :param int height: Height of the image in pixels.
        :param int frame_epoch_time: The time when the image was captured. If
            not specified current time will be used
        :return int size: The video input buffer size after adding this image
        """
        size = self._push_frame_func(
            self._alprstream_pointer, pixels.flatten(), bpp, width, height, ctypes.c_int64(frame_epoch_time))
        return size

    def set_encode_jpeg(self, always_return_jpeg):
        """Change default settings for the JPEG encoder.

        By default, OpenALPR only encodes/returns a JPEG image if a plate is
        found. Encoding a JPEG has some performance impact. When processing on
        GPU, the encoding happens on CPU background threads.  Disabling this
        setting (if the JPEG images are not used) will reduce CPU usage. If
        ``always_return_jpeg`` is set to never, vehicle recognition will not
        function correctly.

        :param always_return_jpeg: 0=Never, 1=On Found Plates, 2=Always
        :return: None
        """
        self._set_encode_jpeg_func(self._alprstream_pointer, always_return_jpeg)

    def set_env_parameters(self, company_id, agent_uid, camera_id):
        """Set company, agent, and camera IDs for returned metadata.

        NOTE: this will also automatically set the UUID format for RollingDB.

        :param str company_id: Assigned ID for OpenALPR Cloud Stream service.
        :param str agent_uid: From /etc/openalpr/install_id.
        :param int camera_id: From /etc/openalpr/stream.d.
        :return: None 
        """
        self._set_env_parameters_func(
            self._alprstream_pointer, _convert_to_charp(company_id), _convert_to_charp(agent_uid), camera_id)
        self.set_uuid_format('{agent_uid}-{camera}-{time}')

    def set_group_parameters(self, min_plates_to_group=2, max_plates_per_group=10,
                             min_confidence=0.8, max_delta_time=1800):
        """Change grouping parameters for AlprStream behavior.

        :param int min_plates_to_group: Number of similar plates required to
            form a group.
        :param int max_plates_per_group: Maximum plates allowed in a group.
        :param float min_confidence: Minimum OCR confidence for a group.
        :param int max_delta_time: Plate reads separated by more than this
            will be put in different groups, in ms.
        :return: None
        """
        self._set_group_parameters_func(
            self._alprstream_pointer, min_plates_to_group, max_plates_per_group, min_confidence, max_delta_time)

    def set_jpeg_compression(self, jpeg_compression_level):
        """Set the jpeg compression level (between 1-100) for full-sized images
        :param jpeg_compression_level: 100 is high quality with a large file size, 1 is low quality with a small file size.  Default=85
        :return: None
        """

        self._set_jpeg_compression_func(self._alprstream_pointer, jpeg_compression_level)


    def enable_overview_image(self, enabled, max_width=560, max_height=315, jpeg_compression_level=55):
        """Enable/disable an overview JPEG to be base64 encoded and sent along with every plate result
        :param enabled: True/False to enable this option.  Default=disabled
        :param max_width: Set a maximum width for the overview image.  The image will be resized preserving the aspect ratio 
        :param max_height: Set a maximum height for the overview image.  The image will be resized preserving the aspect ratio 
        :param jpeg_compression_level: 100 is high quality with a large file size, 1 is low quality with a small file size.  
        :return: None
        """
        is_enabled_int = 0
        if enabled:
            is_enabled_int = 1
        self._enable_overview_image_func(self._alprstream_pointer, is_enabled_int, max_width, max_height, jpeg_compression_level)

    def set_uuid_format(self, format):
        """Sets the format used for generating UUIDs.

        The default is "{time}-{random}" and valid options are:
            - {time} - epoch_ms time the image was received
            - {frame} - Frame number (starts at 0)
            - {camera}, {company_uuid}, {agent_uid}
            - {random} - a 16 character random string

        :param format: A string containing the UUID format
        :return: None
        """
        format = _convert_to_charp(format)
        self._set_uuid_format_func(self._alprstream_pointer, format)

    def video_file_active(self):
        """Check the status of the video file thread.

        :return: True if currently active, false if inactive or complete
        """
        status = self._video_file_active_func(self._alprstream_pointer)
        return status

    def set_gpu_async(self, gpu_id):
        """Set the stream to preload video data onto the specified GPU.  For high-speed GPU applications, this is 
           a more efficient way of loading the GPU with image data (rather than synchronously)

        :param gpu_id: the GPU ID to preload with data
        """
        self._set_gpu_async_func(self._alprstream_pointer, gpu_id)