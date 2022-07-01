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

enum COMMAND {    // 열거형 정의
    NONE = 0,     // 초기값 할당
    MAX,         
    CONFIDENCE
};


void Config_WriteInit(void)
{
    FILE* stream = NULL;
    errno_t num = fopen_s(&stream, "server.conf", "w");

    if (num == 0)
    {
        fprintf(stream, "%s%d\n", "MaxClientNum=", 5);
        fprintf(stream, "%s%f\n", "MinThreshold=", 80.0);

        fclose(stream);
    }
    else
    { 
        printf("Open failed");
        return;
    }
}

void Update_ConfigFile(unsigned short clientNum,float threshold)
{
    FILE* stream = NULL;
    errno_t num = fopen_s(&stream, "server.conf", "w+");

    if (num == 0)
    {
        fprintf(stream, "%s%d\n", "MaxClientNum=", clientNum);
        fprintf(stream, "%s%f\n", "MinThreshold=", threshold);

        fclose(stream);
    }
    else
    {
        printf("Open failed");
        return;
    }
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

    int i;

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

    /*Start configure Thread*/
    configThread = CreateThread(NULL, 0, Configure_thread_process, 0, 0, &threadID);
    if (configThread == NULL)
    {
        printf("thread not created");
    }



    /*Start Logging Thread*/
    loggingThread = CreateThread(NULL, 0, Logging_info_perSec, 0, 0, &threadID);
    if (loggingThread == NULL)
    {
        printf("thread not created");
    }
    /*start Logging*/
    logging_start();
    /*Mutex*/
    ghMutex = CreateMutex(NULL, FALSE, NULL);
    if (ghMutex == NULL)
    {
        printf("logging thread not created");
    }
    /* TCP/IP 부분. */

    //TCP 연결
    std::cout << "Listening\n";
    if ((TcpListenPort = OpenTcpListenPort(2222)) == NULL)  // Open UDP Network port
    {
        std::cout << "OpenTcpListenPortFailed\n";
        return(-1);
    }
    clilen = sizeof(cli_addr);

    while (1)
    {

        if ((TcpConnectedPort = AcceptTcpConnection(TcpListenPort, &cli_addr, &clilen)) == NULL)
        {
            printf("AcceptTcpConnection Failed\n");
            return(-1);
        }

        if (Client_num < MaxClientNum)
        {
            printf("connected\n");

            // 스레드 생성 2 
            aThread[Client_num] = CreateThread(NULL, 0, ProcessClient, (LPVOID)TcpConnectedPort, 0, &threadID);
            if (aThread[Client_num] == NULL)
            {
                printf("thread not created");
            }
            else
            {
                //Maximum 5 client can connect
                Client_num++;
            }
        }
        else
        {
            std::cout << "can not connect lookup server " << "\n";

            //CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
        }
    }

    //Wait for all threads to terminate
    WaitForMultipleObjects(MAXCLIENT, aThread, TRUE, INFINITE);

    // close thread and mutex handles
    for (i = 0; i < MAXCLIENT; i++)
        CloseHandle(aThread[i]);
    CloseHandle(ghMutex);

    //Close TCP listen Port
    CloseTcpListenPort(&TcpListenPort);
    int closeCTX();
}


// Fills lps[] for given patttern pat[0..M-1]
void computeLPS(char* pat, int M, int* lps)
{
    // length of the previous longest prefix suffix
    int len = 0;

    lps[0] = 0; // lps[0] is always 0

    // the loop calculates lps[i] for i = 1 to M-1
    int i = 1;
    while (i < M) {
        if (pat[i] == pat[len]) {
            len++;
            lps[i] = len;
            i++;
        }
        else // (pat[i] != pat[len])
        {
            // This is tricky. Consider the example.
            // AAACAAAA and i = 7. The idea is similar
            // to search step.
            if (len != 0) {
                len = lps[len - 1];

                // Also, note that we do not increment
                // i here
            }
            else // if (len == 0)
            {
                lps[i] = 0;
                i++;
            }
        }
    }
}

