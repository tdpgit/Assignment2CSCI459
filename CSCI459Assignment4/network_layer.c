#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include "structs.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

/*the thread function to receive packets from the data link layer and display the messages*/
void * rcvmsg (int threadsockfd)
{
	/*add codes for local varialbes*/
    int readSize;
    packet holdPacket;

	while (1)
	{
	/*add codes to read a packet from threadsockfd and display it to the screen*/
        if((readSize = read(threadsockfd, &holdPacket, sizeof(packet)) < 0)) {
            printf("Could not read from socket may be closed\n");
            return 0;
        }
        
        printf("Receive Message: %s\n", holdPacket.message);
        printf("From machine: %s\n", holdPacket.nickname);
        fflush(stdout);
	}
	return NULL;
}


int main(int argc, char *argv[])
{
	/*add codes for local variables*/
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    int readSize;

	/*check number of aruguments*/
    if (argc < 4) {
       fprintf(stderr,"usage %s data_add data_port nickname\n", argv[0]);
       exit(0);
    }

	/*add codes to connect to the data link layer*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /* add codes to connect to the wire. Assign value to gobal varialbe wiresockfd */
    server = gethostbyname(argv[1]);
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&server_addr.sin_addr.s_addr,
          server->h_length);
    
    server_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        error("ERROR connecting");
    }
    printf("Ready to communicate.\n");

	/*creat a thread to receive packets from the data link layer*/
	pthread_t pth;	// this is our thread identifier
	pthread_create(&pth,NULL,rcvmsg,sockfd);

	/* the main function will receive messages from keyboard and send packets to the data link layer*/
    char buffer[256];
    packet sendPacket;
	while (1)
	{
        bzero(buffer, sizeof(buffer));
		/*add codes to receive a message from keyboard, wrap it into a packet and send it to the data link layer*/
        fgets(buffer, 255, stdin);
        
        strcpy(sendPacket.message, buffer);
        strcpy(sendPacket.nickname, argv[3]);
        if((readSize = write(sockfd, &sendPacket, sizeof(packet))) < 0) {
            error("Error writing to socket");
        }
        
		/*if the meassge is "EXIT"*/
		if (strcmp (buffer, "EXIT\n")==0)
		{
			pthread_cancel(pth); //kill the child thread
			close(sockfd); // close socket
			return 0;	//terminate
		}
	}

}
