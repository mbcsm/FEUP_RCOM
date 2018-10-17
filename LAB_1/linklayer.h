#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

//MACHINE_ROLE
#define NOT_DEFINED -1
#define EMITOR 0
#define RECEIVER 1

//CONTROL WORDS
#define SET 0

//PORT
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//MISC.
#define FALSE 0
#define TRUE 1


/* port control functions*/
 
//Set the new termios structure on "port" and open it on "fd"
//return "1" on error and "0" on sucesses
int llsetNewtio(char * port, int *fd);

//Reset the termios structure of the port and close "fd"
//return "1" on error and "0" on sucesses
int llsetOldtio(int *fd);

/*Conection Functions*/

/*It constructs the control word specified in "type" and puts it on "word"*/
//return "1" on error and "0" on sucesses
int constructControlWord(char * word, int type);

/*Opens a conection using the "port"
//"machie_role" defines the role of the machine:
//0==Emitor
//1=Receiver
//return "1" on error and "0" on sucesses*/  
int llopen(char* port, int machine_role);


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
