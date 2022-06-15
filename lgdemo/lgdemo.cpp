// lgdemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "NetworkTCP.h"
#include <windows.h>
#include <map>
#include <iostream>
#include <stdlib.h>
#include <tchar.h> 
#include "opencv2/opencv.hpp"
#include "support/timing.h"
#include "motiondetector.h"
#include "alpr.h"
#include "DeviceEnumerator.h"


using namespace alpr;
using namespace std;
using namespace cv;


enum class Mode { mNone, mLive_Video,mPlayback_Video, mImage_File };
enum class VideoResolution { rNone, r640X480, r1280X720 };
enum class VideoSaveMode { vNone, vNoSave, vSave, vSaveWithNoALPR};
enum class ResponseMode { ReadingHeader,ReadingMsg };

ResponseMode GetResponseMode= ResponseMode::ReadingHeader;
short RespHdrNumBytes;
char ResponseBuffer[2048];
unsigned int BytesInResponseBuffer = 0;
ssize_t BytesNeeded = sizeof(RespHdrNumBytes);


bool measureProcessingTime = false;
std::string templatePattern;
MotionDetector motiondetector;
bool do_motiondetection = false;

bool _qpcInited = false;
double PCFreq = 0.0;
__int64 CounterStart = 0;
double _avgdur = 0;
double _fpsstart = 0;
double _avgfps = 0;
double _fps1sec = 0;
TTcpConnectedPort* TcpConnectedPort;

#define NUMBEROFPREVIOUSPLATES 10
char LastPlates[NUMBEROFPREVIOUSPLATES][64]={"","","","",""};
unsigned int CurrentPlate = 0;

