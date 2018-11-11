#include "linklayer.h"
#include "applicationlayer.h"
#include <stdlib.h>

int main(int argc, char** argv){
	if ( (argc < 3) || ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) && (argv[2] == NULL) )) {
      		printf("Usage:\tnserial SerialPort FileToSend\n\tex: nserial /dev/ttyS1 FileToSend\n");
      		exit(1);
    	}
	
	int role = strtol(argv[2], NULL, 10);
    applicationLayerMain(argv[1], role, argv[3],10, 5, 5, 2);
	return 0;
}
