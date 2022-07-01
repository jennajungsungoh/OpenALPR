// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string.h>
#include <iostream>
#include "NetworkTCP.h"
#include <Windows.h>
#include <stdio.h>
#include <db.h> 
#include "ConfigParser.h"
#include "logger.h"

//5 support multiple users using multi thread
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI Logging_info_perSec(LPVOID arg);
DWORD WINAPI Configure_thread_process(LPVOID arg);
HANDLE ghMutex;
static int Query[100][3];//[Number of Client][0:logging disable or enable, 1: Port Number, 2: Quert Info]
static int Client_num;
bool logging_flag = FALSE;

//Minimum threshold from config (server.conf)
unsigned __int16 MaxClientNum;
float MinThreshold;

const char code[12] = { 0x32, 0x54, 0x65, 0x61, 0x6d, 0x5f, 0x41, 0x68, 0x6e, 0x4c, 0x61, 0x62 };

void Config_WriteInit(void)
{
    FILE* stream = NULL;
    errno_t num = fopen_s(&stream, "server.conf", "w");

    fprintf(stream, "%s%d\n", "MaxClientNum=", 5);
    fprintf(stream, "%s%f\n", "MinThreshold=", 80.0);

    fclose(stream);
}

int main()
{
    TTcpListenPort* TcpListenPort;
    TTcpConnectedPort* TcpConnectedPort;
    struct sockaddr_in cli_addr;
    socklen_t          clilen;
    bool NeedStringLength = true;

    HANDLE aThread[MAXCLIENT];
    HANDLE loggingThread;
    DWORD threadID;

    HANDLE configThread;
    DWORD configThreadID;
    unsigned short PlateStringLength;
    char PlateString[1024];
    int i;
    ssize_t result;
    DBT key, data;
    char sendStaticData[] = "LKY1360\nNo Wants / Warrants\n08 / 22 / 2023\nJennifer Green\n08 / 01 / 2001\n5938 Juan Throughway Apt. 948\nWest Corey, TX 43780\n1998\nMitsubishi\nForte5\nlime";

    // load config
    CConfigParser conf("server.conf");
    if (conf.IsSuccess())
    {
        MaxClientNum = conf.GetInt("MaxClientNum");
        MinThreshold = conf.GetFloat("MinThreshold");
    }
    else
    {
        Config_WriteInit();     // make config by default
        MaxClientNum = 5;       // default 5
        MinThreshold = 80.0;    // default 80%     
    }
    //printf("MaxClientNum is %d & MinThreshold is %f\n", MaxClientNum, MinThreshold);

    std::cout << "Attacker Server Start\n";
    //TCP 연결
    std::cout << "Listening\n";
    if ((TcpListenPort = OpenTcpListenPort(2222)) == NULL)  // Open UDP Network port
    {
        std::cout << "OpenTcpListenPortFailed\n";
        return(-1);
    }
    clilen = sizeof(cli_addr);

    if ((TcpConnectedPort = AcceptTcpConnection(TcpListenPort, &cli_addr, &clilen)) == NULL)
    {
        printf("AcceptTcpConnection Failed\n");
        return(-1);
    }

    printf("connected\n");

    SSL* ssl = 0;


     /*Openssl init 부분*/
    ssl = InitOpensslServer();
    if (ssl == 0)
    {
        printf("Failed init openssl\n");
        CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
        return 0;
    }


    while (1)
    {
        //데이터 수신부분
        if (ReadDataTcp(ssl, TcpConnectedPort, (unsigned char*)&PlateStringLength,
            sizeof(PlateStringLength)) != sizeof(PlateStringLength))
        {
            printf("exit and disconnected\n");
            break;
        }
        PlateStringLength = ntohs(PlateStringLength); //data ordering (little 

        printf("PlateStringLength : %d\n", PlateStringLength);

        if (PlateStringLength > sizeof(PlateString)) //plate stringdms 1024 
        {
            printf("Plate string length  error\n");
            break;
        }
        if (ReadDataTcp(ssl, TcpConnectedPort, (unsigned char*)&PlateString,
            PlateStringLength) != PlateStringLength)
        {
          
            printf("ReadDataTcp 2 error\n");
            break;
        }
        printf("Plate is : %s\n", PlateString);

       
        
        int sendlength = (int)(strlen((char*)sendStaticData) + 1);
        short SendMsgHdr = ntohs(sendlength);
        if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
            printf("WriteDataTcp %lld\n", result);
        if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)sendStaticData, sendlength)) != sendlength)
            printf("WriteDataTcp %lld\n", result);
        printf("sent ->%s\n", (char*)sendStaticData);

       
    }
    CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
    closeSSL(ssl);
    CloseTcpListenPort(&TcpListenPort);

    return 0;

}




