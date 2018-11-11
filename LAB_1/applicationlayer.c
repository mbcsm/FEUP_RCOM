#include "applicationlayer.h"

struct applicationLayer *al;

int applicationLayerMain(char* serialPort,  int role, char *fileName, int maxDataSize, int baudRate, int numTries, int timeOut){
    //alocate mem for stuct
    al= (struct applicationLayer*) malloc(sizeof(struct applicationLayer));

    //Config Application Parameters
    if(configApplication(serialPort, role, fileName, maxDataSize)==ERROR){
        printf("ERROR: Could not config Application\n");
        return ERROR;
    }

    //print the conection info
    printf("-------------------\n");
    printf("--Connection info--\n");
    printf("-------------------\n");
    printf("Baudrate: %d\nMax Size of Data Sent: %d\nMax Number of Tries: %d\nTime until Timeout: %d\n",baudRate,maxDataSize,numTries,timeOut);

    //set Parameters Of the Conection
    if(configConection(serialPort, role, baudRate, numTries, timeOut)!=OK){
        printf("ERROR: Couldn't configurate Conection\n");
        return ERROR;
    }

    //Open the Connection (llopen())
    printf("\n\n--Opening Conection--\n");
    if (llopen() == ERROR) {
        printf("Error: Could not estabilish conection\n");
        return ERROR;
    }

    //Role Functions
    switch(al->role){
        case EMITOR: {
            printf("Sending File\n");
            return sendFile();
            break;
        }
        case RECEIVER: {
            printf("Receiving File\n");
            return receiveFile();
            break;
        }
        default:{
            return ERROR;
            break;
        }
    }

    //End Conection
    printf("\n\n--Closing Conection--\n");
    if(llclose() == ERROR) {
        printf("ERROR:Could not disconnect properly\n");
        return ERROR;
    }

    //Free Alocated Memory
    free(al->fileName);
    free(al);
    
    if(freeMemoryLinkLayer()==ERROR){
        printf("ERROR:Possible memory leak in linkLayer");
        return ERROR;
    }
    
    return OK;

}

int configApplication(char * serialPort, int role, char * fileName, int maxDataSize){
    al->role=role;
    al->fileName=fileName;
    al->maxDataSize=maxDataSize;
    al->sizeDataProcessed=0;

    return OK;
}

int openFile(){
    //Open File acording to role
    if(al->role==EMITOR){
        al->fileDescriptor = fopen(al->fileName, "r");
        if (al->fileDescriptor == NULL) {
            printf("Erro: Could not open file\n");
            return ERROR;
        }

        char * fileBuf= malloc(al->maxDataSize * sizeof(char));
        int readBytes = fread(fileBuf, sizeof(char), al->maxDataSize, al->fileDescriptor);

        //Calculate Size Of File
        al->sizeOfFile = calculateFileSize();
        printf("delete this line : SIZE OF FILE: %d", al->sizeOfFile);
        if (al->sizeOfFile == ERROR) {
            printf("Erro: Could not get file size\n");
            return ERROR;
        }
        return OK;

    }else if(al->role==RECEIVER){
        al->fileDescriptor = fopen(al->fileName, "wb");
        if (al->fileDescriptor == NULL) {
            printf("Erro: Could not create file\n");
            return ERROR;
        }
        return OK;

    }else{
        printf("ERROR: Role of machine not recognised\n");
        return ERROR;
    }
}

//------------------------------------------------------------------------------------------
//Package Handlers
//------------------------------------------------------------------------------------------

int sendControlPackage(int type){
    //Calculate Size of Package
    char temp[255];
    snprintf(temp,sizeof temp,"%ld",al->sizeOfFile);
    int package_size = 5 + strlen(temp) + strlen(al->fileName);

    char package[package_size];
    int c = 0;

    //Construct Package
    //C
    package[c] = type;
    c++;

    //T1 (Size of File)
    package[c] = FILE_SIZE;
    c++;
    //L1 (Lenght of Section that dercribes Size of File)
    package[c] = strlen(temp);
    c++;
    //V1 (Size of File)
    int i = 0;
    for ( i; i < strlen(temp); i++) {
        package[c] = temp[i];
        c++;
    }

    //T2 (File Name)
    package[c] = FILE_NAME;
        c++;
    //L2 (Lenght of Section that dercribes Name of File)    
    package[c] = strlen(al->fileName);
        c++;
    //V2 (Name of File)    
    int k = 0;
    for (k; k < strlen(al->fileName); k++) {
        package[c] = al->fileName[k];
        c++;
    }

    //Send Package
    if(llwrite(package, package_size) == ERROR) {
        printf("ERROR: Could not send control Package with llwrite\n");
        return ERROR;
    }
    return OK;
}

