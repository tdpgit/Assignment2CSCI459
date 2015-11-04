
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include "structs.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/*global variables to be used in the threads*/
int network_layersockfd;//the socket through which the network layer is connected.
int wiresockfd; //the socket through which the wire is connected.


/*the thread function that receives frames from the wiresocket and sends packets to the network_layer */
void * rcvfromwiresend2network_layer ( char *argv[] )
{
	/*add codes to declare locals*/
    int readSize;
    frame holdFrame;
    packet sendPacket;

	 while (1)
	 {
         int socketClosed = 0;
		 /*add codes receive a frame from wire*/
         if((readSize = read(wiresockfd, &holdFrame, sizeof(frame))) < 0) {
             socketClosed = 1;
         }
         
         printf("Recieved frame from wire.\n\tSequence number: %d\n\tFrame type: %d\n\tSending the included packet to network_layer ...\n", holdFrame.seq_num, holdFrame.type);
         fflush(stdout);
			
         if (socketClosed == 1) {
             printf("Socket may be closed.");
         }
         
		/*add codes to send the included packet to the network layer*/
         sendPacket = holdFrame.my_packet;
         if((readSize = write(network_layersockfd, &sendPacket, sizeof(packet))) < 0) {
             error("Error writing to socket");
         }
		
	 }
}


int main(int argc, char *argv[])
{
	/*add codes to declear local variables*/
    int sockfd;
    struct sockaddr_in server_addr, cli_addr, wire_addr;
    socklen_t clilen;
    struct hostent *wire;
    
	/*check numeber of arguments*/
     if (argc < 4) {
		 fprintf(stderr,"Usage: %s  wire__IP  wire_port data_port\n",argv[0] );
         exit(1);
     }

	/* add codes to connect to the wire. Assign value to gobal varialbe wiresockfd */
    wire = gethostbyname(argv[1]);
    bzero((char *) &wire_addr, sizeof(wire_addr));
    wire_addr.sin_family = AF_INET;
    
    bcopy((char *)wire->h_addr,
          (char *)&wire_addr.sin_addr.s_addr,
          wire->h_length);
    
    wire_addr.sin_port = htons(atoi(argv[2]));
    
    wiresockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(wiresockfd,(struct sockaddr *) &wire_addr,sizeof(wire_addr)) < 0) {
        error("ERROR connecting");
    }
    
	/*generate a new thread to receive frames from the wire and pass packets to the network layer */
	pthread_t wirepth;	// this is our thread identifier
	pthread_create(&wirepth,NULL,rcvfromwiresend2network_layer, NULL);

	/*add codes to create and listen to a socket that the network_layer will connect to. Assign value to global variable network_layersockfd*/
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[3]));
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(sockfd, (struct sockaddr *) &server_addr,
             sizeof(server_addr)) < 0) {
        error("ERROR on binding");
    }
    listen(sockfd,1);
    
    /*add codes to receive a packet from the network layer*/
    clilen = sizeof(cli_addr);
    network_layersockfd = accept(sockfd,
                                 (struct sockaddr *) &cli_addr,
                                 &clilen);
    
    if (network_layersockfd < 0) {
        error("Failed to create socket");
    }
    
    printf("Created socket.\n");
    fflush(stdout);

	/*the main function will receive packets from the network layer and pass frames to wire*/
    int sequenceNumber = 0;
	 while (1)
	 {
         int readSize;
         packet holdPacket;
         frame sendFrame;
         
         if((readSize = read(network_layersockfd, &holdPacket, sizeof(packet))) < 0) {
             error("Error reading from socket");
         }
         
         printf("Recieved a packet from network_layer.\n");
         printf("Sending a frame to the wire ...\n");
         fflush(stdout);
         
		/* add codes to wrap the packet into a frame*/	
         sendFrame.type = 0;
         sendFrame.seq_num = sequenceNumber;
         sendFrame.my_packet = holdPacket;

		/*add codes to send the frame to the wire*/
         if((readSize = write(wiresockfd, &sendFrame, sizeof(sendFrame))) < 0) {
             error("Error writing to socket");
         }
         
         sequenceNumber = sequenceNumber + 1;
         
		/*if the message is "EXIT/n" */
		 if (strcmp (holdPacket.message, "EXIT\n")==0)
		 {
			 pthread_cancel(wirepth); //kill the child thread
			 close(wiresockfd);
			 close (network_layersockfd); //close sockets
			 return 0; //terminate the main function
		 }

	 }

}
