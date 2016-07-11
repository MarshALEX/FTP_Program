/**

usage: ftserver [port number 30020 to 65535]

 Alex Marsh
 05/24/2016
 CS 372 Project 2
 written in C++
 
 Code taken from resources: 
    Linux How To: Socket Tutorial  
            http://www.linuxhowtos.org/C_C++/socket.htm
    Beej's Guide to Network Programming
            http://beej.us/guide/bgnet/output/html/multipage/index.html
    How can I get the list of files in a directory using C or C++?
            http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
 
For opening and reading files
    http://www.cplusplus.com/doc/tutorial/files/
 
 This is a simple server simulating the FTP that will except connections on the port number designated by the command line.  The server will remain open until Cntr+C is entered by the user. The serverwill accept requests from a client either '-l' (list items in a directory) or '-g' (give the client a file.)
 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fstream>


//#define MAXHOSTNAME 100 //max number of bytes
#define MAXDATASIZE 550 //max number of bytes we can get at once

void error(const char *msg);
int setUp(int argc, int portno, struct sockaddr_in serv_addr, struct sockaddr_in cli_addr);
//int getMessage(char *clientMsg);
int getCmd(char *clientMsg, int newsockfd);
int getDataPort(char *clientMsg, int newsockfd);
void sendList(struct dirent *dptr, DIR *dp, char *currDir, char* clientName, int newsockfd, int dataPort);
int setUpDataPort(int portno, struct sockaddr_in cli_addr, struct hostent *he);
void sendFile(struct dirent *dptr, DIR *dp, char *currDir, char* clientName, int controlsockfd, int datasockfd, int dataPort, char *clientMsg, int portno);

using namespace std;

int main(int argc, char *argv[])
{
    
    
    int sockfd, newsockfd, datasockfd;
    int pid, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char hostName[100];
    char clientName[100];
    int bytes_recv;
    int list = 0;
    int get = 1;
    
    char buffer[MAXDATASIZE];
    char clientMsg[MAXDATASIZE];
    char currDir[MAXDATASIZE];
    char *reqFileName;
    struct dirent *dptr = NULL;
    DIR *dp = NULL;
    struct hostent *he; 
    
    //get current working directory
    getcwd(currDir, MAXDATASIZE);
    portno = atoi(argv[1]); //get port number server will use
    
    sockfd = setUp(argc, portno, serv_addr, cli_addr); //set up connection and listening
    clilen = sizeof(cli_addr); //get size (# of bytes) of client struct
    
    //get servers host name
    if(gethostname(hostName, sizeof hostName) == -1){
     error("ERROR with obtaining hostname");   
    }
    //printf("Server open on %s\n", hostName);

    
    while(1){
        signal(SIGCHLD, SIG_IGN); //ignore the SIGCHLD signal

        //connect to pending connection and make a new socket fd for this connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); 
        
        if (newsockfd < 0) //error check accept
            error("ERROR on accept");
        pid = fork();
        if(pid < 0)
            error("ERROR on fork");
        else if(pid == 0){//in the child process
          //close socket so it may be open 
    
          memset(clientName, 0, MAXDATASIZE); //set clientName to all 0
          //get client hostname 
          if(getnameinfo((struct sockaddr *) &cli_addr, clilen, clientName, sizeof(clientName), hostName, sizeof(hostName), NI_NAMEREQD) != 0)
            error("ERROR with obtaining getnameinfo");
        
          printf("Connection from  %s.\n", clientName);    
          
          //get client/dataport host info
          if((he=gethostbyname(clientName)) == NULL){
            error("ERROR getting host info");   
          }
            
         
          //for another connection    
          close(sockfd);   
        
          //recieve message and erro check
          if((bytes_recv = recv(newsockfd, clientMsg, MAXDATASIZE-1, 0)) <= 0){
                if(bytes_recv == 0){ //client closed the connection
                    //client closed the chat
                    printf("There is NO client here.\n");
                }
                else  //there is an error connecting
                    error("ERROR recieving from socket\n");
            }
            else{  //get data from client
                
                int req = getCmd(clientMsg, newsockfd); //get command from user
                int dataPort = getDataPort(clientMsg, newsockfd); //get data port from user
                //set up dataPort to send dir or files over 
                datasockfd = setUpDataPort(dataPort, cli_addr, he); 
                
                if(req == list){
                    sendList(dptr, dp, currDir, clientName, datasockfd, dataPort);  
                }
                
                if(req == get){
                    /**if(send(datasockfd, "You wanted a get!\n", 19, 0) == -1)
                        error("ERROR in server sending");**/
                    sendFile(dptr, dp, currDir, clientName, newsockfd, datasockfd, dataPort, clientMsg, portno);
                }
                printf("In server waiting...listening...watching...\n");
                exit(0);
            }
        }
        else //we are in the parent process
            close(newsockfd);
       
    }
    close(sockfd);
    return 0; //should never reach here
}

    
/*
*           setUp
* Description: This function sets up an open socket to accept clients
* Input: the number of arguments from the command line, the array holding the arguments
            from teh command line, the two struct sockaddr_in objects for the server and client 
* Output: The integer of the open socket. 
*/