// Check Partial Match using KMP (Knuth Morris Pratt) pattern searching algorithm
bool checkPartialMatch(char* pat, char* txt)
{
    int M = strlen(pat);
    int N = strlen(txt);

    float threshold = (float)(M * 100) / N;

    // filtering by Minimum  Threshold
    if (threshold < MinThreshold) {
        return FALSE;
    }
    else if (MinThreshold == 0)
    {
        return TRUE;
    }

    // create lps[] that will hold the longest prefix suffix
    // values for pattern
    int lps[24];

    // Preprocess the pattern (calculate lps[] array)
    computeLPS(pat, M, lps);

    int i = 0; // index for txt[]
    int j = 0; // index for pat[]
    while (i < N) {
        if (pat[j] == txt[i]) {
            j++;
            i++;
        }

        if (j == M) {
            printf("Found pattern at index %d ", i - j);
            j = lps[j - 1];

            return TRUE;
        }

        // mismatch after j matches
        else if (i < N && pat[j] != txt[i]) {
            // Do not match lps[0..lps[j-1]] characters,
            // they will match anyway
            if (j != 0)
                j = lps[j - 1];
            else
                i = i + 1;
        }
    }

    return FALSE;
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
    DBT key, data;
    DBC* dbcp;
    DB* dbp; /* DB structure handle */
    u_int32_t flags; /* database open flags */
    int ret; /* function return value */
    ssize_t result;
    unsigned short PlateStringLength;
    char PlateString[1024];
    char DBRecord[2048];
    char logging_info[1024];
    TTcpConnectedPort* TcpConnectedPort = (TTcpConnectedPort*)arg;
    int Query_num = 0;
    int i;
    SSL* ssl = 0;;

    bool matchResult = false;  // Result of Partial Match 

     /*Openssl init 부분*/
    ssl = InitOpensslServer();
    if (ssl == 0)
    {
        printf("Failed init openssl\n");
        CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
        return 0;
    }

    /*DB 부분. */

  /* Initialize the structure. This
   * database is not opened in an environment,
   * so the environment pointer is NULL. */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) {
        /* Error handling goes here */
        printf("DB Create Error\n");
        return -1;
    }

    /* Database encrypt flags */
    // get encrypt flag
    ret = dbp->get_encrypt_flags(dbp, &flags);
    if (ret != 0) {
        /* Error handling goes here */
        printf("DB Encrypt Error\n");
        return -1;
    }

    flags = DB_ENCRYPT_AES;
    ret = dbp->set_encrypt(dbp, code, flags);
    if (ret != 0) {
        /* Error handling goes here */
        printf("DB Encrypt Error\n");
        return -1;
    }

    /* Database open flags */
    flags = DB_CREATE; /* If the database does not exist,
     * create it.*/
     /* open the database */
    ret = dbp->open(dbp, /* DB structure pointer */
        NULL, /* Transaction pointer */
        "licenseplate.db", /* On-disk file that holds the database. */
        NULL, /* Optional logical database name */
        DB_HASH, /* Database access method */
        flags, /* Open flags */
        0); /* File mode (using defaults) */
    if (ret != 0) {
        /* Error handling goes here */
        printf("DB Open Error\n");
        return -1;
    }
    else
    {
        printf("DB Open Success\n");
    }

    /*add query Port ID*/
    WaitForSingleObject(ghMutex, INFINITE);
    for (i = 1; i < 6; i++)
    {
        if (Query[i][0] != VALID_LOGGER)
        {
            Query[i][0] = VALID_LOGGER;
            Query[i][1] = (int)TcpConnectedPort->ConnectedFd;
            Query_num = i;
            break;
        }
    }
    //std::cout << "QUERY start at : " << Query_num <<"\n";
    ReleaseMutex(ghMutex);

    while (1)
    {
        //데이터 수신부분
        if (ReadDataTcp(ssl, TcpConnectedPort, (unsigned char*)&PlateStringLength,
            sizeof(PlateStringLength)) != sizeof(PlateStringLength))
        {
            printf("exit and disconnected\n");

            WaitForSingleObject(ghMutex, INFINITE);
            Query[Query_num][0] = 0;
            Query[Query_num][1] = 0;
            Query[Query_num][2] = 0;
            Client_num--;
            //std::cout << "Delate Query" << Query[1][0] << ", " << Query[2][0] << ", " << Query[3][0] << ", " << Query[4][0] << ", " << Query[5][0]  << "\n";
            ReleaseMutex(ghMutex);
            break;
        }
        PlateStringLength = ntohs(PlateStringLength); //data ordering (little 

        printf("PlateStringLength : %d\n", PlateStringLength);

        if (PlateStringLength > sizeof(PlateString)) //plate stringdms 1024 
        {
            WaitForSingleObject(ghMutex, INFINITE);
            Query[Query_num][0] = 0;
            Query[Query_num][1] = 0;
            Query[Query_num][2] = 0;
            Client_num--;
            //std::cout << "Delate Query" << Query[1][0] << ", " << Query[2][0] << ", " << Query[3][0] << ", " << Query[4][0] << ", " << Query[5][0]  << "\n";
            ReleaseMutex(ghMutex);
            printf("Plate string length  error\n");
            break;
        }
        if (ReadDataTcp(ssl, TcpConnectedPort, (unsigned char*)&PlateString,
            PlateStringLength) != PlateStringLength)
        {
            WaitForSingleObject(ghMutex, INFINITE);
            Query[Query_num][0] = 0;
            Query[Query_num][1] = 0;
            Query[Query_num][2] = 0;
            Client_num--;
            //std::cout << "Delate Query" << Query[1][0] << ", " << Query[2][0] << ", " << Query[3][0] << ", " << Query[4][0] << ", " << Query[5][0]  << "\n";
            ReleaseMutex(ghMutex);
            printf("ReadDataTcp 2 error\n");
            break;
        }
        printf("Plate is : %s\n", PlateString);
        if (strcmp(PlateString, "LTM378") == 0)
        {
            logging_flag = TRUE;
        }
        printf("logging_flag is : %d\n", logging_flag);
        /*Query incremental*/
        WaitForSingleObject(ghMutex, INFINITE);

        Query[0][2]++; //Total Query
        Query[Query_num][2]++; // Client Query
        /*logging informaiton*/
        sprintf_s(logging_info, 1024, " Port Number (%d)  requested plate (%s)  ", Query[Query_num][1], PlateString);
        Logging_Index(logging_info);
        ReleaseMutex(ghMutex);
        //std::cout << "Total Query " << Query[0][2] << "    Port Number : " <<  Query[Query_num][1] << "   Number of QUERY : " << Query[Query_num][2] << "\n";
        /* Zero out the DBTs before using them. */
                /* Zero out the DBTs before using them. */
        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));
        key.data = PlateString;
        key.size = (u_int32_t)(strlen(PlateString) + 1);
        data.data = DBRecord;
        data.ulen = sizeof(DBRecord);
        data.flags = DB_DBT_USERMEM;
        if (dbp->get(dbp, NULL, &key, &data, 0) != DB_NOTFOUND)
        {
            int sendlength = (int)(strlen((char*)data.data) + 1);
            short SendMsgHdr = ntohs(sendlength);
            if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
                printf("WriteDataTcp %lld\n", result);
            if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)data.data, sendlength)) != sendlength)
                printf("WriteDataTcp %lld\n", result);
            printf("sent ->%s\n", (char*)data.data);
            if (logging_flag == TRUE)
            {
                sprintf_s(logging_info, 1024, " Port Number (%d)  Personal information (%s)  ", Query[Query_num][1], data.data);
                Logging_Index(logging_info);
            }
        }
        else
        {
            /* Acquire a cursor for the database. */
            if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
                /* Error handling goes here */
                printf("DB Cursor Error\n");
                return -1;
            }

            memset(&key, 0, sizeof(DBT));
            memset(&data, 0, sizeof(DBT));

            while ((ret = dbcp->get(dbcp, &key, &data, DB_NEXT)) == 0)
            {
                if (PlateStringLength <= (int)key.size)
                {
                    matchResult = checkPartialMatch(PlateString, (char*)key.data);
                }
                else
                {
                    matchResult = checkPartialMatch((char*)key.data, PlateString);
                }

                if (matchResult)
                {

                    int sendlength = (int)(strlen((char*)data.data) + 1);
                    short SendMsgHdr = ntohs(sendlength);
                    if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
                        printf("WriteDataTcp %lld\n", result);
                    if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)data.data, sendlength)) != sendlength)
                        printf("WriteDataTcp %lld\n", result);

                    // debug
                    printf("[Partial Match]Org Plate [%s]\n", PlateString);
                    printf("Matched Plate ->%s\n", (char*)data.data);
                    if (logging_flag == TRUE)
                    {
                        sprintf_s(logging_info, 1024, " Port Number (%d)  Personal information (%s)  ", Query[Query_num][1], data.data);
                        Logging_Index(logging_info);
                    }
                }
            }

            printf("matchReuslt : %d\n", matchResult);

            if (matchResult == false)
            {
                int sendlength = 0;
                short SendMsgHdr = ntohs(sendlength);
                if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
                    printf("WriteDataTcp %lld\n", result);
                printf("No Matched Plate\n");

            }
        }
    }

    CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
    closeSSL(ssl);
    return 0;
}

