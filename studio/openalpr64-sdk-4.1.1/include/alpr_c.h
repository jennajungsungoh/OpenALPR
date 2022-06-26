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

/**
 * @file alpr_c.h
 * @brief OpenALPR C API for license plate recognition on single images.
 * 
 */

#ifndef ALPR_C_H
#define ALPR_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void OPENALPR;
typedef void OPENALPR_IMAGE_BATCH;

/**
 * Specifies a region of interest to restrict the area that OpenALPR analyzes.
 * The region(s) must be rectangular.  The x,y positions are specified in pixels 
 * and correspond to the top left corners of the rectangle.
 * The width and height are also specified in pixels. 
 * The coordinate system origin (0,0) is the top left of the image
 */
struct AlprCRegionOfInterest {
  int x;
  int y;
  int width;
  int height;
};

enum AlprCHardwareAcceleration { ALPRC_CPU = 0, ALPRC_NVIDIA_GPU = 1 };

/**
 * Initializes the openALPR library and returns a pointer to the OpenALPR instance
 * 
 * The instance should be reused many times.  Eventually you will want to destroy the instance 
 * when you are finished recognizing license plates.
 * 
 * see also openalpr_cleanup()
 * @return An OpenALPR instance that can be used with other openalpr functions
 */
OPENALPR* openalpr_init(const char* country, const char* configFile, const char* runtimeDir, const char* licenseKey);

/**
 * Initializes the openALPR library, overriding the openalpr.conf configuration for GPU
 * returns a pointer to the OpenALPR instance
 *
 * The instance should be reused many times.  Eventually you will want to destroy the instance
 * when you are finished recognizing license plates.
 *
 * see also openalpr_cleanup()
 * @return An OpenALPR instance that can be used with other openalpr functions
 */
OPENALPR* openalpr_init_gpu(const char* country, const char* configFile, const char* runtimeDir, const char* licenseKey,
                            int acceleration_type, int gpu_id, int batch_size);
/**
 * Verify that the OpenALPR library loaded correctly and can accept image inputs
 * 
 * see also alpr::Alpr::isLoaded() 
 * @return Returns 1 if the library was loaded successfully, 0 otherwise
 */
int openalpr_is_loaded(OPENALPR* instance);

/**
 * Set the country used for plate recognition
 * see also alpr::Alpr::setCountry() 
 */
void openalpr_set_country(OPENALPR* instance, const char* country);

/**
 * Update the detection mask without reloading the library
 * 
 * see also alpr::Alpr::setMask() 
 */
void openalpr_set_mask(OPENALPR* instance, unsigned char* pixelData, int bytesPerPixel, int imgWidth, int imgHeight);

// set_detect_region is deprecated.  Configure this via openalpr.conf instead
/**
 * /deprecated Enables/disables state/province detection.  Set this in openalpr.conf
 * 
 * see also alpr::Alpr::setDetectRegion() 
 */
void openalpr_set_detect_region(OPENALPR* instance, int detectRegion);

/**
 * /Enables/disables vehicle bounding box detection.
 * 
 * see also alpr::Alpr::setDetectVehicles() 
 */
void openalpr_set_detect_vehicles(OPENALPR* instance, int detectVehicles, int generate_tracking_signatures);

/**
 * Specify a region to use when applying patterns.  
 * 
 * see also alpr::Alpr::setTopN()
 */
void openalpr_set_topn(OPENALPR* instance, int topN);

/**
 * Specify a region to use when applying patterns.
 * 
 * see also alpr::Alpr::setDefaultRegion()
 */
void openalpr_set_default_region(OPENALPR* instance, const char* region);

/**
 * Sets the JPEG encoding quality used for encoding plate crops.  Default = 75
 * set to -1 to disable jpeg cropping
 * see also alpr::Alpr::set_jpeg_crop_quality()
*/
void openalpr_set_jpeg_crop_quality(OPENALPR* instance, int crop_quality);

/**
 * Recognizes the provided image and responds with JSON. 
 * Image is expected to be raw pixel data (BGR, 3 channels)
 * Caller must call openalpr_free_response_string() on the returned object
 * 
 * see also alpr::Alpr::recognize()
 * @return JSON formatted plate recognition results
 */
char* openalpr_recognize_rawimage(OPENALPR* instance, unsigned char* pixelData, int bytesPerPixel, int imgWidth,
                                  int imgHeight, struct AlprCRegionOfInterest roi);

/**
 * Recognizes the encoded (e.g., JPEG, PNG) image.  bytes are the raw bytes for the image data.
 * Caller must call openalpr_free_response_string() on the returned object
 * 
 * see also alpr::Alpr::recognize(), openalpr_recognize_rawimage()
 * @return JSON formatted plate recognition results
 */
char* openalpr_recognize_encodedimage(OPENALPR* instance, unsigned char* bytes, long long length,
                                      struct AlprCRegionOfInterest roi);

/**
 * Recognizes the encoded (e.g., JPEG, PNG) image from a file. input argument is the path to a file on disk. 
 * Caller must call openalpr_free_response_string() on the returned object
 *
 * see also alpr::Alpr::recognize(), openalpr_recognize_rawimage()
 * @return JSON formatted plate recognition results
 */
char* openalpr_recognize_imagefile(OPENALPR* instance, char* filepath);

/** 
 * Creates an object that can be used to send a batch of images to OpenALPR
 * Make sure to free the memory with openalpr_release_image_batch after you finish with the batch
 *
 * @return a pointer that can be passed into openalpr_add_image_to_batch and openalpr_recognize_batch
*/
OPENALPR_IMAGE_BATCH* openalpr_create_image_batch();

/**
 * Add an image to an OpenALPR batch object.
 * Image is expected to be raw pixel data (BGR, 3 channels)
 * The number of images should not exceed the configured batch_size property in openalpr.conf
 */
