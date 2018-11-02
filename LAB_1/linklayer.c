#include "linklayer.h"

struct linkLayer *ll;

//alarm variabels
volatile int ALARM_COUNTING=FALSE;
int ALARM_COUNT=5;
int ALARM_FLAG = FALSE;
volatile int STOP=FALSE;

struct termios oldtio, newtio;

int fd;

int configConection(char* serialPort, int role, int baudRate, int numTries, int timeOut){
	//alocate mem for stuct
    ll= (struct linkLayer*) malloc(sizeof(struct linkLayer));
	
	ll->serialPort=serialPort;
    ll->role=role;
    ll->baudRate=baudRate;
    ll->numTries=numTries;
    ll->timeOut=timeOut;

	return 0;
}

int freeMemoryLinkLayer(){
	free(ll);
	return 0;
}

int llsetNewtio(){

	  /*
		Open serial port device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	  */
  
    
    ll->serialPortDescriber = open(ll->serialPort, O_RDWR | O_NOCTTY );
    if (ll->serialPortDescriber <0) {perror(ll->serialPort); exit(-1); }

    if ( tcgetattr(ll->serialPortDescriber,&oldtio) == -1) { /* save current port settings */
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


    tcflush(ll->serialPortDescriber, TCIOFLUSH);

    if ( tcsetattr(ll->serialPortDescriber,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
    return 0;
}

int llsetOldtio(){
    tcsetattr(ll->serialPortDescriber,TCSANOW,&oldtio);
    close(ll->serialPortDescriber);
    return 0;
}

int sendControlWord(int type, int response){
	char * word;

	char A;//CAMPO DE ENDEREÇO
	char C;
	
	//TODO:MACROS FOR THIS 
	//Word is a response if int response == 1;

	//CAMPO DE ENDEREÇO
	if(ll->role == EMITOR && response==0){
		A=0x03;
	}
	else if(ll->role == RECEIVER && response==1){
		A=0x03;
	}
	else if(ll->role == RECEIVER && response==0){
		A=0x01;
	}
	else if(ll->role == EMITOR && response==1){
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


char* getControlWord(){
	char * word;
	char byte;
	int i = 0;

	while(i <= 4) {
		int res = read(fd, &byte, 1);
		
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



int llopen(){
	if(llsetNewtio(&fd)){
		printf("Problem seting the new termios structure\n");
		return 1;
	}

	//alarm set up
	setNumberOfTimeOuts(5); //seting 5 timeouts
	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao


	//char set[255], msg[255], buf[255];
	//int i=0, res;
	int res;
	
	if(ll->role==RECEIVER){	
		while(STOP==FALSE) {
			if(getControlWord()[2] == SET){
				sendControlWord(UA, 0);
				printf("connected\n");
				STOP = TRUE;
			}
		}	
	}else if (ll->role==EMITOR){
		sendControlWord(SET, 0);
		if(!ALARM_COUNTING || res>0){
			alarm(3);
			ALARM_FLAG=FALSE;
		}
		while(STOP==FALSE) {
			if(getControlWord()[2] == UA){
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

int llwrite(char* buf, int bufSize) {
	STOP = FALSE;
	int res=sendData(buf, bufSize);
	
	if(!ALARM_COUNTING || res>0){
		alarm(3);
		ALARM_FLAG=FALSE;
	}

	while(STOP == FALSE) {
		if (getControlWord()[2] == RR) {
			STOP = TRUE;
			break;
		} else if (getControlWord()[2] == REJ) {
			STOP = TRUE;
		}
	}
	return 0;
}

int llread(char* package){
	//TODO
	/*It returns the size of the package and -1 on failure*/
	return -1;
}

int llclose(){
	//TODO
	return -1;
}

int sendData(char* data, int size) {
	int packageSize = size + DATA_PACKET_SIZE;

	char package[packageSize];
	
	char F = FLAG;
	char A = 0x03;
	char C = 0x00; //TODO: WHAT TO PLACE HERE??!?!?!?!
	char BCC1 = A ^ C;
	char BCC2 = getBCC2(data, size);

	package[0] = F;
	package[1] = A;
	package[2] = C;
	package[3] = BCC1;
	memcpy(&package[4], data, size);
	package[4 + size] = BCC2;
	package[5 + size] = F;

	char * packageAfterStuffing;
	int packageSizeAfterStuffing = byteStuffing(package, packageSize, packageAfterStuffing);
	
	write(fd, packageAfterStuffing, packageSizeAfterStuffing);

	return 0;
}

/*
int sendFile(char * file){
	fileSize = fsize(file);

	sendControlPacket(2, file, fileSize);
	
	FILE *fp;
    fp = fopen (file,"r");
    if (fp == NULL) 
        return -1;
		
	int nBytes = 0, i = 0;
	char * buffer = malloc(256 * sizeof(char));

	while((nBytes = fread(buffer, sizeof(char), 256, fp)) > 0){
		sendDataPacket(i , buffer, nBytes)
		i++;
	}

    fclose(fp);
	
	sendControlPacket(3, file, fileSize);

	return 0;
}
int sendControlPacket(int type, char * filePath, int fileSize){

	int packageSize = 5 + strlen(fileSize) + strlen(filePath);

	char package[packageSize];

	package[0] = type;

	int j = 0;
	// define file size
	char fileSizeStr[8*2];
	sprintf(fileSizeStr, "%d", fileSize);

	package[1] = 0;
	package[2] = strlen(fileSize);
	j = 3;
	for(int i = 0; i < strlen(fileSizeStr); i++) {
		package[j] = fileSizeStr[i];
		j++;;
	}

	// define file name
	package[4] = 1;
	package[5] = strlen(filePath);
	j = 6;
	for(int i = 0; i < strlen(filePath); i++) {
		package[j] = filePath[i];
		j++;;
	}
	llwrite(package, packageSize);

	return 0;
}

int sendDataPacket(int N , char * buffer, int nBytes) {
	char C = 1;
	char L2 = nBytes / 256;
	char L1 = nBytes % 256;

	int packageSize = 4 + nBytes;

	char package[packageSize];

	package[0] = C;
	package[1] = N;
	package[2] = L2;
	package[3] = L1;

	memcpy(&package[4], buffer, nBytes);

	llwrite(buffer, packageSize);

	return 0;
}
*/
char getBCC2(char* data, int size) {
	char BCC2 = 0;

	for (int i = 0; i < size; i++)
		BCC2 ^= data[i];

	return BCC2;
}

int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
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