DWORD WINAPI Logging_info_perSec(LPVOID arg)
{
    int i;
    int j;
    char test[1024];
    while (1)
    {
        j = 0;
        ::memset(test, 0, 100);
        Sleep(1000);
        /*logging 시작 부분*/
        WaitForSingleObject(ghMutex, INFINITE);
        //print logging
        if (Client_num > 0)
        {

            j = sprintf_s(test, 1024, "Total Query = %d", Query[0][2]);
            //std::cout << "Total Query " << Query[0][2] <<"   client number  : " << Client_num << "\n";

            Query[0][2] = 0;
            for (i = 1; i < MAXCLIENT; i++)
            {
                if (Query[i][0] == VALID_LOGGER)
                {
                    j += sprintf_s(test + j, 1024 - j, " Port Number : %d , Query Per Second : %d ", Query[i][1], Query[i][2]);
                    //std::cout << "Port Number : " << Query[i][1] << "  Query Per Second :  " << Query[i][2] << "  ";
                    Query[i][2] = 0;
                }
            }
            //std::cout << "\n";
            Logging_Index(test);

        }
        ReleaseMutex(ghMutex);
    }
    return 0;
}


DWORD WINAPI Configure_thread_process(LPVOID arg)
{
    TTcpListenPort* TcpListenPortConfig;
    TTcpConnectedPort* TcpConnectedPortConfig;
    struct sockaddr_in cli_addr_config;
    socklen_t          clilen_config;
    SSL* ssl = 0;
    char dataRecv[1024];
    unsigned short dataRecvLen;
    unsigned int dataValue;
    int command = NONE;
    int i = 0;
    unsigned short maxUser = 0;
    float confidenceLevel = 0;

    //TCP 연결
    std::cout << "Listening configure Port\n";
    if ((TcpListenPortConfig = OpenTcpListenPort(3333)) == NULL)  // Open UDP Network port
    {
        std::cout << "Config OpenTcpListenPortFailed\n";
        return(-1);
    }
    clilen_config = sizeof(cli_addr_config);

    while (true)
    {

        if ((TcpConnectedPortConfig = AcceptTcpConnection(TcpListenPortConfig, &cli_addr_config, &clilen_config)) == NULL)
        {
            printf("Config AcceptTcpConnection Failed\n");
            return(-1);
        }


        /*Openssl init 부분*/
        ssl = InitOpensslServer();
        if (ssl == 0)
        {
            printf("Failed init openssl\n");
            CLOSE_SOCKET(TcpConnectedPortConfig->ConnectedFd);
            return 0;
        }


        for (i = 0; i < 2; i++)
        {
            dataRecvLen = ReadDataTcp(ssl, TcpConnectedPortConfig, (unsigned char*)&dataRecv, sizeof(dataRecv));

            printf("command : %s\n", dataRecv);

            if (0 == strcmp(dataRecv, "max"))
            {
                command = MAX;
                // do something for max
            }
            else if (0 == strcmp(dataRecv, "confidence"))
            {
                command = CONFIDENCE;
                //do something for confidence
            }

            //Receive Data
            dataRecvLen = ReadDataTcp(ssl, TcpConnectedPortConfig, (unsigned char*)&dataValue, sizeof(dataValue));
            printf("1Data : %d\n", dataValue);
            dataValue = dataValue >> 8;
            dataValue = ntohs(dataValue);

            printf("2Data : %d\n", dataValue);

            // Receive configuration values
            if (command == MAX)
            {
                maxUser = dataValue;
            }
            else if (command == CONFIDENCE)
            {
                confidenceLevel = (float)dataValue;
            }
        }

        //update configuration variables and config file.
        if ((maxUser != MaxClientNum) || (confidenceLevel != MinThreshold))
        {
            MaxClientNum = maxUser;
            MinThreshold = confidenceLevel;
            Update_ConfigFile(maxUser, confidenceLevel);
        }

        memset(dataRecv, 0, sizeof(dataRecv));
        dataValue = 0;
        dataRecvLen = 0;

        closeSSL(ssl);
        CLOSE_SOCKET(TcpConnectedPortConfig->ConnectedFd);

        TcpConnectedPortConfig = 0;
    }

    return 0;
}