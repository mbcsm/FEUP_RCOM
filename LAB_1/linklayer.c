#include "linklayer.h"

struct linkLayer *ll;

//alarm variabels
int ALARM_COUNT=5;
int ALARM_FLAG = FALSE;
volatile int STOP=FALSE;
volatile int ALL_STOP=FALSE;

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
    ll->N=0;

	setNumberOfTimeOuts(ll->numTries);

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
    char word[5];
    int size = 0;

    char A;//CAMPO DE ENDEREÇO
    char C;
    

    //TODO:MACROS FOR THIS
    //Word is a response if int response == 1;

    //CAMPO DE ENDEREÇOW
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
            if(ll->N==1){
                C=(0x05|N_EQUALS_1);
            }else{
                C=0x05;
            }        
            break;
        case(REJ):
            if(ll->N==1){
                C=(0x01|N_EQUALS_1);
            }else{
                C=0x01;
            }                
            break;
        default:
            printf("Control word not defined\n");
            return 1;
    }

    word[size]=FLAG;
    size++;
    word[size]=A;
    size++;
    word[size]=C;
    size++;
    word[size]=A^C;  //BCC
    size++;
    word[size]=FLAG;
    size++;

    write(ll->serialPortDescriber, word, size);

    printf("Word Sent: %x | %x | %x | %x | %x\n", word[0], word[1], word[2], word[3], word[4]);

    return 0;
}

/*
char* getControlWord(){
    char * word;
    char byte;
    int i = 0;

    while(i <= 4) {
        int res = llread("asd");
        
    
        if (res < 1)
            return word;

        word[i] = byte;
        
        i++;
    }

    return word;
}
*/