int sendDataPackage(int N, int size, char* data){
    int package_size=size+4;
    char* package=(char*)malloc(size);

    //Construct Package
    //C
    package[0] = DATA;
    //N
    package[1] = (char) ((N) % 255);
    //L1
    package[2] = (char) (size / 256);
    //L2
    package[3] = (char) (size % 256);

    //P (Data Section)
    memcpy(&package[4], data, size);

    printf("\n--------------------------------------------\nFile Data in Send Data Package:");
   
    int i;
    for(i=0; i<size; i++){
        printf(" %2x |", data[i]);
    }
    printf("\n");

    //Send Package
    if(llwrite(package, package_size) == ERROR) {
        printf("ERROR: Could not send control Package with llwrite\n");
        return ERROR;
    }
    return OK;
}

int receiveControlPackage(){
    //Read Package
    char package[512];
    int package_size = llread(package);

    if (package_size <= 0) {
        printf("ERROR: Could not receive control package from llread\n");
        return ERROR;
    }

    //Process Control Package
    switch (package[0]) {
        case START: {
            unsigned int c = 1;
            
            //Get File Size
            if (package[c] != FILE_SIZE){
                printf("ERROR:Package not in the expected format\n");
                return ERROR;
            }
            c++;
            
            int fieldSize=package[c];
            c++;
            char file_size[fieldSize+1];

            int i;
            for (i = 0; i < fieldSize; i++) {
                file_size[i] = package[c+i];
            }
            file_size[i]='\0';
            c+=i;
    
            //Get File Name
            
    if (package[c] != FILE_NAME){
                printf("ERROR:Package not in the expected format\n");
                //return ERROR;
            }
            c++;

            fieldSize=package[c];
            c++;
            char file_name[fieldSize+1];

            for (i = 0; i < fieldSize; i++) {
                file_name[i] = package[c+i];
            }
            file_name[i]='\0';
            c+=i;

            //Save Information
            al->fileName = (char*) malloc(strlen((char*) file_name));
            strcpy(al->fileName, (char*) file_name);
            al->sizeOfFile = atoi((char *)file_size);

            return START;
        break;
        }
        case END: {
            unsigned int c = 1;
            
            //Get File Size
            if (package[c] != FILE_SIZE){
                printf("ERROR:Package not in the expected format\n");
                return ERROR;
            }
            c++;
            
            int fieldSize=package[c];
            c++;
            char file_size[fieldSize+1];

            int i;
            for (i = 0; i < fieldSize; i++) {
                file_size[i] = package[c+i];
            }
            file_size[i]='\0';
            c+=i;
    
            //Get File Name
            if (package[c] != FILE_NAME){
                printf("ERROR:Package not in the expected format\n");
                return ERROR;
            }
            c++;

            fieldSize=package[c];
            c++;
            char file_name[fieldSize+1];

            for (i = 0; i < fieldSize; i++) {
                file_name[i] = package[c+i];
            }
            file_name[i]='\0';
            c+=i;

            //Compare Information with Current Values
            if (strcmp((char*) file_name, al->fileName) != 0){
                printf("Discrepancy found with File Name\n The name of the File may not be correct\n StartPackage:%s - EndPackage:%s\n",al->fileName,file_name);
            }
            
            int size_at_end_Package = atoi((const char *)file_size);
            if (size_at_end_Package!= al->sizeOfFile) {
                printf("Discrepancy found with File Size\nThe File may not have been sent correctly\nStartPackage:%ld - EndPackage:%d\n", al->sizeOfFile, size_at_end_Package);
            }
            
            return END;
        break;
        }
        default:{
            printf("Erro: Received an not expeted Package\n");
            return ERROR;
        }
    }
}

int receiveDataPackage(int* N, char* buf, int* length){
    //Read Package
    char package[512];
    int package_size = llread(package);

    if (package_size <= 0) {
        printf("ERROR: Could not receive control package from llread\n");
        return ERROR;
    }

    //C --> Check if package is a DATA package
    int C = package[0];
    if (C != DATA) {
        printf("ERROR: Received package is not a data package (C = %d).\n", C);
        return ERROR;
    }

    //N --> Sequence Number of Package
    *N = (int) package[1];

    //L2 and L1 fields --> Size of Data Field
    int L2 = package[2];
    int L1 = package[3];

    //Calculate the Size of the Data Field
    *length = 256 * L2 + L1;

    //Copy file chunk to the buffer
	int i;
    
	for(i=0; i<*length; i++){
        buf[i]=package[i+4];
    }

    return OK;
}

//------------------------------------------------------------------------------------------
//Emitor Only Functions
//------------------------------------------------------------------------------------------

