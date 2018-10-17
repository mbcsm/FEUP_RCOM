#include <stdio.h>
#include "linklayer.h"

int main(int argc, char** argv){
	if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	     (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      		exit(1);
    	}
	
	return llopen(argv[1], 0);
}