int byteStuffing(char * word, int size, char * buff){
    buff[0]=word[0];
    int j=1;    
    int i=1;
    for(; i < size-1; i++){
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
    buff[j]=word[i];
    j++;
    
    return j;
}


int byteDestuffing(char * word, int size, char * buff){
    int j=0;
    int i = 4;
    for(i; i < size-2; i++){
        printf(" %x ", word[i]);
    }

    i = 4;
    for(i; i < size-2; i++){
        if(word[i]==ESCAPE){
            i++;            
            word[i]=word[i]^STUFFING;
        }

        buff[j]=word[i];
        j++;
        printf(" %x ", buff[j]);
    }
    
    return j;
}



int llopen(){
    if(llsetNewtio()){
        printf("Problem seting the new termios structure\n");
        return 1;
    }

    //alarm set up
    setNumberOfTimeOuts(ll->numTries); //seting 5 timeouts
    (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

    
    if(ll->role==RECEIVER){    
        while(STOP==FALSE) {
            
            int status = 0, size = 0, type = 0;
            char*  message = receiveMessage(&status, &size);
            if(message[2] == 0x03){
                sendControlWord(UA, 0);
                printf("connected\n");
                STOP = TRUE;
            }
        }    
    }else if (ll->role==EMITOR){
        sendControlWord(SET, 0);      
	while(STOP==FALSE) {
            
            int status = 0, size = 0, type = 0;
            char*  message = receiveMessage(&status, &size);

            printf("Message received:");
        
		int i;
    
		for(i=0; i<size; i++){
                printf(" %x |", message[i]);
            }
            printf("\n");

            if(message[2] == 0x07){
                printf("connected\n");
                STOP = TRUE;
            }
        }
    }    

    return 0;
}

int llwrite(char* buf, int bufSize) {
    printf("Word To Send LLWrite Start :");
    int i=0;
    while(i<bufSize){
        printf(" %x |", buf[i]);
        i++;
    }
    printf("\n");


    setNumberOfTimeOuts(ll->numTries);
    ALL_STOP=FALSE;
    int STOP2=FALSE;
    while(STOP2==FALSE && ALL_STOP!=TRUE){
        int res=sendData(buf, bufSize);

	alarm(ll->timeOut);
    	STOP=FALSE;

        while(STOP == FALSE) {
            
            int status = 0, size = 0, type = 0;
            char*  message = receiveMessage(&status, &size);

	    if(status==4){
		continue;	
             }	

            printf("Message Received: 0x%2x | 0x%2x | 0x%2x | 0x%2x | 0x%2x\n", message[0], message[1], message[2], message[3], message[4]);
            
            if(ll->N==1){

                if(message[2] == 0x05){
                    STOP = TRUE;
                    STOP2= TRUE;
                    break;
                }else if(message[2] == (0x01|N_EQUALS_1)){
                    STOP = TRUE;
                    STOP2= FALSE;
                    break;
                }else if(message[2] == (0x05|N_EQUALS_1)){
                    STOP = TRUE;
                    STOP2= FALSE;
                    break;
                }
            }else{
                if (message[2] == (0x05|N_EQUALS_1)) {
                    STOP = TRUE;
                    STOP2= TRUE;
                    break;
                } else if (message[2] == 0x01) {
                    STOP = TRUE;
                    STOP2= FALSE;
                    break;
                }else if(message[2] == 0x05){
                    STOP = TRUE;
                    STOP2= FALSE;
                    break;
                }
            }            
        }
    }
	if(ALL_STOP==TRUE){
		return 1;	
	}

    if(ll->N==1){
        ll->N=0;
    }else{
        ll->N=1;
    }
    return 0;
}

int llread(char* package){
    int dataSize;
    int received = FALSE;
    int status = 0, size = 0, type = 0;
    char* message;
    int i;
    while (received == FALSE) {
        message = receiveMessage(&status, &size);
        switch (status) {
        case -1:
            sendControlWord(REJ, 1);
            break;
        case 1:
            if (message[2] == 0x0b)
                received = TRUE;
            break;
        case 2:
            i = 0;
            for(; i<size; i++){
                package[i] = message[i];
            }

            if(ll->N==1){
                ll->N=0;
            }else{
                ll->N=1;
            }

            sendControlWord(RR, 1);
            received = TRUE;
            break;
        case 3:
            sendControlWord(RR, 1);
            break;
        }
    }
    /*It returns the size of the package and -1 on failure*/
    return size;
}

int llclose(){

    if(ll->role==RECEIVER){    
        int state = 0;
        while(STOP==FALSE) {
            int status = 0, size = 0, type = 0;
            char*  message = receiveMessage(&status, &size);
            if(message[2] == 0x0b){
                state = 1;
            }else if(message[2] == UA && state == 1){
                printf("Disconnected\n");
                STOP = TRUE;
            }
        }
    }else if (ll->role==EMITOR){
        sendControlWord(DISC, 0);
        while(STOP==FALSE) {
        
            int status = 0, size = 0, type = 0;
            char*  message = receiveMessage(&status, &size);
            if(message[2] == 0x0b){
                sendControlWord(UA, 0);
                printf("Disconnected\n");
                STOP = TRUE;
            }
        }    
    }

    if(llsetOldtio()){
        printf("Problem seting the old termios structure\n");
        return 1;
    }    

    return -1;
}


char* receiveMessage(int * status, int * size) {
    int steps = 0;
    int tempSize = 0;

    unsigned char* message = malloc(512);
    
    int done = FALSE;
    while (steps < 5 && STOP!=TRUE) {
        unsigned char c;

        int bytes = read(ll->serialPortDescriber, &c, 1);

        if(bytes == -1 || bytes == 0)
            continue;

        switch (steps) {
        case 0:
            if (c == 0x7e) {
                message[tempSize] = c;
                tempSize++;
                steps = 1;
            }
            break;
        case 1:
            message[tempSize] = c;
            tempSize++;
            steps = 2;
            
            break;
        case 2:
                message[tempSize] = c;
                tempSize++;
                steps = 3;
            break;
        case 3:
            if (c == message[1] ^ message[2]) {
                message[tempSize] = c;
                tempSize++;
                steps = 4;
            }
            break;
        case 4:
            message[tempSize] = c;
            tempSize++;
            if (c == 0x7e && tempSize == 5) {
                *status = 1;//COMMAND
                steps = 5;
            }else if (c == 0x7e && tempSize > 5) {
                *status = 2;//DATA
                steps = 5;
            }
            break;
        default:
            break;
        }
    }
    

	if(STOP==TRUE){
		*status=4;
		return message;
	}

    if(*status==2){
        if( !(ll->N==1 && message[2]==N_EQUALS_1) && !(ll->N==0 && message[2]==0x00)){
            *status = 3;
        }
    }

    if(*status == 2){
        unsigned char* messageAfterDestuffing = malloc(tempSize);
        int i = 4, j = 0;
        for(i; i < tempSize-2; i++){
            messageAfterDestuffing[j]=message[i];
            j++;
        }
        //tempSize = byteDestuffing(message, tempSize, &messageAfterDestuffing);


        //check if BCC1 is correct
        if (message[3] != (message[1] ^ message[2])) {
            *status = -1;
            return messageAfterDestuffing;
        }

        unsigned char BCC2 = getBCC2(messageAfterDestuffing, j);

        //check if BCC2 is correct
        if (BCC2 != message[4 + j]) {
            *status = -1;
            return messageAfterDestuffing;
        }
        *size = j;
        return messageAfterDestuffing;

    }

    return message;
}


int sendData(char* data, int size) {

    printf("Data Received Before Stuffing:\n");
    int i=0;
    while(i<size){
        printf(" %x |", data[i]);
        i++;
    }
    printf("\n");


    int packageSize=size + DATA_PACKET_SIZE;

    char package[packageSize];
    
    char F = 0x7e;
    char A = 0x03;

    char C = 0x00;
    if(ll->N==1){
        C=N_EQUALS_1;
    }
    char BCC1 = A ^ C;

    char BCC2 = getBCC2(data, size);

    package[0] = F;
    package[1] = A;
    package[2] = C;
    package[3] = BCC1;
    memcpy(&package[4], data, size);

    printf("BCC2: %x\n", BCC2);
    package[packageSize-2] = BCC2;
    package[packageSize-1] = F;

    //char * packageAfterStuffing=malloc(packageSize*2);
    //printf("SendDAta: It's Here 48\n");
    //int packageSizeAfterStuffing = byteStuffing(package, packageSize, packageAfterStuffing);

    printf("Package Sent Before Stuffing:\n");
    i=0;
    while(i<packageSize){
        printf(" %x |", package[i]);
        i++;
    }
    printf("\n");

    char * packageAfterStuffing=malloc(packageSize*2);
    int packageSizeAfterStuffing=byteStuffing(package, packageSize, packageAfterStuffing);
    
    write(ll->serialPortDescriber, packageAfterStuffing, packageSizeAfterStuffing);

    printf("Package Sent After Stuffing:\n");
    i=0;
    while(i<packageSizeAfterStuffing){
        printf(" %x |", packageAfterStuffing[i]);
        i++;
    }
    printf("\n");
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

    int i = 0;
    for (i; i < size; i++){
        BCC2 ^= data[i];
    }

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
    ALARM_COUNT--;
    STOP=TRUE;
    if(ALARM_COUNT==0)  ALL_STOP=TRUE;
}

int setNumberOfTimeOuts(int n){
    if(n>0){
        ALARM_COUNT=n;
        return 0;
    }else{
        return 1;
    }
}