static VideoSaveMode GetVideoSaveMode(void);
static VideoResolution GetVideoResolution(void);
static Mode GetVideoMode(void);
static int GetVideoDevice(void);
static bool GetFileName(Mode mode, char filename[MAX_PATH]);
static bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
static void InitCounter();
static double CLOCK();
static bool getconchar(KEY_EVENT_RECORD& krec);
static double avgdur(double newdur);
static double avgfps();
static void GetResponses(void);
/***********************************************************************************/
/* Main                                                                            */
/***********************************************************************************/
int main()
{
    Mode mode;
    VideoSaveMode videosavemode;
    VideoResolution vres;

    char filename[MAX_PATH];

    char text[1024] = "";
    int frameno = 0;
    VideoCapture cap;
    VideoWriter outputVideo;
    int deviceID=-1;

    int apiID = cv::CAP_ANY;      // 0 = autodetect default API

    std::string county;


    if ((TcpConnectedPort = OpenTcpConnection("127.0.0.1", "2222")) == NULL)
    {
        std::cout << "Connection Failed" << std::endl;
        return(-1);
    }
    else std::cout << "Connected" << std::endl;

    county = "us";

    mode = GetVideoMode();
    if (mode == Mode::mNone) exit(0);


    if (mode == Mode::mLive_Video)
    {
        deviceID = GetVideoDevice();
        if (deviceID == -1) exit(0);
        vres = GetVideoResolution();
        if (vres == VideoResolution::rNone) exit(0);
    }
    else
    {
        if (GetFileName(mode, filename)) std::cout << "Filename is " << filename << std::endl;
        else exit(0);
    }

    if (mode != Mode::mImage_File)
    {
        videosavemode = GetVideoSaveMode();
        if (videosavemode == VideoSaveMode::vNone) exit(0);
    }
    else videosavemode = VideoSaveMode::vNoSave;

    Alpr alpr(county, "");
    alpr.setTopN(2);
    if (alpr.isLoaded() == false)
    {
        std::cerr << "Error loading OpenALPR" << std::endl;
        return 1;
    }

    if (mode == Mode::mLive_Video)
    {
        // open selected camera using selected API
        cap.open(deviceID, apiID);
        if (!cap.isOpened()) {
            cout << "Error opening video stream" << endl;
            return -1;
        }
    }
    else if (mode == Mode::mPlayback_Video)
    {
        cap.open(filename);
        if (!cap.isOpened()) {
            cout << "Error opening video file" << endl;
            return -1;
        }
    }

    if (vres == VideoResolution::r1280X720)
    {
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    }
    else if (vres == VideoResolution::r640X480)
    {
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    }
    if (mode != Mode::mImage_File)
    {
        // Default resolutions of the frame are obtained.The default resolutions are system dependent.
        int frame_width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        int frame_height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        printf("Frame width= %d height=%d\n", frame_width, frame_height);


        // Define the codec and create VideoWriter object.The output is stored in 'outcpp.avi' file.
        if (videosavemode != VideoSaveMode::vNoSave)
        {

            outputVideo.open("output.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 25, Size(frame_width, frame_height), true);
            if (!outputVideo.isOpened())
            {
                cout << "Could not open the output video for write" << endl;
                return -1;
            }
        }
    }


    while (1) {

        Mat frame;
        double start = CLOCK();
        // Capture frame-by-frame
        if (mode == Mode::mImage_File)
        {
            frame = imread(filename);
        }
        else cap >> frame;

        // 
        // If the frame is empty, break immediately
        if (frame.empty())
            break;

        if (frameno == 0) motiondetector.ResetMotionDetection(&frame);
        if (videosavemode != VideoSaveMode::vSaveWithNoALPR)
        {
            detectandshow(&alpr, frame, "", false);
            GetResponses();

            cv::putText(frame, text,
                cv::Point(10, frame.rows - 10), //top-left position
                FONT_HERSHEY_COMPLEX_SMALL, 0.5,
                Scalar(0, 255, 0), 0, LINE_AA, false);

        }

        // Write the frame into the file 'outcpp.avi'
        if (videosavemode != VideoSaveMode::vNoSave)
        {
            outputVideo.write(frame);
        }

        // Display the resulting frame    
        imshow("Frame", frame);

        // Press  ESC on keyboard to  exit
        char c = (char)waitKey(1);
        if (c == 27)
            break;
        double dur = CLOCK() - start;
        sprintf_s(text, "avg time per frame %f ms. fps %f. frameno = %d", avgdur(dur), avgfps(), frameno++);
    }

    // When everything done, release the video capture and write object
    cap.release();
    if (videosavemode != VideoSaveMode::vNoSave)  outputVideo.release();

    // Closes all the frames
    destroyAllWindows();
    return 0;
}
/***********************************************************************************/
/* End Main                                                                        */
/***********************************************************************************/
/***********************************************************************************/
/* detectandshow                                                                   */
/***********************************************************************************/
static bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson)
{

    timespec startTime;
    getTimeMonotonic(&startTime);
    unsigned short SendPlateStringLength;
    ssize_t result;

    std::vector<AlprRegionOfInterest> regionsOfInterest;
    if (do_motiondetection)
    {
        cv::Rect rectan = motiondetector.MotionDetect(&frame);
        if (rectan.width > 0) regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));
    }
    else regionsOfInterest.push_back(AlprRegionOfInterest(0, 0, frame.cols, frame.rows));
    AlprResults results;
    if (regionsOfInterest.size() > 0) results = alpr->recognize(frame.data, (int)frame.elemSize(), frame.cols, frame.rows, regionsOfInterest);

    timespec endTime;
    getTimeMonotonic(&endTime);
    double totalProcessingTime = diffclock(startTime, endTime);
    if (measureProcessingTime)
        std::cout << "Total Time to process image: " << totalProcessingTime << "ms." << std::endl;


    if (writeJson)
    {
        std::cout << alpr->toJson(results) << std::endl;
    }
    else
    {
        for (int i = 0; i < results.plates.size(); i++)
        {
            char textbuffer[1024];
            std::vector<cv::Point2f> pointset;
            for (int z = 0; z < 4; z++)
                pointset.push_back(Point2i(results.plates[i].plate_points[z].x, results.plates[i].plate_points[z].y));
            cv::Rect rect = cv::boundingRect(pointset);
            cv::rectangle(frame, rect, cv::Scalar(0, 255, 0),2);
            sprintf_s(textbuffer, "%s - %.2f", results.plates[i].bestPlate.characters.c_str(), results.plates[i].bestPlate.overall_confidence);

            cv::putText(frame, textbuffer,
                cv::Point(rect.x, rect.y-5), //top-left position
                FONT_HERSHEY_COMPLEX_SMALL, 1,
                Scalar(0, 255, 0), 0, LINE_AA, false);
            if (TcpConnectedPort)
            {
                bool found = false;
                for (int x = 0; x < NUMBEROFPREVIOUSPLATES; x++)
                {
                    if (strcmp(results.plates[i].bestPlate.characters.c_str(), LastPlates[x]) == 0)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    unsigned short SendMsgHdr;
                    SendPlateStringLength = (unsigned short)strlen(results.plates[i].bestPlate.characters.c_str())+1;
                    SendMsgHdr = htons(SendPlateStringLength);
                    if ((result = (int)WriteDataTcp(TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendPlateStringLength))
                        printf("WriteDataTcp %d\n", result);
                    if ((result = (int)WriteDataTcp(TcpConnectedPort, (unsigned char*)results.plates[i].bestPlate.characters.c_str(), SendPlateStringLength)) != SendPlateStringLength)
                        printf("WriteDataTcp %d\n", result);
                    printf("sent ->%s\n", results.plates[i].bestPlate.characters.c_str());
                }
            }
            strcpy_s(LastPlates[CurrentPlate], results.plates[i].bestPlate.characters.c_str());
            CurrentPlate = (CurrentPlate + 1) % NUMBEROFPREVIOUSPLATES;
#if 0
            std::cout << "plate" << i << ": " << results.plates[i].topNPlates.size() << " results";
            if (measureProcessingTime)
                std::cout << " -- Processing Time = " << results.plates[i].processing_time_ms << "ms.";
            std::cout << std::endl;

            if (results.plates[i].regionConfidence > 0)
                std::cout << "State ID: " << results.plates[i].region << " (" << results.plates[i].regionConfidence << "% confidence)" << std::endl;

            for (int k = 0; k < results.plates[i].topNPlates.size(); k++)
            {
                // Replace the multiline newline character with a dash
                std::string no_newline = results.plates[i].topNPlates[k].characters;
                std::replace(no_newline.begin(), no_newline.end(), '\n', '-');

                std::cout << "    - " << no_newline << "\t confidence: " << results.plates[i].topNPlates[k].overall_confidence;
                if (templatePattern.size() > 0 || results.plates[i].regionConfidence > 0)
                    std::cout << "\t pattern_match: " << results.plates[i].topNPlates[k].matches_template;

                std::cout << std::endl;
            }
#endif
        }
    }
    return results.plates.size() > 0;
}
/***********************************************************************************/
/* End detectandshow                                                               */
/***********************************************************************************/
/***********************************************************************************/
/* InitCounter                                                                     */
/***********************************************************************************/
static void InitCounter()
{
    LARGE_INTEGER li;
    if (!QueryPerformanceFrequency(&li))
    {
        std::cout << "QueryPerformanceFrequency failed!\n";
    }
    PCFreq = double(li.QuadPart) / 1000.0f;
    _qpcInited = true;
}
/***********************************************************************************/
/* End InitCounter                                                                 */
/***********************************************************************************/
/***********************************************************************************/
/* Clock                                                                           */
/***********************************************************************************/
static double CLOCK()
{
    if (!_qpcInited) InitCounter();
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart) / PCFreq;
}
/***********************************************************************************/
/* End Clock                                                                       */
/***********************************************************************************/
/***********************************************************************************/
/* Avgdur                                                                          */
/***********************************************************************************/
static double avgdur(double newdur)
{
    _avgdur = 0.98 * _avgdur + 0.02 * newdur;
    return _avgdur;
}
/***********************************************************************************/
/* End Avgdur                                                                      */
/***********************************************************************************/
/***********************************************************************************/
/* Avgfps                                                                          */
/***********************************************************************************/
static double avgfps()
{
    if (CLOCK() - _fpsstart > 1000)
    {
        _fpsstart = CLOCK();
        _avgfps = 0.7 * _avgfps + 0.3 * _fps1sec;
        _fps1sec = 0;
    }
    _fps1sec++;
    return _avgfps;
}
/***********************************************************************************/
/*  End Avgfps                                                                     */
/***********************************************************************************/
/***********************************************************************************/
/* Get Console Char                                                                */
/***********************************************************************************/
static bool getconchar(KEY_EVENT_RECORD& krec)
{
    DWORD cc;
    INPUT_RECORD irec;
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

    if (h == NULL)
    {
        return false; // console not found
    }

    for (; ; )
    {
        ReadConsoleInput(h, &irec, 1, &cc);
        if (irec.EventType == KEY_EVENT
            && ((KEY_EVENT_RECORD&)irec.Event).bKeyDown
            )//&& ! ((KEY_EVENT_RECORD&)irec.Event).wRepeatCount )
        {
            krec = (KEY_EVENT_RECORD&)irec.Event;
            return true;
        }
    }
    return false; //future ????
}
/***********************************************************************************/
/* End Get Console Char                                                            */
/***********************************************************************************/
/***********************************************************************************/
/* Get File Name                                                                   */
/***********************************************************************************/
static bool GetFileName(Mode mode,char filename[MAX_PATH])
{
    TCHAR CWD[MAX_PATH];
    bool retval = true;
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH];
    ZeroMemory(&szFile, sizeof(szFile));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
    if (mode ==Mode::mImage_File) ofn.lpstrFilter = _T("Image Files\0*.png;*.jpg;*.tif;*.bmp;*.jpeg;*.gif\0Any File\0*.*\0");
    else if (mode == Mode::mPlayback_Video) ofn.lpstrFilter = _T("Video Files\0*.avi;*.mp4;*.webm;*.flv;*.mjpg;*.mjpeg\0Any File\0*.*\0");
    else ofn.lpstrFilter = _T("Text Files\0*.txt\0Any File\0*.*\0");
    ofn.lpstrFile = LPWSTR(szFile);
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = _T("Select a File, to Processs");
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
    GetCurrentDirectory(MAX_PATH, CWD);
    if (GetOpenFileName(&ofn))
    {
        size_t output_size;
        wcstombs_s(&output_size,filename, MAX_PATH, ofn.lpstrFile, MAX_PATH);
    }
    else
    {
        // All this stuff below is to tell you exactly how you messed up above. 
        // Once you've got that fixed, you can often (not always!) reduce it to a 'user cancelled' assumption.
        switch (CommDlgExtendedError())
        {
        case CDERR_DIALOGFAILURE: std::cout << "CDERR_DIALOGFAILURE\n";   break;
        case CDERR_FINDRESFAILURE: std::cout << "CDERR_FINDRESFAILURE\n";  break;
        case CDERR_INITIALIZATION: std::cout << "CDERR_INITIALIZATION\n";  break;
        case CDERR_LOADRESFAILURE: std::cout << "CDERR_LOADRESFAILURE\n";  break;
        case CDERR_LOADSTRFAILURE: std::cout << "CDERR_LOADSTRFAILURE\n";  break;
        case CDERR_LOCKRESFAILURE: std::cout << "CDERR_LOCKRESFAILURE\n";  break;
        case CDERR_MEMALLOCFAILURE: std::cout << "CDERR_MEMALLOCFAILURE\n"; break;
        case CDERR_MEMLOCKFAILURE: std::cout << "CDERR_MEMLOCKFAILURE\n";  break;
        case CDERR_NOHINSTANCE: std::cout << "CDERR_NOHINSTANCE\n";     break;
        case CDERR_NOHOOK: std::cout << "CDERR_NOHOOK\n";          break;
        case CDERR_NOTEMPLATE: std::cout << "CDERR_NOTEMPLATE\n";      break;
        case CDERR_STRUCTSIZE: std::cout << "CDERR_STRUCTSIZE\n";      break;
        case FNERR_BUFFERTOOSMALL: std::cout << "FNERR_BUFFERTOOSMALL\n";  break;
        case FNERR_INVALIDFILENAME: std::cout << "FNERR_INVALIDFILENAME\n"; break;
        case FNERR_SUBCLASSFAILURE: std::cout << "FNERR_SUBCLASSFAILURE\n"; break;
        default: std::cout << "You cancelled.\n";
            retval=false;
        }
    }
    SetCurrentDirectory(CWD);
    return(retval);
}
/***********************************************************************************/
/* End Get File Name                                                               */
/***********************************************************************************/

