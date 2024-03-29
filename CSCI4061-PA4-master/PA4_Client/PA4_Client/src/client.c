/*test machine: KH4250-15 * date: 12/04/2019
* name: Avery Swank , Brandon Chan
* x500: swank026 , chanx411 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/wait.h>
#include "../include/protocol.h"

FILE *logfp;

// create the mapper file name using a mapperID
// return format: Mapper_i.txt
void createMapperName(char mapper[], int mapperID) {
  char numStr[4];
  memset(mapper, '\0', 256);
  strcat(mapper, "Mapper_");
  sprintf(numStr, "%d", mapperID);
  strcat(mapper, numStr);
  strcat(mapper, ".txt\0");
}

void mapClient(int mapperID, char* serverIP, int serverPort){

    // Create a TCP socket.
    int clientSocket = socket(AF_INET , SOCK_STREAM , 0);

    // Specify an address to connect
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(serverPort);
    address.sin_addr.s_addr = inet_addr(serverIP);

    char mapperLine[256];   // used to hold the line of a mapper_i.txt file
    char textLine[256];     // used to hold word from a text file
    char mapper[256];       // mapper name - used to open file
    FILE *file;
    FILE *mapperFile;

    // creating the mapper file name
    createMapperName(mapper, mapperID);
    int opened = chdir("./MapperInput");
    file = fopen(mapper, "r");

    // Check MapperInput exists
    if(opened != 0){
      //fprintf(logfp, "Failed to open MapperInput\n");
      exit(1);
    }

    // Check file exists
    if(file == NULL){
      //fprintf(logfp, "Failed to open file: mapper\n");
      exit(1);
    }

    // Connect it.
    if (connect(clientSocket, (struct sockaddr *) &address, sizeof(address)) != 0) {
      perror("Connection failed!\n");
    } else {
      fprintf(logfp, "[%d] open connection\n", mapperID);

      int msgbuf[REQUEST_MSG_SIZE];
      int rspbuf[LONG_RESPONSE_MSG_SIZE];

      // init buffers at zero
      for(int i = 2; i < REQUEST_MSG_SIZE; i++) { msgbuf[i] = 0; }
      for(int i = 0; i < LONG_RESPONSE_MSG_SIZE; i++) { rspbuf[i] = 0; }

      // Request formatting: [mapperID, request code, data...]
      msgbuf[0] = mapperID;

      // send CHECKIN
      msgbuf[1] = CHECKIN;
      send(clientSocket, msgbuf, sizeof(msgbuf), 0);
      recv(clientSocket, rspbuf, sizeof(rspbuf), 0);
      fprintf(logfp, "[%d] CHECKIN: %d %d\n", mapperID, rspbuf[1], rspbuf[2]);

      // count num msgs sent to the server
      int count = 0;

      // iterate lines in a mapper_i.txt file
      while(fgets(mapperLine, sizeof(mapperLine), file)) {

        // init buffers to zero
        for(int i = 2; i < REQUEST_MSG_SIZE; i++) { msgbuf[i] = 0; }
        for(int i = 0; i < LONG_RESPONSE_MSG_SIZE; i++) { rspbuf[i] = 0; }

        // break apart the textfile filePath and fileName
        int len = strlen(mapperLine);
        char cwd[256];
        char textFile[256] = "";
        while(strncmp(mapperLine+len, "/", 1)) {
          len--;
          if(!strncmp(mapperLine+len, "\n", 1)) {
            memset(mapperLine+len, '\0', 1);
          }
        }
        strcat(textFile, mapperLine+len+1);
        memset(mapperLine+len, '\0', 1);

        // open text file
        getcwd(cwd, 256);
        chdir("../");
        chdir(mapperLine);

        mapperFile = fopen(textFile, "r");
        if(mapperFile == NULL) {
          fprintf(logfp, "couldn't open txt file\n");
          exit(1);
        }

        // open individual files from mapper_i and parse data
        while(fgets(textLine, sizeof(textLine), mapperFile)) {
          int firstLetter = tolower(textLine[0]);
          msgbuf[firstLetter-95]++;
        }

        // send UPDATE_AZLIST
        msgbuf[1] = UPDATE_AZLIST;
        send(clientSocket, msgbuf, sizeof(msgbuf), 0);
        recv(clientSocket, rspbuf, sizeof(rspbuf), 0);
        fprintf(logfp, "[%d] UPDATE_AZLIST: %d\n", mapperID, ++count);

        // send GET_AZLIST
        msgbuf[1] = GET_AZLIST;
        send(clientSocket, msgbuf, sizeof(msgbuf), 0);
        recv(clientSocket, rspbuf, sizeof(rspbuf), 0);

        // print azList
        fprintf(logfp, "[%d] GET_AZLIST: ", mapperID);
        for(int i = 0; i < LONG_RESPONSE_MSG_SIZE; i++){
          fprintf(logfp, "%d ", rspbuf[i]);
        }
        fprintf(logfp, "\n");

        // send GET_MAPPER_UPDATES
        msgbuf[1] = GET_MAPPER_UPDATES;
        send(clientSocket, msgbuf, sizeof(msgbuf), 0);
        recv(clientSocket, rspbuf, sizeof(rspbuf), 0);
        fprintf(logfp, "[%d] GET_MAPPER_UPDATES: %d %d\n", mapperID, rspbuf[1], rspbuf[2]);

        // send GET_ALL_UPDATES
        msgbuf[1] = GET_ALL_UPDATES;
        send(clientSocket, msgbuf, sizeof(msgbuf), 0);
        recv(clientSocket, rspbuf, sizeof(rspbuf), 0);
        fprintf(logfp, "[%d] GET_ALL_UPDATES: %d %d\n", mapperID, rspbuf[1], rspbuf[2]);

        // Reset current directory from when opening a file
        getcwd(cwd, 256);
        int cwLen = strlen(cwd);
        while(strcmp(cwd+(cwLen-10), "PA4_Client")) {
          chdir("../");
          getcwd(cwd,256);
          cwLen = strlen(cwd);
        }
        chdir("src/MapperInput");
      }

      // send CHECKOUT
      msgbuf[1] = CHECKOUT;
      send(clientSocket, msgbuf, sizeof(msgbuf), 0);
      recv(clientSocket, rspbuf, sizeof(rspbuf), 0);
      fprintf(logfp, "[%d] CHECKOUT: %d %d\n", mapperID, rspbuf[1], rspbuf[2]);

      // close connection
      msgbuf[1] = CHECKOUT;
      for(int i=2; i<28; i++) {
        msgbuf[i] = 0;
      }

      close(clientSocket);
      fprintf(logfp, "[%d] close connection\n", mapperID);
    }
}

void createLogFile(void) {
    pid_t p = fork();
    if (p==0)
        execl("/bin/rm", "rm", "-rf", "log", NULL);

    wait(NULL);
    mkdir("log", ACCESSPERMS);
    logfp = fopen("log/log_client.txt", "w");
}

int main(int argc, char *argv[]) {

    int mappers;
    char folderName[256] = {'\0'};
    char *server_ip;
    int server_port;

    // 4 arguments
    if (argc == 5) {
        strcpy(folderName, argv[1]);
        mappers = atoi(argv[2]);
        server_ip = argv[3];
        server_port = atoi(argv[4]);
        if (mappers > MAX_MAPPER_PER_MASTER) {
            printf("Maximum number of mappers is %d.\n", MAX_MAPPER_PER_MASTER);
            printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
            exit(1);
        }

    } else {
        printf("Invalid or less number of arguments provided\n");
        printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
        exit(1);
    }

    // create log file
    createLogFile();

    // phase1 - File Path Partitioning
    traverseFS(mappers, folderName);

    // Phase2 - Mapper Clients's Deterministic Request Handling
    pid_t pids[mappers];

    // Start forking for every mapper process that wants to be created
    for(int i = 0; i < mappers; i++){

      pids[i] = fork();
      int mapperID = i+1;

      if(pids[i] == 0)
        mapClient(mapperID, server_ip, server_port);
    }

    // Wait for all to finish before closing
    for(int i = 0; i < mappers; i++){
      wait(&pids[i]);
    }

    fclose(logfp);
    return 0;

} 
