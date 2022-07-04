#ifndef LOGGER_H
#define LOGGER_H


#define  VALID_LOGGER 1
#define  MAXCLIENT 5 // add for logging

const char test[6] = {0x6c, 0x54, 0x4D, 0x33, 0x37};

bool logging_start();
int Logging_Index(char* data);



#endif /* LOG_H */