/***********************************************************************************/
/* GetVideoMode                                                                    */
/***********************************************************************************/
static Mode GetVideoMode(void)
{
    KEY_EVENT_RECORD key;
    Mode mode= Mode::mNone;
    do
    {
        std::cout << "Select Live Video, PlayBack File or Image File" << std::endl;
        std::cout << "1 - Live Video" << std::endl;
        std::cout << "2 - PlayBack File" << std::endl;
        std::cout << "3 - Image File" << std::endl;
        std::cout << "E - Exit" << std::endl;

        getconchar(key);
        std::cout << key.uChar.AsciiChar << std::endl;
        if ((key.uChar.AsciiChar == 'E') || (key.uChar.AsciiChar == 'e')) break;
        else if (key.uChar.AsciiChar == '1') mode = Mode::mLive_Video;
        else if (key.uChar.AsciiChar == '2') mode = Mode::mPlayback_Video;
        else if (key.uChar.AsciiChar == '3') mode = Mode::mImage_File;
        else std::cout << "Invalid Input" << std::endl << std::endl;
    } while (mode == Mode::mNone);
    return(mode);
}
/***********************************************************************************/
/* End GetVideoMode                                                                */
/***********************************************************************************/
/***********************************************************************************/
/* GetVideoDevice                                                                */
/***********************************************************************************/
static int GetVideoDevice(void)
{
    int deviceID = -1;
    KEY_EVENT_RECORD key;
    int numdev;
    DeviceEnumerator de;
    std::map<int, Device> devices = de.getVideoDevicesMap();

    int* deviceid = new int[devices.size()];
    do {
        numdev = 0;
        std::cout << "Select video Device" << std::endl;
        for (auto const& device : devices)
        {
            deviceid[numdev] = device.first;
            std::cout << numdev + 1 << " - " << device.second.deviceName << std::endl;
            numdev++;
        }
        std::cout << "E - exit" << std::endl;
        getconchar(key);
        if ((key.uChar.AsciiChar == 'E') || (key.uChar.AsciiChar == 'e')) break;
        int value = static_cast<int>(key.uChar.AsciiChar) - 48;
        if ((value >= 1) && value <= numdev) deviceID = deviceid[value - 1];
        else std::cout << "Invalid Input" << std::endl << std::endl;
    } while (deviceID == -1);
    delete[] deviceid;
    return(deviceID);
}
/***********************************************************************************/
/* End GetVideoDevice                                                              */
/***********************************************************************************/
/***********************************************************************************/
/* GetVideoResolution                                                              */
/***********************************************************************************/
static VideoResolution GetVideoResolution(void)
{
    VideoResolution vres = VideoResolution::rNone;
    KEY_EVENT_RECORD key;
    do
    {
        std::cout << "Select Video Resolution" << std::endl;
        std::cout << "1 - 640 x 480" << std::endl;
        std::cout << "2 - 1280 x 720" << std::endl;
        std::cout << "E - Exit" << std::endl;

        getconchar(key);
        std::cout << key.uChar.AsciiChar << std::endl;
        if ((key.uChar.AsciiChar == 'E') || (key.uChar.AsciiChar == 'e')) break;
        else if (key.uChar.AsciiChar == '1') vres = VideoResolution::r640X480;
        else if (key.uChar.AsciiChar == '2') vres = VideoResolution::r1280X720;
        else std::cout << "Invalid Input" << std::endl << std::endl;
    } while (vres == VideoResolution::rNone);
    
    return(vres);
}
/***********************************************************************************/
/* End GetVideoResolution                                                          */
/***********************************************************************************/
/***********************************************************************************/
/* GetVideoSaveMode                                                            */
/***********************************************************************************/
static VideoSaveMode GetVideoSaveMode(void)
{
    VideoSaveMode videosavemode = VideoSaveMode::vNone;
    KEY_EVENT_RECORD key;
    do
    {
        std::cout << "Select Video Save Mode" << std::endl;
        std::cout << "1 - No Save" << std::endl;
        std::cout << "2 - Save" << std::endl;
        std::cout << "3 - Save With No ALPR" << std::endl;
        std::cout << "E - Exit" << std::endl;

        getconchar(key);
        std::cout << key.uChar.AsciiChar << std::endl;
        if ((key.uChar.AsciiChar == 'E') || (key.uChar.AsciiChar == 'e')) exit(0);
        else if (key.uChar.AsciiChar == '1') videosavemode = VideoSaveMode::vNoSave;
        else if (key.uChar.AsciiChar == '2') videosavemode = VideoSaveMode::vSave;
        else if (key.uChar.AsciiChar == '3') videosavemode = VideoSaveMode::vSaveWithNoALPR;
        else std::cout << "Invalid Input" << std::endl << std::endl;
    } while (videosavemode == VideoSaveMode::vNone);

    return(videosavemode);
}
/***********************************************************************************/
/* End GetVideoSaveMode                                                            */
/***********************************************************************************/
/***********************************************************************************/
/* GetResponses                                                                    */
/***********************************************************************************/
static void GetResponses(void)
{
 ssize_t BytesRead;
 ssize_t BytesOnSocket = 0;
 while ((BytesOnSocket = BytesAvailableTcp(TcpConnectedPort)) > 0)
 {
     if (BytesOnSocket < 0) return;
     if (BytesOnSocket > BytesNeeded) BytesOnSocket = BytesNeeded;
     BytesRead = ReadDataTcp(TcpConnectedPort, (unsigned char*)& ResponseBuffer[BytesInResponseBuffer], BytesOnSocket);
     if (BytesRead <= 0)
     {
         printf("Read Response Error - Closing Socket\n");
         CloseTcpConnectedPort(&TcpConnectedPort);
     }
     BytesInResponseBuffer += BytesRead;

     if (BytesInResponseBuffer == BytesNeeded)
     {
         if (GetResponseMode == ResponseMode::ReadingHeader)
         {
             memcpy(&RespHdrNumBytes, ResponseBuffer, sizeof(RespHdrNumBytes));
             RespHdrNumBytes = ntohs(RespHdrNumBytes);
             GetResponseMode = ResponseMode::ReadingMsg;
             BytesNeeded = RespHdrNumBytes;
             BytesInResponseBuffer = 0;
         }
         else if (GetResponseMode == ResponseMode::ReadingMsg)
         {
             printf("Response %s\n", ResponseBuffer);
             GetResponseMode = ResponseMode::ReadingHeader;
             BytesInResponseBuffer = 0;
             BytesNeeded = sizeof(RespHdrNumBytes);
         }
     }
 }
 if (BytesOnSocket < 0)
 {
     printf("Read Response Error - Closing Socket\n");
     CloseTcpConnectedPort(&TcpConnectedPort);
 }
}
/***********************************************************************************/
/* End GetResponses                                                                */
/***********************************************************************************/


