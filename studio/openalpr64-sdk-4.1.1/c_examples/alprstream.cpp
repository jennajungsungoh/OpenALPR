// OpenALPR sample for alprstreamgpu library
// Copyright 2017, OpenALPR Technology, Inc.

// System imports
#include <Windows.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>

// Import OpenALPR alprstreamgpu (also pulls in alprgpu.h and alpr.h)
// Object definitions for individual results are found in alpr.h
// Object definitions for group results are found in alprstreamgpu.h
#include <alprstream_c.h>
#include <alpr_c.h>

using namespace std;


int main(int argc, char** argv) {

  cout << "Initializing" << endl;
  const std::string LICENSEPLATE_COUNTRY = "eu";
  
  
  // Size of image buffer to maintain in stream queue -- This only matters if you are feeding
  // images/video into the buffer faster than can be processed (i.e., on a background thread)
  // Setting this to the batch size since we're feeding in images synchronously, so it's only needed to 
  // hold a single batch

  // Batch size and GPU ID set in openalpr.conf
  // Video buffer frames controls the number of frames to buffer in memory.  Must be >= gpu batch size
  const int VIDEO_BUFFER_SIZE = 15;
  
  // The stream will assume sequential frames.  If there is no motion from frame to frame, then 
  // processing can be skipped for some frames
  const int USE_MOTION_DETECTION = 1;

  // The point in time (ms) to start in the video file
  const int VIDEO_START_MS = 0;

  OPENALPR* alpr = openalpr_init(LICENSEPLATE_COUNTRY.c_str(), "", "", "");
  ALPRSTREAM* stream = alprstream_init(VIDEO_BUFFER_SIZE, USE_MOTION_DETECTION);
  alprstream_connect_video_file(stream, "C:/Temp/eu-clip.mp4", VIDEO_START_MS);
  
  // Process until the video file is done and all remaining frames in the buffer have been processed
  while (alprstream_video_file_active(stream) || alprstream_get_queue_size(stream) > 0)
  {
	  // If the buffer is empty wait for it to replenish
	  if (alprstream_get_queue_size(stream) <= 0)
		  Sleep(100);

	  AlprStreamRecognizedFrameC* frameresult = alprstream_process_frame(stream, alpr);
	  
	  cout << "Content: " << frameresult->results_str << endl;

    // Free the memory
	  alprstream_free_frame_response(frameresult);

	  cout << "Stream queue size: " << alprstream_get_queue_size(stream) << endl;
  }

  char* group_json = alprstream_pop_completed_groups(stream);
  cout << "Groups: " << group_json << endl;

  alprstream_free_response_string(group_json);
  cout << "Done" << endl;
  
  return 0;
}