int setUp(int argc, int portno, struct sockaddr_in serv_addr, struct sockaddr_in cli_addr){
    int sockfd;
    
    //check usage
     if (argc < 2) {
         printf("usage: ftserver [port number]\n");
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
    
     //create a socekt
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) //error check socket
        error("ERROR opening socket");
    
     bzero((char *) &serv_addr, sizeof(serv_addr));
    // portno = atoi(argv[1]); //get port number server will use
     serv_addr.sin_family = AF_INET; //host byte order
     serv_addr.sin_addr.s_addr = INADDR_ANY; //use my IP address
     serv_addr.sin_port = htons(portno); //short network byte order
     
    
    //associate the socket with the port number with error checking
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
    
     if((listen(sockfd,5)) == -1)//listen for someone (up to 5 people)
         error("ERROR listening"); //error check listening
     printf("Server open on %d\n", portno);
     printf("In server waiting...listening...watching...\n");
     
    return sockfd;  //return the open socket
}



/*
*           setUpDataPort
* Description: This function sets up an open socket to accept clients
* Input: the number of arguments from the command line, the array holding the arguments
            from teh command line, the two struct sockaddr_in objects for the server and client 
* Output: The integer of the open socket to send data. 
*/
int setUpDataPort(int portno, struct sockaddr_in cli_addr, struct hostent *he){
    int datasockfd; //socket to send file data over
    
    //create a socekt
    datasockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (datasockfd < 0) //error check socket
        error("ERROR opening data socket");

    bzero((char *) &cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET; //host byte order
    cli_addr.sin_addr = *((struct in_addr *) he->h_addr); //use client's IP address
    cli_addr.sin_port = htons(portno); //short network byte order 
    memset(&(cli_addr.sin_zero), '\0', 8); //zero the rest of the struct

    //connect to socket
    if(connect(datasockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr)) == -1)
        error("ERROR connecting to data socket");
    
    return datasockfd;   
}



/*
*           error
* Description: This function prints an error message
* Input: char array of message
* Output: None
*/
void error(const char *msg)
{
    perror(msg);
    exit(1);
}


/*      getCmd
*
*Description: This function parses the clients message to requested command
*Input: a char array holding the clients message
*Output: this function returns a:
*       0: if client requests a list
*        1: if a client requests a file
*/
int getCmd(char *clientMsg, int newsockfd){
    if(clientMsg[1] == 'l'){
        return 0; //for list
    }
    
    else if(clientMsg[1] == 'g'){
        return 1; //for get
    }
    
    else{
     error("ERROR invalid request from client. Niether -l or -g");
     //send error message to client
     if(send(newsockfd, "ERROR invalid command", 22, 0) == -1)
            error("ERROR in server sending invalid command error");    
    }   
}

