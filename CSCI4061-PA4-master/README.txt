
CSCI 4061 - Intro to Operating Systems
Programming Assignment 4

Team Members: Avery Swank (swank026) and Brandon Chan (chanx411)
Section: 10
Lab: Mondays 2:30 - 3:20

How To Compile Client and Server:
  Run 'gcc -o server server.c -pthread'
  Run 'gcc -o client client.c phase1.c'

How To Run Client and Server:
  1) Run './server <Server Port>' in one terminal
  2) Run './client <Folder Name> <# of Mappers> <Server IP> <Server Port>' in a separate terminal

  - All server information is output in terminal that is running ./server
  - All client information is stored in PA4_Client/src/log/log_client.c

  NOTE: The server has to be running before the clients

  Example:
    ./server 4061
    ./client ../Testcases/TestCase4 5 127.0.0.1 4061

Individual Contributions - Avery:
 - Client forking properly
 - Server pthreading properly
 - Logging all of the client information to log_client.c
 - Recieving all of the request types and processing them, updating azList and updateStatus as well as sending responses back to the clients

Individual Contributions - Brandon:
 - Client mapper parsing. Mapper text file parsing
 - Word counting
 - Client and Server TCP connections
 - Client and Server Deterministic Request Handling and Responses
 - Thread Function

Assumptions:
 Currently: the server must be restarted in order to reset azList
  - In a situation where we want to run the same test case multiple times, it will continue to add to the azList

Extra Credit:
  Our Test Case 5 managed to work for the extra credit
