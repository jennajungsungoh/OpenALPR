Rekor Scout SDK 64-bit binaries for Windows
=================================================


Licensing
-----------

Before running the software you must obtain a commercial license key.  Please visit fill out the form at https://license.openalpr.com/evalrequest/ 
to receive a free 2-week evaluation key sent to your e-mail.

Paste the license key in the license.conf file to begin using the software.  Alternatively, you can set the license as an environment variable 
named OPENALPR_LICENSE_KEY.


CLI Application
---------------

alpr.exe is a command-line application that can analyze license plates.  Type alpr --help from the Windows command prompt for more information.

Examples: 

    # Recognize a US-style plate
    alpr -c us samples/us-1.jpg

    # Recognize a US-style plate and measure the processing time
    alpr -c us --clock samples/us-1.jpg

    # Recognize a European-style plate with JSON output
    alpr -c eu -j samples/eu-1.jpg


API Integration
----------

Rekor Scout SDK is written in C++ and has native support for integrating with C/C++ applications.

To include Rekor Scout SDK in your C/C++ application, include the alpr_c.h file in the include directory.  You must also link the libopenalpr.dll 
shared library to your code. 

Rekor Scout SDK also includes bindings for C#, Java, and Python.  Each of these bindings fully implement the Rekor Scout API.

All of the DLLs in the root directory are required at runtime in order for the application to function.  If you run Rekor Scout in C#, Java, 
or Python, for example, the native DLLs must be accessible to the program so that it can load.


C#
----

The C# integration is bundled into the DLL file "alprnet.dll"  You should add this as a reference to your .NET application to use the functionality.
You may also include the C# binding source code in your project.

The C# library includes two example applications.  The full source code is available at 
https://github.com/openalpr/openalpr/tree/master/src/bindings/csharp


Java
-------

Run the java_test.bat program to test the Java integration.  This assumes that javac and java are available on your system PATH.  

The java integration uses Java Native Interface (JNI) to connect java code to Rekor Scout.  This requires the openalprjni.dll file is available at runtime. 
The java source code is in java/com/openalpr/jni and there is in example located in java/Main.java


Python
--------

Run the python_test.bat program to test the Python integration.  This assumes that python is available on your system PATH.

The Python integration uses ctypes to bind the Rekor Scout functions to Python code.  This requires the libopenalprpy.dll file is available at runtime.
The python code is located in python/openalpr.py.  There is a test program in python/test.py


AlprStream API
--------------

AlprStream is a C API that is used for license plate recognition on videos or time-ordered image sequences.

The AlprStream API organizes video streams so that they can be efficiently processed by Rekor Scout SDK. The library 
utilizes motion detection to improve performance, and groups sequential plate results for each vehicle into a 
single result with the highest possible confidence. AlprStream also buffers incoming video and handles cases 
where video input is faster than the available processing by evenly dropping frames from the video buffer.

You may configure AlprStream to connect directly to a video feed (either an IP camera stream URL or a file). 
AlprStream will create a background thread that constantly pulls from the source to keep the video buffer full. 
You may also feed video frames directly. If sending video manually, you should do this on a separate thread.

AlprStream maintains a buffer of frames for processing. You may share an AlprStream object across threads. Each 
thread should maintain its own Alpr object. Passing the Alpr object to AlprStream using a process_frame() call 
performs a recognition and returns individual results. The AlprStream object keeps track of these results and 
forms plate groups.

Plate Groups can be accessed at any time by popping or peeking from the active list. Once a plate group is fully 
formed, and we are confident that the vehicle is no longer in the scene, it is available to be popped. Otherwise, 
it will remain on the list that you can peek, until enough time has passed.

Vehicle recognition is optionally run on each AlprGroup after it has been popped. The vehicle recognition is CPU 
intensive. AlprStream uses a region centered around the license plate to perform the vehicle recognition.

The AlprStream object should be initialized once for each video stream. The initialization time is minimal. 
The AlprStream instance is threadsafe.


Additional Documentation
--------------------------

Please visit http://doc.openalpr.com for additional documentation