/*      getDataPort
*
*Description: This function parses the clients message to obtain the 
*   data port 
*Input: a char array holding the clients message
*Output: this function returns the integer of the dataPort      
*/
int getDataPort(char *clientMsg, int newsockfd){
    int dataPort; //variable to hold int dataPort
    char dataPortStr[10];   
    char fileNameStr[20];
    
    if(clientMsg[1] == 'l'){
        //get dataPort from client
        int i;    
        for(i=2; i < strlen(clientMsg); i++){
            //printf("Client %c \n", clientMsg[i]);
            dataPortStr[i-2] = clientMsg[i]; //copy dataPort into string 
        }    
        
        dataPort = atoi(dataPortStr); //change dataPortStr to integer

        printf("List directory requested on port %d\n", dataPort);   

        return dataPort; //for list
    }
    
    else if(clientMsg[1] == 'g'){
        
        int x; 
        int i = 0;
        //get data port 
        for(x=2; x <= 6; x++){
            //printf("Client %c \n", clientMsg[i]);
            dataPortStr[x-2] = clientMsg[x]; //copy dataPort into string 
        }    
        //get file name from message
        for(x=7; x < strlen(clientMsg); x++){
            
            fileNameStr[i] = clientMsg[x]; //copy dataPort into string 
            i++;
        } 
        
        dataPort = atoi(dataPortStr); //change dataPortStr to integer  
        
        printf("File \"%s\" requested on port %d\n", fileNameStr, dataPort);   
     
        return dataPort; //for get
    }
    
    else{
     error("ERROR invalid request from client. Niether -l or -g");
     //send error message to client
     if(send(newsockfd, "ERROR invalid command", 22, 0) == -1)
            error("ERROR in server sending invalid command error");    
    }   
}


    
/*      sendList
*
*Description: This function sends a list of the contents in the servers directory
*Input: the parameters include a structure dirent, directory pointer, name of the current 
*       working directory, the clients server name, the control sockfd, and the dataport of
*       which to send the list over to the client.
*Output: The function does not return a value, but sends a message to the connected client 
*/
void sendList(struct dirent *dptr, DIR *dp, char *currDir, char* clientName, int newsockfd, int dataPort){
    int count = 0; //count of items in directory
    char dirCount[MAXDATASIZE];
    char* dirContents[MAXDATASIZE];
    char fileName[MAXDATASIZE];
    int fileNameLen, x;
    
    printf("Sending directory contents to %s : %d\n", clientName, dataPort);
    

    dp = opendir(currDir); //open the directory and error check 
    if(dp == NULL)
        error("ERROR opening directory");


    //read the directory contents into an array
    while(NULL != (dptr = readdir(dp)))
    {
        dirContents[count] = dptr->d_name;
        count++; 
    }
    
    
    //iterate through array and send to client
    for(x = 0; x < count; x++){
        
        strcpy(fileName, dirContents[x]);
        //fileName = dirContents[x]; //get item in dir
        fileNameLen = strlen(fileName);  //get length of item name
        //printf("[%s] ", fileName);
        //send items in directory 
        if(send(newsockfd, fileName, fileNameLen, 0) == -1)
            error("ERROR in server sending directory");
        
       
        //send new line
        if(send(newsockfd, "\n", 2, 0) == -1)
            error("ERROR in server sending newLine"); 
        
        memset(fileName, 0, MAXDATASIZE); //reset array with 0s
        
    }
    
    close(newsockfd);
    closedir(dp); //close directory
}

    
/*      sendFile
*Decription: This function sends the contents of a file over to the connected client over 
*       the data connection. If the requested does not exist in the servers working directory
*       an error message is printed to the screen and sent to the client over the control
*       connection
*Input: The parameters include a structure dirent, directory pointer, name of the current 
*       working directory, the clients server name, the control socket descriptor, the data
*       socket descriptor, the dataport to the client, the clients incoming message, and the 
*       servers port number for the control connection
*Output: the function does not return any value. 
*/      
void sendFile(struct dirent *dptr, DIR *dp, char *currDir, char* clientName, int controlsockfd, int datasockfd, int dataPort, char *clientMsg, int portno){
  
    char fileNameStr[20];
    int x; 
    int i = 0;
    bool found = false;
    int pos = 0;
    int count = 0;
    char* dirContents[MAXDATASIZE];
    FILE* textFile; //to read from files
    char textLine[1024];
    int len; 
    
    //get file name from message
    for(x=7; x < strlen(clientMsg); x++){
        fileNameStr[i] = clientMsg[x]; //copy dataPort into string 
        i++;
    } 
    
    //add null terminator to string
    fileNameStr[i] = '\0';
    
    dp = opendir(currDir); //open the directory and error check 
    if(dp == NULL)
        error("ERROR opening directory");

    //read the directory contents into an array
    while(NULL != (dptr = readdir(dp)))
    {
        //compare contents of directory with requested file name
        if(strcmp(dptr->d_name, fileNameStr) == 0){
                //if found set found flag to true
                found = true;
                //printf("found a match!\n");
                pos =  count;
        }
            
        dirContents[count] = dptr->d_name;
        count++; 
    }
    
    //if file not found, send error message to client
    if(found == false){
        printf("File not found. Sending error message to %s:%d\n", clientName, portno);
        if(send(controlsockfd, "FILE NOT FOUND", 15, 0) == -1)
            error("ERROR in server sending directory");
    }
    //if file is found send file
    else{

        //open the file
        textFile = fopen(fileNameStr, "r");
        if(!textFile)
        {
            error("ERROR: couldn't open file\n");    
        }
        
        printf("Sending: \"%s\" to %s : %d\n", fileNameStr, clientName, dataPort); 
        
        while(fgets(textLine, 1024, textFile) != NULL){
            len = strlen(textLine);
            if(send(datasockfd, textLine, len, 0) == -1)
                error("ERROR in server sending directory");    
            memset(textLine, 0, 1024);
        }
                
        //close file
        fclose(textFile);;
    }
     
    close(datasockfd);
    closedir(dp); //close directory
}