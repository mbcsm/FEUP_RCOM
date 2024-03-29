#ifndef LINKLAYER_SEEN
#define LINKLAYER_SEEN

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct linkLayer{
    char* serialPort;
    int serialPortDescriber;
    int role;
    int baudRate;
    int numTries;
    int timeOut;
    int N;
};

//MACHINE_ROLE
#define NOT_DEFINED -1
#define EMITOR 0
#define RECEIVER 1

//CONTROL WORDS
#define SET 0
#define DISC 1
#define UA 2
#define RR 3
#define REJ 4

//CONTROL BYTES
#define FLAG 0x7e
#define N_EQUALS_1 0x40

//BYTE STUFFING
#define ESCAPE 0x7d
#define STUFFING 0x20

//SIZES
#define DATA_PACKET_SIZE 6

//PORT
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//MISC.
#define FALSE 0
#define TRUE 1


/* port control functions*/
int configConection(char * serialPort, int role, int baudRate, int numTries, int timeOut);


int freeMemoryLinkLayer();


 
//Set the new termios structure on "port" and open it on "fd"
//return "1" on error and "0" on sucesses
int llsetNewtio();

//Reset the termios structure of the port and close "fd"
//return "1" on error and "0" on sucesses
int llsetOldtio();

/*Conection Functions*/

/*It constructs the control word specified in "type" and puts it on "word"*/
//return "1" on error and "0" on sucesses
int constructControlWord(char * word, int type, int response);

/*does the byte stuffing for a specific char array,
//@param word - word to do the byte stuffing of
//@param size - size of the word
//@param buff - buffer to write the final stuffed word to
//@return sizeBuff - size of the buff*/
int ByteStuffing(char * word, int size, char * buff);

/*Opens a conection using the "port"
//"machie_role" defines the role of the machine:
//0==Emitor
//1=Receiver
//return "1" on error and "0" on sucesses*/  
int llopen();
int llwrite(char* buf, int bufSize);
int llread(char* package);
int llclose();

int sendData(char* data, int size);

char* receiveMessage(int* status, int *size);

//int sendFile(char * file);
//int sendControlPacket(int type, char * filePath, int fileSize);
//int sendDataPacket(int N , char * buffer, int nBytes);
//int fsize(FILE *fp);


char getBCC2(char* data, int size);

/*Alarm Functions*/

/*Alarm Handler
It turn the STOP flag True after ALARM_COUNT turns 0
ALARM_COUNT decresses by one after "n" time defined with alarm(int n);
*/
void atende();

/*It defines the Number of timeouts
If "0" alarm will run only once.
*/
int setNumberOfTimeOuts(int n);

#endif



