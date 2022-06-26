/*************************************************************************
 * OPENALPR CONFIDENTIAL
 * 
 *  Copyright 2017 OpenALPR Technology, Inc.
 *  All Rights Reserved.
 * 
 * NOTICE:  All information contained herein is, and remains
 * the property of OpenALPR Technology Incorporated. The intellectual
 * and technical concepts contained herein are proprietary to OpenALPR  
 * Technology Incorporated and may be covered by U.S. and Foreign Patents.
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from OpenALPR Technology Incorporated.
 */

#ifndef ALPRSTREAM_C_H
#define ALPRSTREAM_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <alpr_c.h>

/**
 * void ALPRSTREAM
 * 
 * The instantiated AlprStream object.  This is created by the 
 * alprstream_init() function and should be passed into subsequent functions.
 */
typedef void ALPRSTREAM;

/**
 * @file alprstream_c.h
 * @brief OpenALPR AlprStream C API for license plate recognition on videos or time-ordered image sequences.
 *
 */

/// See alpr::RecognizedFrame
struct AlprStreamRecognizedFrameC {
  bool image_available;
  char* jpeg_bytes;
  int64_t jpeg_bytes_size;
  int64_t frame_epoch_time_ms;
  int64_t frame_number;
  char* results_str;
};

struct AlprStreamRecognizedBatchC {
  struct AlprStreamRecognizedFrameC** results_array;
  size_t results_size;
  char* batch_results;
};

/**
 * Initializes the ALPRSTREAM instance.  Each stream of video should have its own
 * AlprStream instance.  This object is threadsafe.
 * 
 * @return an ALPRSTREAM instance that can be used in future calls.  Call alprstream_cleanup() once you're finished with the object.
 * See also alpr::AlprStream::AlprStream()
 */
ALPRSTREAM* alprstream_init(int frame_queue_size, int use_motion_detection);

/**
 * Check the size of the video buffer
 * 
 * See also alpr::AlprStream::get_queue_size()
 */
int alprstream_get_queue_size(ALPRSTREAM* instance);

/// See alpr::AlprStream::get_video_file_fps()
double alprstream_get_video_file_fps(ALPRSTREAM* instance);

/// See alpr::AlprStream::connect_video_stream_url()
void alprstream_connect_video_stream_url(ALPRSTREAM* instance, const char* url, const char* gstreamer_format);

/// See alpr::AlprStream::disconnect_video_stream()
void alprstream_disconnect_video_stream(ALPRSTREAM* instance);

/// See alpr::AlprStream::connect_video_file()
void alprstream_connect_video_file(ALPRSTREAM* instance, const char* video_file_path, int64_t video_start_time);

/// See alpr::AlprStream::disconnect_video_file()
void alprstream_disconnect_video_file(ALPRSTREAM* instance);

/// See alpr::AlprStream::video_file_active()
int alprstream_video_file_active(ALPRSTREAM* instance);

/// See alpr::AlprStream::push_frame()
int alprstream_push_frame_encoded(ALPRSTREAM* instance, unsigned char* bytes, long long length,
                                  int64_t frame_epoch_time);

/// See alpr::AlprStream::push_frame()
int alprstream_push_frame(ALPRSTREAM* instance, unsigned char* pixelData, int bytesPerPixel, int imgWidth,
                          int imgHeight, int64_t frame_epoch_time);

/// See alpr::AlprStream::push_frame_gpu()
int alprstream_push_frame_gpu(ALPRSTREAM* instance, void* gpu_pointer, int bytesPerPixel, int imgWidth, int imgHeight,
                              int64_t frame_epoch_time);

/// See alpr::AlprStream::process_frame().  Each response must be freed with alprstream_free_frame_response()
struct AlprStreamRecognizedFrameC* alprstream_process_frame(ALPRSTREAM* instance, OPENALPR* alpr);

/// Frees memory from an alpr_stream_process_frame() response
void alprstream_free_frame_response(struct AlprStreamRecognizedFrameC* response);

/// Frees memory from an alprstream_process_batch() response
void alprstream_free_batch_response(struct AlprStreamRecognizedBatchC* response);

/// See alpr::AlprStream::skip_frame()
struct AlprStreamRecognizedFrameC* alprstream_skip_frame(ALPRSTREAM* instance, int return_image);

/// See alpr::AlprStream::process_batch()
struct AlprStreamRecognizedBatchC* alprstream_process_batch(ALPRSTREAM* instance, OPENALPR* alpr);

/**
 * See alpr::AlprStream::pop_completed_groups()
 * @param include_images Encodes the JPEG image as base64 in the response
 * @return a JSON-formatted string describing the list of completed group results. 
 *         Make sure to release the string memory using alprstream_free_response_string() after using it
 */
char* alprstream_pop_completed_groups(ALPRSTREAM* instance, int include_images);

/**
 * See alpr::AlprStream::peek_active_groups()
 * @return a JSON-formatted string describing the list of active group results. 
 *         Make sure to release the string memory using alprstream_free_response_string() after using it
 */
