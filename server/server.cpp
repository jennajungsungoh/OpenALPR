// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include "NetworkTCP.h"
#include <Windows.h>
#include <db.h> 



//4 support multiple users using multi thread
DWORD WINAPI ProcessClient(LPVOID arg);


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


DWORD WINAPI ProcessClient(LPVOID arg)
{
    DBT key, data;
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
        else printf("not Found\n");
    }
   
    CLOSE_SOCKET((*TcpConnectedPort).ConnectedFd);
    return 0;
}
