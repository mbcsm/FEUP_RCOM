#include "linklayer.h"

int main(int argc, char** argv){
	if ( (argc < 3) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) && (argv[2] == NULL) )) {
      		printf("Usage:\tnserial SerialPort FileToSend\n\tex: nserial /dev/ttyS1 FileToSend\n");
      		exit(1);
    	}
	
	return llopen(argv[1], 1, argv[2]);
}