char* alprstream_peek_active_groups(ALPRSTREAM* instance);

/**
 * \deprecated
 * Gets the completed groups from the list and performs vehicle make/model/color/type 
 * recognition before returning vehicle results.  This is a CPU or GPU intensive operation
 * so it may be best to perform on a separate thread.
 * The function alprstream_pop_completed_groups_and_recognize_vehicle_signature is a superset of this function that also includes vehicle signature
 * This function will be removed in a future version and is left for backwards compatibility
 * 
 * @param vehicle_classifier An initialized instance of the VehicleClassifier that AlprStream will use to perform vehicle recognition
 * @param include_images Encodes the JPEG image as base64 in the response
 * @return a JSON-formatted string describing the list of completed group results.  Each group result will contain vehicle information as well.
 *         Make sure to release the string memory using alprstream_free_response_string() after using it
 */
char* alprstream_pop_completed_groups_and_recognize_vehicle(ALPRSTREAM* instance, VEHICLECLASSIFIER* vehicle_classifier,
                                                            int include_images);
/**
 * Gets the completed groups from the list and performs vehicle make/model/color/type 
 * recognition before returning vehicle results.  This is a CPU or GPU intensive operation
 * so it may be best to perform on a separate thread.
 * This also recognizes the vehicle signature
 * 
 * @param alpr_obj An initialized instance of Alpr that AlprStream will use to perform vehicle signature recognition
 * @param vehicle_classifier An initialized instance of the VehicleClassifier that AlprStream will use to perform vehicle recognition
 * @param include_images Encodes the JPEG image as base64 in the response
 * @return a JSON-formatted string describing the list of completed group results.  Each group result will contain vehicle information as well.
 *         Make sure to release the string memory using alprstream_free_response_string() after using it
 */
char* alprstream_pop_completed_groups_and_recognize_vehicle_signature(ALPRSTREAM* instance, OPENALPR* alpr_obj,
                                                                      VEHICLECLASSIFIER* vehicle_classifier,
                                                                      int include_images);

/// See alpr::AlprStream::combine_grouping()
void alprstream_combine_grouping(ALPRSTREAM* instance, ALPRSTREAM* other_stream);

/// See alpr::AlprStream::set_uuid_format()
void alprstream_set_uuid_format(ALPRSTREAM* instance, const char* format);

/// See alpr::AlprStream::set_parked_vehicle_detect_params()
void alprstream_set_parked_vehicle_detect_params(ALPRSTREAM* instance, int enabled, long long max_delta_ms,
                                                 float max_distance_multiplier, int max_text_distance);

/// See alpr::AlprStream::set_group_parameters()
void alprstream_set_group_parameters(ALPRSTREAM* instance, int min_plates_to_group, int max_plates_per_group,
                                     float min_confidence, int max_delta_time);

/// See alpr::AlprStream::set_group_preview();
void alprstream_set_group_preview(ALPRSTREAM* instance, int enabled, int minimum_reads);

/// See alpr::AlprStream::set_env_parameters()
void alprstream_set_env_parameters(ALPRSTREAM* instance, const char* company_id, const char* agent_uid, int camera_id);

/// See alpr::AlprStream::set_detection_mask()
void alprstream_set_detection_mask_encoded(ALPRSTREAM* instance, unsigned char* bytes, long long length);

/// See alpr::AlprStream::set_detection_mask()
void alprstream_set_detection_mask(ALPRSTREAM* instance, unsigned char* pixelData, int bytesPerPixel, int imgWidth,
                                   int imgHeight);

/// See alpr::AlprStream::set_jpeg_compression()
void alprstream_set_jpeg_compression(ALPRSTREAM* instance, int compression_level);

/// See alpr::AlprStream::set_jpeg_crop_compression()
void alprstream_set_jpeg_crop_compression(ALPRSTREAM* instance, int plate_compression_level,
                                          int vehicle_compression_level);

/// See alpr::AlprStream::enable_overview_jpeg()
void alprstream_enable_overview_jpeg(ALPRSTREAM* instance, int enabled, int max_width, int max_height,
                                     int jpeg_compression_level);

void alprstream_set_encode_jpeg(ALPRSTREAM* instance, int always_return_jpeg);

/// See alpr::AlprStream::set_record_video()
void alprstream_set_record_video(ALPRSTREAM* instance, int enabled, int max_storage_size_gb, char* rolling_db_path);

/// See alpr::AlprStream::set_gpu_async()
void alprstream_set_gpu_async(ALPRSTREAM* instance, int gpu_id);

/**
 * Frees a char* response that was provided from a recognition request.
 * @param response A JSON character string from a previous pop/peek operation
 */
void alprstream_free_response_string(char* response);

/// Free the memory for the OpenALPR instance created with alprstream_init()
void alprstream_cleanup(ALPRSTREAM* instance);

#ifdef __cplusplus
}
#endif

#endif /* ALPRSTREAM_C_H */
