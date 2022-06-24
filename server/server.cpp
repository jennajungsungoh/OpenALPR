// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include "NetworkTCP.h"
#include <Windows.h>
#include <db.h> 
#include "ConfigParser.h"
#include "logger.h"

//5 support multiple users using multi thread
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI Logging_info_perSec(LPVOID arg);
HANDLE ghMutex;
static int Query[6][3];//[Number of Client][0:logging disable or enable, 1: Port Number, 2: Quert Info]
static int Client_num;

//Minimum threshold from config (server.conf)
float MinThreshold;

const char code[12] = { 0x32, 0x54, 0x65, 0x61, 0x6d, 0x5f, 0x41, 0x68, 0x6e, 0x4c, 0x61, 0x62 };

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

    int i;



    // load config
    CConfigParser conf("server.conf");

    if (conf.IsSuccess())
    {
        MinThreshold = conf.GetFloat("MinThreshold");
    }
    else
    {
        MinThreshold = 80.0;   // default 80%
    }

    /*1초당 1번씩 Loggging 하는 thread 생성*/
    loggingThread = CreateThread(NULL, 0, Logging_info_perSec, 0, 0, &threadID);
    if (loggingThread == NULL)
    {
        printf("thread not created");
    }
    /*Mutex 생성*/
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

        if (Client_num < 5)
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

    float threshold =(float) (M*100)/N;

    // filtering by Minimum  Threshold
    if (threshold < MinThreshold) {
        return FALSE;
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
    TTcpConnectedPort* TcpConnectedPort = (TTcpConnectedPort*)arg;
    int Query_num = 0;
    int i;
    SSL* ssl = 0;;

     bool matchResult;  // Result of Partial Match 

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

        /*Query incremental*/
        WaitForSingleObject(ghMutex, INFINITE);
        Query[0][2]++; //Total Query
        Query[Query_num][2]++; // Client Query
        std::cout << "Total Query " << Query[0][2] << "    Port Number : " <<  Query[Query_num][1] << "   Number of QUERY : " << Query[Query_num][2] << "\n";
        ReleaseMutex(ghMutex);

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
                    if ((result = WriteDataTcp(ssl,TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
                        printf("WriteDataTcp %lld\n", result);
                    if ((result = WriteDataTcp(ssl, TcpConnectedPort, (unsigned char*)data.data, sendlength)) != sendlength)
                        printf("WriteDataTcp %lld\n", result);
                    
                    // debug
                    printf("[Partial Match]Org Plate [%s]\n", PlateString);
                    printf("Matched Plate ->%s\n", (char*)data.data);
                }
            }
        }
    }

    CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
    closeSSL(ssl);
    return 0;
}

DWORD WINAPI Logging_info_perSec(LPVOID arg)
{
    while (1)
    {
        Sleep(1000);
        /*logging 시작 부분*/
        //WaitForSingleObject(ghMutex, INFINITE);
        //Logging_Index(Query);
        //ReleaseMutex(ghMutex);
        //printf("logging \n");
    }
    return 0;
}