void openalpr_add_image_to_batch(OPENALPR_IMAGE_BATCH* batch, unsigned char* pixelData, int bytesPerPixel, int imgWidth,
                                 int imgHeight, struct AlprCRegionOfInterest roi);

/**
 * Add an encoded image (e.g., JPEG, PNG, BMP, etc) to the batch
 * @param bytes raw bytes for the encoded image
 * @param length length of the data
 * @param roi region of interest to look for license plates
 */
void openalpr_add_encoded_image_to_batch(OPENALPR_IMAGE_BATCH* batch, unsigned char* bytes, long long length,
                                         struct AlprCRegionOfInterest roi);

/**
 * Recognizes the provided batch of images and responds with JSON.
 * Caller must call openalpr_free_response_string() on the returned object
 *
 * @return JSON formatted plate recognition results for each image in the batch
 */
char* openalpr_recognize_batch(OPENALPR* instance, OPENALPR_IMAGE_BATCH* batch);

/**
 * Free memory associated with an image batch created by openalpr_create_image_batch()
 */
void openalpr_release_image_batch(OPENALPR_IMAGE_BATCH* batch);

/**
 * Returns the OpenALPR version number
 *
 * @return JSON formatted plate recognition results for each image in the batch
 */
char* openalpr_get_version();

/**
 * Frees a char* response that was provided from a recognition request.
 * You must call this function on every response to avoid memory leaks.
 * @param response The string returned from an openalpr_recognize function
 */
void openalpr_free_response_string(char* response);

/**
 * Free the memory for the OpenALPR instance created with openalpr_init
 * @param instance OpenALPR instance object
 */
void openalpr_cleanup(OPENALPR* instance);

char* openalpr_vehiclesignature_rawimage(OPENALPR* instance, unsigned char* pixelData, int bytesPerPixel, int imgWidth,
                                         int imgHeight, struct AlprCRegionOfInterest roi, int high_resolution);

char* openalpr_vehiclesignature_encodedimage(OPENALPR* instance, unsigned char* bytes, long long length,
                                             struct AlprCRegionOfInterest roi, int high_resolution);

double openalpr_vehiclesignature_similarity(OPENALPR* instance, char* signatureA, char* signatureB);

typedef void VEHICLECLASSIFIER;

/**
 * Initializes the vehicle classifier library and returns a pointer to the VEHICLECLASSIFIER instance
 * 
 * See also alpr::VehicleClassifier::VehicleClassifier()
 * @param device_type 0 = CPU, 1 = GPU 
 * @return An instance of the Vehicle Classifier that can be used in other functions.  
 *         You must destroy this instance with vehicleclassifier_cleanup() when you're done recognizing vehicles
 */
VEHICLECLASSIFIER* vehicleclassifier_init(const char* configFile, const char* runtimeDir, int device_type,
                                          int batch_size, int gpu_id, const char* licenseKey);

/**
 * Returns 1 if the library was loaded successfully, 0 otherwise
 * 
 * See also alpr::VehicleClassifier::isLoaded()
 */
int vehicleclassifier_is_loaded(VEHICLECLASSIFIER* instance);

/**
 * Recognizes the provided image and responds with JSON.  Image is expected to be raw pixel data (BGR, 3 channels)
 * Caller must call vehicleclassifier_free_response_string() on the returned object
 * 
 * See also alpr::VehicleClassifier::detect()
 * @return a JSON-formatted string containing the vehicle recognition results
 */
char* vehicleclassifier_recognize_rawimage(VEHICLECLASSIFIER* instance, const char* country, unsigned char* pixelData,
                                           int bytesPerPixel, int imgWidth, int imgHeight,
                                           struct AlprCRegionOfInterest roi);

// Recognizes the encoded (e.g., JPEG, PNG) image.  bytes are the raw bytes for the image data.
/**
 * Recognizes the provided image and responds with JSON.  Image is expected to be an encoded image (e.g., PNG, JPEG, BMP, etc).
 * Caller must call vehicleclassifier_free_response_string() on the returned object
 * 
 * See also alpr::VehicleClassifier::detect()
 * @return a JSON-formatted string containing the vehicle recognition results
 */
char* vehicleclassifier_recognize_encodedimage(VEHICLECLASSIFIER* instance, const char* country, unsigned char* bytes,
                                               long long length, struct AlprCRegionOfInterest roi);

// Recognizes the encoded (e.g., JPEG, PNG) image from a file on disk.
/**
 * Recognizes the provided file and responds with JSON.  Image file is expected to be an encoded image (e.g., PNG, JPEG, BMP, etc).
 * Caller must call vehicleclassifier_free_response_string() on the returned object
 * 
 * See also alpr::VehicleClassifier::detect()
 * @return a JSON-formatted string containing the vehicle recognition results
 */
char* vehicleclassifier_recognize_imagefile(VEHICLECLASSIFIER* instance, const char* country, char* image_path);

//
/**
 * Specify the maximum number of candidates to return
 * 
 * See also alpr::VehicleClassifier::setTopN()
 */
void vehicleclassifier_set_topn(VEHICLECLASSIFIER* instance, int topN);

/**
 * Frees a char* response that was provided from a recognition request.
 * @param response a JSON char* object from a previous recognize request
 */
void vehicleclassifier_free_response_string(char* response);

/**
 * Cleans up the VEHICLECLASSIFIER object and frees memory.  Use this once you are 
 * finished recognizing vehicles.
 * @param instance The VEHICLECLASSIFIER instance created with vehicleclassifier_init()
 */
void vehicleclassifier_cleanup(VEHICLECLASSIFIER* instance);

#ifdef __cplusplus
}
#endif

#endif /* ALPR_C_H */
