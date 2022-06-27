//------------------------------------------------------------------------------------------------
// File: NetworkTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive TCP byte streams for both Window platforms
//------------------------------------------------------------------------------------------------
#include <iostream>
#include <new>
#include <stdio.h>
#include <string.h>
#include "NetworkTCP.h"
WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;
addrinfo* result = NULL;
addrinfo hints;
int client = 0;
SOCKET sClient;
SSL_CTX* ctx;
SOCKET_FD_TYPE ConnectedFd_ssl;
const char* KeyFile = { "serverKey.pem" };
const char* CertFile = { "serverCrt.pem" };
//-----------------------------------------------------------------
// OpenTCPListenPort - Creates a Listen TCP port to accept
// connection requests
//-----------------------------------------------------------------
TTcpListenPort *OpenTcpListenPort(short localport)
{
  TTcpListenPort *TcpListenPort;
  struct sockaddr_in myaddr;

  TcpListenPort= new (std::nothrow) TTcpListenPort;  
  
  if (TcpListenPort==NULL)
     {
      fprintf(stderr, "TUdpPort memory allocation failed\n");
      return(NULL);
     }
  TcpListenPort->ListenFd=BAD_SOCKET_FD;

  WSADATA wsaData;
  int     iResult;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) 
    {
     delete TcpListenPort;
     printf("WSAStartup failed: %d\n", iResult);
     return(NULL);
    }

  // create a socket
  if ((TcpListenPort->ListenFd= socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
     {
      CloseTcpListenPort(&TcpListenPort);
      perror("socket failed");
      return(NULL);  
     }
  int option = 1; 

   if(setsockopt(TcpListenPort->ListenFd,SOL_SOCKET,SO_REUSEADDR,(char*)&option,sizeof(option)) < 0)
     {
      CloseTcpListenPort(&TcpListenPort);
      perror("setsockopt failed");
      return(NULL);
     }

  // bind it to all local addresses and pick any port number
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(localport);

  if (bind(TcpListenPort->ListenFd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
      CloseTcpListenPort(&TcpListenPort);
      perror("bind failed");
      return(NULL); 
    }
   
 
  if (listen(TcpListenPort->ListenFd,5)< 0)
  {
      CloseTcpListenPort(&TcpListenPort);
      perror("bind failed");
      return(NULL);	  
  }
  return(TcpListenPort);
}
//-----------------------------------------------------------------
// END OpenTCPListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseTcpListenPort - Closes the specified TCP listen port
//-----------------------------------------------------------------
void CloseTcpListenPort(TTcpListenPort **TcpListenPort)
{
  if ((*TcpListenPort)==NULL) return;
  if ((*TcpListenPort)->ListenFd!=BAD_SOCKET_FD)  
     {
      CLOSE_SOCKET((*TcpListenPort)->ListenFd);
      (*TcpListenPort)->ListenFd=BAD_SOCKET_FD;
     }
   delete (*TcpListenPort);
  (*TcpListenPort)=NULL;

   WSACleanup();

}
//-----------------------------------------------------------------
// END CloseTcpListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// AcceptTcpConnection -Accepts a TCP Connection request from a 
// Listening port
//-----------------------------------------------------------------
TTcpConnectedPort *AcceptTcpConnection(TTcpListenPort *TcpListenPort, 
                       struct sockaddr_in *cli_addr,socklen_t *clilen)
{
  TTcpConnectedPort *TcpConnectedPort;

  TcpConnectedPort= new (std::nothrow) TTcpConnectedPort;  
  
  if (TcpConnectedPort==NULL)
     {
      fprintf(stderr, "TUdpPort memory allocation failed\n");
      return(NULL);
     }
  TcpConnectedPort->ConnectedFd= accept(TcpListenPort->ListenFd,
                      (struct sockaddr *) cli_addr,clilen);
					  
  if (TcpConnectedPort->ConnectedFd== BAD_SOCKET_FD) 
  {
	perror("ERROR on accept");
	delete TcpConnectedPort;
	return NULL;
  }
  
 int bufsize = 200 * 1024;
 if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, 
                 SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(&TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(NULL);
	}
 if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, 
                 SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(&TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(NULL);
	}

 client = TcpConnectedPort->ConnectedFd;
 ConnectedFd_ssl = TcpConnectedPort->ConnectedFd;


 return TcpConnectedPort;
}
//-----------------------------------------------------------------
// END AcceptTcpConnection
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// OpenTCPConnection - Creates a TCP Connection to a TCP port
// accepting connection requests
//-----------------------------------------------------------------
TTcpConnectedPort *OpenTcpConnection(const char *remotehostname, const char * remoteportno)
{
  TTcpConnectedPort *TcpConnectedPort;
  int                s;
  struct addrinfo   hints;
  struct addrinfo   *result = NULL;

  TcpConnectedPort= new (std::nothrow) TTcpConnectedPort;  
  
  if (TcpConnectedPort==NULL)
     {
      fprintf(stderr, "TUdpPort memory allocation failed\n");
      return(NULL);
     }
  TcpConnectedPort->ConnectedFd=BAD_SOCKET_FD;

  WSADATA wsaData;
  int     iResult;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) 
    {
     delete TcpConnectedPort;
     printf("WSAStartup failed: %d\n", iResult);
     return(NULL);
    }

  // create a socket
   memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  
  s = getaddrinfo(remotehostname, remoteportno, &hints, &result);
  if (s != 0) 
    {
	  delete TcpConnectedPort;
      fprintf(stderr, "getaddrinfo: Failed\n");
      return(NULL);
    }
  if ( result==NULL)
    {
	  delete TcpConnectedPort;
      fprintf(stderr, "getaddrinfo: Failed\n");
      return(NULL);
    }
  if ((TcpConnectedPort->ConnectedFd= socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
     {
      CloseTcpConnectedPort(&TcpConnectedPort);
	  freeaddrinfo(result);
      perror("socket failed");
      return(NULL);  
     }

  int bufsize = 200 * 1024;
  if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET,
                 SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(&TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(NULL);
	}
  if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, 
                 SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(&TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(NULL);
	}
	 
  if (connect(TcpConnectedPort->ConnectedFd,result->ai_addr,(int)result->ai_addrlen) < 0) 
          {
	    CloseTcpConnectedPort(&TcpConnectedPort);
	    freeaddrinfo(result);
            perror("connect failed");
            return(NULL);
	  }
  freeaddrinfo(result);	 
  return(TcpConnectedPort);
}
//-----------------------------------------------------------------
// END OpenTcpConnection
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseTcpConnectedPort - Closes the specified TCP connected port
//-----------------------------------------------------------------
void CloseTcpConnectedPort(TTcpConnectedPort **TcpConnectedPort)
{
  if ((*TcpConnectedPort)==NULL) return;
  if ((*TcpConnectedPort)->ConnectedFd!=BAD_SOCKET_FD)  
     {
      CLOSE_SOCKET((*TcpConnectedPort)->ConnectedFd);
      (*TcpConnectedPort)->ConnectedFd=BAD_SOCKET_FD;
     }
   delete (*TcpConnectedPort);
  (*TcpConnectedPort)=NULL;

   WSACleanup();

}
//-----------------------------------------------------------------
// END CloseTcpListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// BytesAvailableTcp - Reads the bytes available 
//-----------------------------------------------------------------
ssize_t BytesAvailableTcp(TTcpConnectedPort* TcpConnectedPort)
{
    unsigned long n = -1;;
    
    if (ioctlsocket(TcpConnectedPort->ConnectedFd, FIONREAD, &n) <0)
    {
        printf("BytesAvailableTcp: error %d\n", WSAGetLastError());
        return -1;
    }
  
    return((ssize_t)n);
}
//-----------------------------------------------------------------
// END BytesAvailableTcp 
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// ReadDataTcp - Reads the specified amount TCP data 
//-----------------------------------------------------------------
ssize_t ReadDataTcp(SSL* ssl, TTcpConnectedPort *TcpConnectedPort,unsigned char *data, size_t length)
{
 ssize_t bytes;
 int err;

#if 0
 for (size_t i = 0; i < length; i += bytes)
    {
      if ((bytes = recv(TcpConnectedPort->ConnectedFd, (char *)(data+i), (int)(length  - i),0)) == -1) 
      {
       return (-1);
      }
      
    }
#else
  err = SSL_read(ssl, data, length);
#endif 


  return(length);
}
//-----------------------------------------------------------------
// END ReadDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// WriteDataTcp - Writes the specified amount TCP data 
//-----------------------------------------------------------------
ssize_t WriteDataTcp(SSL* ssl, TTcpConnectedPort *TcpConnectedPort,unsigned char *data, size_t length)
{
  ssize_t total_bytes_written = 0;
  ssize_t bytes_written;
  int err;

#if 0 
  while (total_bytes_written != length)
    {

      
     bytes_written = send(TcpConnectedPort->ConnectedFd,
	                               (char *)(data+total_bytes_written),
                                  (int)(length - total_bytes_written),0);
     if (bytes_written == -1)
       {
       return(-1);
      }
     total_bytes_written += bytes_written;
   }
   return(total_bytes_written);
#else
  err = SSL_write(ssl, data, length);
  return(length);
#endif 

}
//-----------------------------------------------------------------
// END WriteDataTcp
//-----------------------------------------------------------------
/*---------------------------------------------------------------------*/
/*--- InitOpensslServer - initialize SSL server  and create context ---*/
/*---------------------------------------------------------------------*/
SSL* InitOpensslServer()
{
    SSL* ssl;
    ctx = InitServerCTX();/* initialize SSL */
    LoadCertificates(ctx, KeyFile, CertFile); /* load certs */

    ssl = SSL_new(ctx);
    /* get new SSL state with context */
    SSL_set_fd(ssl, ConnectedFd_ssl);

    if (SSL_accept(ssl) == FAIL)
    {
        SSL_free(ssl);                                    /* release SSL state */
        SSL_CTX_free(ctx);                                /* release context */
        return 0;
    }


    /* Check for Client authentication error */
#ifdef ClientCertVerify
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        printf("SSL Client Authentication error\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        exit(0);
    }
#endif 
    /*Print out connection details*/
    printf("SSL connection on socket %x,Version: %s, Cipher: %s\n",
        ConnectedFd_ssl,
        SSL_get_version(ssl),
        SSL_get_cipher(ssl));

    printf("SSL connected\n");

    return ssl;
}



/*---------------------------------------------------------------------*/
/*--- InitServerCTX - initialize SSL server  and create context     ---*/
/*---------------------------------------------------------------------*/
SSL_CTX* InitServerCTX(void) {
    char errorStr[10] = "ctx error";
    const SSL_METHOD* method;
    SSL_CTX* ctx;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    /* load & register all cryptos, etc. */
    SSL_load_error_strings();
    /* load all error messages */
    method = TLS_server_method();
    /* create new server-method instance */
    ctx = SSL_CTX_new(method);
    /* create new context from method */
    if (ctx == NULL) {
        ErrorHandling(errorStr);
    }

    /*Set the Cipher List*/
    if (SSL_CTX_set_cipher_list(ctx, CIPHER_LIST) <= 0) {
        printf("Error setting the cipher list.\n");
        exit(0);
    }


    return ctx;
}
/*---------------------------------------------------------------------*/
/*--- InitCTX - initialize the SSL engine.                          ---*/
/*---------------------------------------------------------------------*/
SSL_CTX* InitCTX(void) 
{ 
    char errorStr[10] = "ctx error";
    const SSL_METHOD * method;
    SSL_CTX * ctx;
    SSL_library_init(); 
    OpenSSL_add_all_algorithms();
    /* Load cryptos, et.al. */  
    SSL_load_error_strings(); 
    /* Bring in and register error messages */
    method  = TLS_client_method();
    /* Create new client-method instance */ 
    ctx  = SSL_CTX_new(method);    
    /* Create new context */  
    if (ctx  == NULL) 
    { 
        ErrorHandling(errorStr);
    }    
    return ctx; 
}

/*---------------------------------------------------------------------*/
/*--- LoadCertificates - load from files.                           ---*/
/*---------------------------------------------------------------------*/
void LoadCertificates(SSL_CTX* ctx, const char* KeyFile, const char* CertFile)
{
    char errorStr1[37] = "SSL_CTX_use_certificate_file() error";
    char errorStr2[36] = "SSL_CTX_use_PrivateKey_file() error";
    char errorStr3[34] = "SSL_CTX_check_private_key() error";

    /* set the local certificate from CertFile */
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ErrorHandling(errorStr1);
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ErrorHandling(errorStr2);
    }
    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
        ErrorHandling(errorStr3);
    }
}

void ErrorHandling(char* message) {
    puts(message);
    exit(1);
}

/*---------------------------------------------------------------------*/
/*--- closeSSL - closeSSL                           ---*/
/*---------------------------------------------------------------------*/
int closeSSL(SSL* ssl)
{
    SSL_free(ssl);
    //SSL_CTX_free(ctx);

    return 0;
}

int closeCTX()
{
    SSL_CTX_free(ctx);

    return 0;
}

//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------


