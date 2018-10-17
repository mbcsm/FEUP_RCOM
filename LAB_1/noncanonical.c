/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;
volatile int ALARM_FLAG=TRUE;
int ALARM_COUNT=5;

void atende()                   // atende alarme
{
	ALARM_FLAG=TRUE;
	ALARM_COUNT--;

	if(ALARM_COUNT==0)  STOP=TRUE;
	
	printf("Alarm time out\n");
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


   // while (STOP==FALSE) {       /* loop for input */
   //   res = read(fd,buf,255);   /* returns after 5 chars have been input */
   //   buf[res]=0;               /* so we can printf... */
   //   printf(":%s:%d\n", buf, res);
   //   if (buf[0]=='z') STOP=TRUE;
   // }

	char set[255]="FLAG,A,C,BCC,FLAG";
	char msg[255];
	int i=0;

	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

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

		if(ALARM_FLAG || res>0){
      			alarm(3);                 // activa alarme de 3s
      			ALARM_FLAG=FALSE;
   		}
	
		if(strncmp(set, msg, strlen(set))==0 || i==254) STOP=TRUE;
	}

	printf("MSG received: ");	
	printf("%s : %d\n", msg, i);

	printf("---------------------------------/nSending_MSG/n--------------------\n");

	res = write(fd,msg, i);   
    	printf("%d bytes written\n", res);




  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
