#ifndef APPLICATIONLAYER_SEEN
#define APPLICATIONLAYER_SEEN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linklayer.h"

struct applicationLayer {
int role;                       /*TRANSMITTER | RECEIVER*/
FILE *fileDescriptor;           /*Descritor of the File*/
char *fileName;                 /*Name of the File*/
long int sizeOfFile;            /*Size of the File*/ 
int maxDataSize;                /*Max Size of Packages*/
long int sizeDataProcessed;     /*Size of Data Processed so far*/
};

//Return Values
#define OK  0
#define ERROR 1

//MACHINE_ROLE
#define NOT_DEFINED -1
#define EMITOR 0
#define RECEIVER 1

//Control Field Macros
#define DATA 1
#define START 2
#define END 3

//Type of Parameter
#define FILE_NAME 0
#define FILE_SIZE 1

int applicationLayerMain(char* serialPort,  int role, char *fileName, int maxDataSize, int baudRate, int numTries, int timeOut);

int configApplication(char* serialPort, int role, char * fileName, int maxDataSize);

int openFile();

//Package Handlers
int sendControlPackage(int type);

int sendDataPackage(int N, int size, char* data);

int receiveControlPackage();

int receiveDataPackage(int* N, char** buf, int* length);

//Emitor Section
long int calculateFileSize();

int sendFile();

//Receiver Section
int receiveFile();

#endif
