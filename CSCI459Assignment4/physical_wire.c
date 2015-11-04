
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "structs.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/*Global variables that will be accessed in the thread function  */
int clientlist[2]; /*the socket numbers through with the 2 clients (e.g. data link layer) are connected to this wire*/


/*the thread function that will receive frames from socket (i.e. data_link layer) and send the received frames to another socket*/  
void * onesocket ( int threadsockfd)
{
	/*add codes to declear local variables*/
    int readSize;
    frame holdFrame;

	 while (1)
	 {
		/*add codes to receive a frame from threadsocketfd*/
         if((readSize = read(threadsockfd, &holdFrame, sizeof(frame))) < 0) {
             error("Error reading from socket");
         }
         
         printf("Recieved a frame from machine: %s\n", holdFrame.my_packet.nickname);
         fflush(stdout);

		/*if the message in the frame in EXIT close the socket and terminate this thread using*/
         if (strcmp(holdFrame.my_packet.message, "EXIT\n") == 0) {
             close(threadsockfd);
             return NULL;
         }
         
         printf("Sending it to machine on the other side ...\n");
         fflush(stdout);
         
		 /*other wise send the frame to the other socket (which is stored in clientlist)*/
         if (threadsockfd == clientlist[0]) {
             if((readSize = write(clientlist[1], &holdFrame, sizeof(frame))) < 0){
                 error("Error writing to socket");
             }
         } else {
             if((readSize = write(clientlist[0], &holdFrame, sizeof(frame))) < 0){
                 error("Error writing to socket");
             }
         }
	 }
}

int main(int argc, char *argv[])
{
	/*add codes to declear local variables*/
    int sockfd, newsockfd;
    struct sockaddr_in server_addr, cli_addr;
    socklen_t clilen;
    pthread_t threadlist[2];

	/*check the number of arguments*/
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

	/*add codes to create a socket (sockfd), bind its address to it and listen to it*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *) &server_addr,
             sizeof(server_addr)) < 0) {
        error("ERROR on binding");
    }
    listen(sockfd,2);
    
	for (int i = 0;i < 2;i = i + 1) /*only accept two requests*/
	{
		/*accept a request from the data link layer*/
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,
                           (struct sockaddr *) &cli_addr,
                           &clilen);
        
        if (newsockfd < 0) {
            error("Failed to create socket");
        }
        
        if (i == 0) {
            printf("Created first socket.\n");
        } else {
            printf("Created second socket.\n");
        }
        fflush(stdout);
        
        /* store the new socket into clientlist*/
        clientlist[i]=newsockfd;
        
        /*creat a thread to take care of the new connection*/
        pthread_t pth;	/* this is the thread identifier*/
        pthread_create(&pth,NULL,onesocket,clientlist[i]);
        threadlist[i]=pth; /*save the thread identifier into an array*/ 
	}
	close(sockfd); /*so that wire will not accept further connection request*/
	pthread_join(threadlist[0],NULL);
	pthread_join(threadlist[1],NULL); /* the main function will not terminated untill both threads finished*/
	return 0;
}
