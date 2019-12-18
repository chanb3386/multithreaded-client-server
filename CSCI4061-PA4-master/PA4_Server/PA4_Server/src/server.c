/*test machine: KH4250-15 * date: 12/04/2019
* name: Avery Swank , Brandon Chan
* x500: swank026 , chanx411 */

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <zconf.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include "../include/protocol.h"

#define _BSD_SOURCE

// Create lists azList and updateStatus
int azList[ALPHABETSIZE];
int updateStatus[MAX_STATUS_TABLE_LINES][3];

// current pthread
int currentConn;

// no locking needed
pthread_mutex_t currentConn_lock;

struct threadArg {
	int clientfd;
	char* clientip;
	int clientport;
};

void * threadFunction(void * arg) {

	while(1) {

		// locks when servicing a request
		pthread_mutex_lock(&currentConn_lock);
		struct threadArg * tArg = (struct threadArg *) arg;

	  int readBuf[REQUEST_MSG_SIZE];

		// init readBuf to zeros
	  for(int i = 0; i < LONG_RESPONSE_MSG_SIZE; i++){ readBuf[i] = 0; }

	  // Recieve the req information
	  recv(tArg->clientfd, readBuf, sizeof(readBuf), 0);

	  // break up request into mapperID and request type
	  int mapperID = readBuf[0];
	  int reqType = readBuf[1];
	  int response[LONG_RESPONSE_MSG_SIZE];

		// init response to zeros
	  for(int i = 0; i < LONG_RESPONSE_MSG_SIZE; i++){ response[i] = 0; }

		// Check if valid mapperID
		if(mapperID < 1 || mapperID >= MAX_STATUS_TABLE_LINES) {
			printf("Invalid Mapper ID %d\n",mapperID);
			response[0] = mapperID;
			response[1] = RSP_NOK;
			send(tArg->clientfd, response, sizeof(response), 0);
			pthread_mutex_unlock(&currentConn_lock);
			return NULL;
		}

		// response to CHECKIN
		if(reqType == 1){

			// Check if it is already checked in
			if(updateStatus[mapperID-1][2] == 1) {
				printf("[%d] attempting to CHECKIN when already CHECKED IN\n",mapperID);
				response[0] = mapperID;
				response[1] = RSP_NOK;
				send(tArg->clientfd, response, sizeof(response), 0);
				pthread_mutex_unlock(&currentConn_lock);
				return NULL;
			}

			printf("[%d] CHECKIN\n", mapperID);

			// updateStatus
			updateStatus[mapperID-1][0] = mapperID;
			updateStatus[mapperID-1][1] = 0;				// init number of flags = 0
			updateStatus[mapperID-1][2] = 1;				// checkin = 1

			// response
			response[0] = mapperID;
			response[1] = RSP_OK;
		}

		// response to UPDATE_AZLIST
		if(reqType == 2){

			// updateStatus
			updateStatus[mapperID-1][1]++;					// increment number of updates

			// response
			response[0] = mapperID;
			response[1] = RSP_OK;

			// add the counts from the readBuf to the azList
			for(int i = 2; i < LONG_RESPONSE_MSG_SIZE; i++){
				azList[i-2] += readBuf[i];
			}
		}

		// response to GET_AZLIST
		if(reqType == 3){

			printf("[%d] GET_AZLIST\n", mapperID);

			// response
			response[0] = mapperID;
			response[1] = RSP_OK;

			for(int i=2; i<LONG_RESPONSE_MSG_SIZE; i++) {
				response[i] = azList[i-2];
			}
		}

		// response to GET_MAPPER_UPDATES
		if(reqType == 4){
			printf("[%d] GET_MAPPER_UPDATES\n", mapperID);

			// response
			response[0] = mapperID;
			response[1] = RSP_OK;
			response[2] = updateStatus[mapperID-1][1];
		}

		// response to GET_MAPPER_UPDATES
		if(reqType == 5){

			printf("[%d] GET_ALL_UPDATES\n", mapperID);

			// response
			response[0] = mapperID;
			response[1] = RSP_OK;

			// iterates thru ever line of status table and counts the update column
			// update column init to 0s, so counting all lines is fine
			int countUpdates = 0;
			for(int i = 0; i < MAX_STATUS_TABLE_LINES; i++) {
				countUpdates += updateStatus[i][1];
			}
			response[2] = countUpdates;
		}

		// response to CHECKOUT
		if(reqType == 6){

			// Check if it is already checked out
			if(updateStatus[mapperID-1][2] == 0) {
				printf("[%d] attempting to CHECKOUT while CHECKED OUT\n",mapperID);
				response[0] = mapperID;
				response[1] = RSP_OK;
				response[2] = mapperID;
				pthread_mutex_unlock(&currentConn_lock);
				return NULL;
			}

			printf("[%d] CHECKOUT\n", mapperID);

			// updateStatus
			updateStatus[mapperID-1][2] = 0;				// checkout = 0

			// response
			response[0] = mapperID;
			response[1] = RSP_OK;
			response[2] = mapperID;
		}

	  // send response back to client
	  send(tArg->clientfd, response, sizeof(response), 0);
		currentConn--;
		pthread_mutex_unlock(&currentConn_lock);

		if(reqType == 6) {
			return NULL;
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {
		pthread_mutex_init(&currentConn_lock, NULL);
    int server_port;

    // 1 arguments
    if (argc == 2) {
        server_port = atoi(argv[1]);
    } else {
        printf("Invalid or less number of arguments provided\n");
        printf("./server <server Port>\n");
        exit(0);
    }

    // ---------- Server (Reducer) code ----------

		// init azList to zero
    for(int i = 0; i < ALPHABETSIZE; i++){
      azList[i] = 0;
    }

		// init updateStatus to zero
    for(int i = 0; i < MAX_STATUS_TABLE_LINES; i++){
      for(int j = 0; j < 3; j++){
        updateStatus[i][j] = 0;
      }
    }

    // Create pthreads
    pthread_t threads[MAX_CONCURRENT_CLIENTS];
    currentConn = 0;

    // Create a TCP Socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Bind it to a local address.
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(server_port);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

    listen(serverSocket, MAX_CONCURRENT_CLIENTS);
    printf("server is listening on port: %d\n", server_port);

    while(1){

      // Accept incoming connections
      struct sockaddr_in clientAddress;
  		socklen_t size = sizeof(struct sockaddr_in);

			// Added: TCP aspects. It looks like you took from lab 13 which is UDP, we need to connect via TCP
			int clientfd = accept(serverSocket, (struct sockaddr*) &clientAddress, &size);

			struct threadArg *arg = (struct threadArg *) malloc(sizeof(struct threadArg));

			arg->clientfd = clientfd;
			arg->clientip = inet_ntoa(clientAddress.sin_addr);
			arg->clientport = clientAddress.sin_port;

      if(currentConn == MAX_CONCURRENT_CLIENTS) {
  			printf("server is full\n");
  			close(arg->clientfd);
  			free(arg);
  			continue;
  		} else {

        // Create a pthread to handle the new connection
        printf("open connection from %s: %d\n", arg->clientip, arg->clientport);
  			pthread_create(&threads[currentConn], NULL, threadFunction, (void*) arg);
        currentConn++;
  		}
    }

    // Because the server is technically a daemeon we do not have to close the socket or join the pthreads

    return 0;
}
