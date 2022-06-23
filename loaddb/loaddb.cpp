// loaddb.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <iostream>
#include <fstream>
#include <string>
#include <db.h> 

const char code[12] = { 0x32, 0x54, 0x65, 0x61, 0x6d, 0x5f, 0x41, 0x68, 0x6e, 0x4c, 0x61, 0x62 };

int main()
{
    std::ifstream file("datafile.txt");
    std::string str;
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

    while (std::getline(file, str, '$').good()) {
        /* Zero out the DBTs before using them. */
        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));
        std::string plate = str.substr(0, str.find("\n"));
        printf("plate is %s %d\n", plate.c_str(), strlen(plate.c_str()));
        key.data = (void *)plate.c_str();
        key.size = (u_int32_t)(plate.length()+1);
        data.data = (void*)str.c_str();
        data.size = (u_int32_t)str.length() + 1;
        std::cout << str << "\n";
        ret = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
        if (ret == DB_KEYEXIST) {
            dbp->err(dbp, ret,
                "Put failed because Plate already exists %s", plate.c_str());
        }

    }

    /* When we're done with the database, close it. */
    if (dbp != NULL)
        dbp->close(dbp, 0);
}