long int calculateFileSize(){

    int prev=ftell(al->fileDescriptor);

    //seeking end of file
    if(fseek(al->fileDescriptor, 0L, SEEK_END)==ERROR){
        printf("ERROR: Could not find end of file. \n");
        return ERROR;    
    }

    //calculating file size
    long int size = ftell(al->fileDescriptor);

    if(fseek(al->fileDescriptor, 0L, SEEK_SET)==ERROR){
        printf("ERROR: Could not find end of file. \n");
        return ERROR;    
    }

    return size;
}

int sendFile(){
    //Open File
    if(openFile()==ERROR){
        printf("ERROR: Couldn't open File\n");
        return ERROR;
    }

    //Send "START" Package
    printf("\n--Sending File--\n");
    if (sendControlPackage(START) == ERROR) {
        printf("Erro: Could not send the \"Start Package\"\n");
        return ERROR;
    }
    
    //Send File Data
    char * fileBuf= malloc(al->maxDataSize * sizeof(char));
    unsigned int  readBytes = 0;
    int N=0;
    int return_value=0;	
    int i;
    for(i=0; i<al->maxDataSize; i++){
            fileBuf[i]=0;
    }

    while ((readBytes = fread(fileBuf, sizeof(char), al->maxDataSize, al->fileDescriptor))){

        printf("Bytes Read From File :");
	int i;
        for(i=0; i<10; i++){
            printf(" %x |", fileBuf[i]);
        }
        printf("\n");
        
        //send data chunks inside data packages
        if (sendDataPackage(N, readBytes, fileBuf)==ERROR) {
            printf("Couldn't send Data Package\n");
            free(fileBuf);
            return_value=ERROR;
	    break;	 
        }
    
        //reset file buffer
        fileBuf = memset(fileBuf, 0, (al->maxDataSize));

        // increment no. of written bytes
        al->sizeDataProcessed += readBytes;
        printf("Package n. %d sent\n",N);
        printf("Sent:%ld%% ...\n", (al->sizeDataProcessed/al->sizeOfFile)*100);
        N++;

        for(i=0; i<al->maxDataSize; i++){
            fileBuf[i]=0;
        }

    }

    free(fileBuf);

    printf("Sended: %ld bytes of the expected %ld bytes\n", al->sizeDataProcessed, al->sizeOfFile);

    //Send "END" Package
    if(return_value!=ERROR){
    	if (sendControlPackage(END) == ERROR) {
        	printf("Erro: Could not send the \"End Package\"\n");
        	return ERROR;
    	}
    }	
    
    //Close File
    if (fclose(al->fileDescriptor) != 0) {
        printf("Erro: Could not close File\n");
        return ERROR;
    }
    
    return OK;
}

//------------------------------------------------------------------------------------------
//Receiver Only Functions
//------------------------------------------------------------------------------------------

int receiveFile(){
    printf("\n--Receiving File--\n");

    //Receive "Start" Package
    if (receiveControlPackage()!= START) {
        printf("Erro: Could not receive \"Start Package\"\n");
        return ERROR;
    }

    //Open File
    if(openFile()==ERROR){
        printf("ERROR: Couldn't open File\n");
        return ERROR;
    }

    //Receive Data and Write it to File
    int N=-1;
    char fileBuf[al->maxDataSize];
    int return_value=0;	
    while (al->sizeOfFile != al->sizeDataProcessed) {
        int lastN = N;
        int length = 0;
        
        // receive data package with chunk and put chunk in fileBuf
        if (receiveDataPackage(&N, fileBuf, &length)==ERROR) {
            printf("ERROR: Could not receive data package.\n");
            return_value=ERROR;
            break;
        }

	if(lastN==127){
	   lastN=-129;	 
	}

        /*if(lastN==255){
            lastN=-1;
        }*/

        /*if (lastN + 1 < N ) {
            printf("ERROR: Received sequence no. was %d instead of %d.\n", N, lastN + 1);
            //return ERROR;
        }*/

        //if(lastN+1==N){
            // write data chunk to file
            fwrite(fileBuf, sizeof(char), length, al->fileDescriptor);
        
            // incremet no. of read bytes
            al->sizeDataProcessed+= length;
            printf("%ld%% ...\n", (al->sizeDataProcessed/al->sizeOfFile)*100);
        //}
    }

    //Receive "END" Package
    if(return_value!=ERROR){
    	if(receiveControlPackage()!= END){
        	printf("ERROR: Could not receive \"End Package\"\n");
        	return ERROR;
    	}
    }	
    
    printf("Received: %ld bytes of the expected %ld bytes\n", al->sizeDataProcessed, al->sizeOfFile);
    
    //Close file
    if (fclose(al->fileDescriptor)!= 0) {
        printf("ERROR: Could not close File\n");
        return ERROR;
    }

    return OK;
}



