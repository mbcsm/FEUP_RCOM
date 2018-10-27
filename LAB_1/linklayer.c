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

int sendControlWord(int fd, int type, int response){
	char * word;

	char A;//CAMPO DE ENDEREÇO
	char C;
	
	//TODO:MACROS FOR THIS 
	//Word is a response if int response == 1;

	//CAMPO DE ENDEREÇO
	if(MACHINE_ROLE == EMITOR && response==0){
		A=0x03;
	}
	else if(MACHINE_ROLE == RECEIVER && response==1){
		A=0x03;
	}
	else if(MACHINE_ROLE == RECEIVER && response==0){
		A=0x01;
	}
	else if(MACHINE_ROLE == EMITOR && response==1){
		A=0x01;
	}

	switch(type){//CAMPO DE CONTROLO
		case(SET):
			C=0x03;				
			break;
		case(DISC):
			C=0x0B;				
			break;
		case(UA):
			C=0x07;	
			break;
		case(RR):
			C=0x05;
			//TODO
			//Missing N(r)-->identifier of packet sent(either 1 or 0)				
			break;
		case(REJ):
			C=0x01;
			//TODO
			//Missing N(r)-->identifier of packet sent(either 1 or 0)				
			break;
		default:
			printf("Control word not defined\n");
			return 1;
	}

	word[0]=FLAG;
	word[1]=A;
	word[2]=C;
	word[3]=A^C;  //BCC
	word[4]=FLAG;


	write(fd, word, strlen(word));

	return 0;
}


int getControlWord(int fd){
	char * word;
	
	int i = 0;

	while(i <= 4) {
		res = read(fd, &byte, 1);
		
		if (res < 1)
			return word;

		word[i] = byte;
		
		i++;
	}

	return word;
}


int byteStuffing(char * word, int size, char * buff){
	int j=0;	
	for(int i = 0; i < size; i++){
		switch(word[i]){
			case ESCAPE:
				buff[j]=ESCAPE;
				j++;
				buff[j]=word[i]^STUFFING;
				j++;	
				break;
			case FLAG:
				buff[j]=ESCAPE;
				j++;
				buff[j]=word[i]^STUFFING;
				j++;
				break;
			default:
				buff[j]=word[i];
				j++; 
		}
	}
	
	return j;
}


int byteDestuffing(char * word, int size, char * buff){
	int j=0;
	for(int i = 0; i < size; i++){
		if(word[i]==ESCAPE){
			i++;			
			word[i]=word[i]^STUFFING;
		}

		buff[j]=word[i];
		j++;
	}
	
	return j;
}



int llopen(char* port, int machine_role, char * file){
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

	
	if(MACHINE_ROLE==RECEIVER){	
		while(STOP==FALSE) {
			if(getControlWord(fd)[2] == SET){
				sendControlWord(fd, UA, 0);
				printf("connected\n");
				STOP = TRUE;
			}
		}	
	}else if (MACHINE_ROLE==EMITOR){
		sendControlWord(fd, SET, 0);
		if(!ALARM_COUNTING || res>0){
			alarm(3);
			ALARM_FLAG=FALSE;
		}
		while(STOP==FALSE) {
			if(getControlWord(fd)[2] == UA){
				printf("connected\n");
				STOP = TRUE;
			}
		}
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
