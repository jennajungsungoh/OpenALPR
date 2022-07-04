// decrypDB.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <fstream>
#include <string>
#include <db.h> 

int main(int argc, char* argv[]) 
{
    int idx=0;

    char passwd[20];
    char fileName[20];

    strcpy_s(passwd, argv[1]);
    strcpy_s(fileName, argv[2]); 
  
    printf("Password:%s\n", passwd);
    printf("FileName:%s\n", fileName);


    FILE* stream = NULL;
    errno_t num = fopen_s(&stream, "decrypt.txt", "w");

    DBC* dbcp;
    DBT key, data;
    DB* dbp; /* DB structure handle */
    u_int32_t flags; /* database open flags */
    int ret; /* function return value */
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
    flags = DB_ENCRYPT_AES;
    ret = dbp->set_encrypt(dbp, passwd, flags);
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
        fileName, /* On-disk file that holds the database. */
        NULL, /* Optional logical database name */
        DB_HASH, /* Database access method */
        flags, /* Open flags */
        0); /* File mode (using defaults) */
    if (ret != 0) {
        /* Error handling goes here */
        printf("DB Open Error\n");
        return -1;
    }

    /* Acquire a cursor for the database. */
    if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
        /* Error handling goes here */
        printf("DB Cursor Error\n");
        return -1;
    }

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    printf("######  Decrypt DB ######\n");
    while ((ret = dbcp->get(dbcp, &key, &data, DB_NEXT)) == 0)
    {
        idx++;
        // debug
        printf("plate[%d] : %s \n",idx, (char *)key.data);
        printf("%s\n", (char*)data.data);

        fprintf(stream, "plate[%d] : %s \n",idx, (char*)key.data);
        fprintf(stream, "%s\n",(char*)data.data);
    }

    /* When we're done with the database, close it. */
    if (dbp != NULL)
        dbp->close(dbp, 0);

    fclose(stream);
}
