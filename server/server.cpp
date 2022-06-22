// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include "NetworkTCP.h"
#include <Windows.h>
#include <db.h> 
#include "ConfigParser.h"

//4 support multiple users using multi thread
DWORD WINAPI ProcessClient(LPVOID arg);

//Minimum threshold from config (server.conf)
float MinThreshold;

int main()
{
    TTcpListenPort* TcpListenPort;
    TTcpConnectedPort* TcpConnectedPort;
    struct sockaddr_in cli_addr;
    socklen_t          clilen;
    bool NeedStringLength = true;
    unsigned short PlateStringLength;
    char PlateString[1024];
    char DBRecord[2048];
    DBT key, data;
    DB* dbp; /* DB structure handle */
    u_int32_t flags; /* database open flags */
    int ret; /* function return value */
    ssize_t result;

    DWORD threadID;

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
        printf("connected\n");

        // 스레드 생성 2 
        HANDLE hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)TcpConnectedPort, 0, &threadID);
        if (hThread == NULL)
        {
            printf("thread not created");
        }

    }    

    CloseTcpListenPort(&TcpListenPort);
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
    TTcpConnectedPort* TcpConnectedPort;
    // 전달된 소켓 3 
    TcpConnectedPort = (TTcpConnectedPort*)arg;

     bool matchResult;  // Result of Partial Match 

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

    while (1)
    {
        //데이터 수신부분
        if (ReadDataTcp(TcpConnectedPort, (unsigned char*)&PlateStringLength,
            sizeof(PlateStringLength)) != sizeof(PlateStringLength))
        {
            printf("exit and disconnected\n");
            break;
        }
        PlateStringLength = ntohs(PlateStringLength); //data ordering (little 

        if (PlateStringLength > sizeof(PlateString)) //plate stringdms 1024 
        {
            printf("Plate string length  error\n");
            break;
        }
        if (ReadDataTcp(TcpConnectedPort, (unsigned char*)&PlateString,
            PlateStringLength) != PlateStringLength)
        {
            printf("ReadDataTcp 2 error\n");
            break;
        }
        printf("Plate is : %s\n", PlateString);


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
            if ((result = WriteDataTcp(TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
                printf("WriteDataTcp %lld\n", result);
            if ((result = WriteDataTcp(TcpConnectedPort, (unsigned char*)data.data, sendlength)) != sendlength)
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
                    if ((result = WriteDataTcp(TcpConnectedPort, (unsigned char*)&SendMsgHdr, sizeof(SendMsgHdr))) != sizeof(SendMsgHdr))
                        printf("WriteDataTcp %lld\n", result);
                    if ((result = WriteDataTcp(TcpConnectedPort, (unsigned char*)data.data, sendlength)) != sendlength)
                        printf("WriteDataTcp %lld\n", result);
                    
                    // debug
                    printf("[Partial Match]Org Plate [%s]\n", PlateString);
                    printf("Matched Plate ->%s\n", (char*)data.data);
                }
            }
        }
    }
   
    CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
    return 0;
}
