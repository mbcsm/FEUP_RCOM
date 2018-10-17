#include "linklayer.h"

//alarm variabels
volatile int ALARM_COUNTING=FALSE;
int ALARM_COUNT=5;
int ALARM_FLAG = FALSE;
volatile int STOP=FALSE;

//machine 
int MACHINE_ROLE=NOT_DEFINED;

struct termios oldtio, newtio;

int llsetNewtio(char * port, int *fd){

	  /*
		Open serial port device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	  */
  
    
    *fd = open(port, O_RDWR | O_NOCTTY );
    if (*fd <0) {perror(port); exit(-1); }

    if ( tcgetattr(*fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* not blocking read */


    tcflush(*fd, TCIOFLUSH);

    if ( tcsetattr(*fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
    return 0;
}

int llsetOldtio(int *fd){
    tcsetattr(*fd,TCSANOW,&oldtio);
    close(*fd);
    return 0;
}

int constructControlWord(char * word, int type){

	

	strcpy(word, "0x7Eh");//FLAG

	if(MACHINE_ROLE == EMITOR)//CAMPO DE ENDEREÇO
		strcpy(word, "0x03");
	else
		strcpy(word, "0x01");
 	

	switch(type){//CAMPO DE CONTROLO
		case(SET):
			strcpy(word, "0x3h");
			return 0;				
			break;
		case(DISC):
			strcpy(word, "0xBh");
			return 0;				
			break;
		case(UA):
			strcpy(word, "0x7h");
			return 0;				
			break;
		case(RR):
			strcpy(word, "0x5h");
			return 0;				
			break;
		case(REJ):
			strcpy(word, "0x1h");
			return 0;				
			break;
		default:
			printf("Control word not defined\n");
			return 1;
	}
}



int llopen(char* port, int machine_role){
	MACHINE_ROLE=machine_role;
	
	int fd;
	if(llsetNewtio(port, &fd)){
		printf("Problem seting the new termios structure\n");
		return 1;
	}
	

//	int c;

	//alarm set up
	setNumberOfTimeOuts(5); //seting 5 timeouts
	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao


	char set[255], msg[255], buf[255];
	int i=0, res;

	constructControlWord(set, SET);
	
	if(MACHINE_ROLE==RECEIVER){	
		while(STOP==FALSE) {
			res= read(fd, buf, 1);
			if(res>0){
				msg[i]=buf[0];
				msg[i+1]=0;	
				i++;	
			}

			if( i!=0 && strncmp(set, msg, i)!=0){
				i=0;
				printf(" %s : trashed received\n", msg);
			}

			if(!ALARM_COUNTING || res>0){
   	   			alarm(3);                 // activa alarme de 3s
   	   			ALARM_FLAG=FALSE;
   			}
		
			if(strncmp(set, msg, strlen(set))==0 || i==254) STOP=TRUE;
		}

		printf("MSG received: ");	
		printf("%s : %d\n", msg, i);

		printf("\n---------------------------------\nSending_MSG\n---------------------------------\n");

		res = write(fd,msg, i);   
    		printf("%d bytes written\n", res);
	
	}else if (MACHINE_ROLE==EMITOR){
		res = write(fd,set, strlen(set));   
    		printf("%d bytes written\n", res);

		printf("\n---------------------------------\nReceiving_MSG\n---------------------------------\n");
		while(STOP==FALSE) {
			res= read(fd, buf, 1);
			if(res>0){
				msg[i]=buf[0];
				msg[i+1]=0;	
				i++;	
			}

			if( i!=0 && strncmp(set, msg, i)!=0){
				i=0;
				printf(" %s : trashed received\n", msg);
			}

			if(!ALARM_COUNTING || res>0){
   	   			alarm(3);                 // activa alarme de 3s
   	   			ALARM_FLAG=FALSE;
   			}
		
			if(strncmp(set, msg, strlen(set))==0 || i==254) STOP=TRUE;
		}

		printf("MSG received: ");	
		printf("%s : %d\n", msg, i);

	}	

	if(llsetOldtio(&fd)){
		printf("Problem seting the old termios structure\n");
		return 1;
	}
	return 0;
}


//Alarm Functions
void atende()                   // atende alarme
{
	ALARM_COUNTING=TRUE;
	ALARM_COUNT--;

	if(ALARM_COUNT==0)  STOP=TRUE;
}

int setNumberOfTimeOuts(int n){
	if(n>0){
		ALARM_COUNT=n;
		return 0;
	}else{
		return 1;
	}
}
