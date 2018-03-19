//Brandon Lo
//CS372 Project 1
//chatclient.c
//10/29/17
//Create a client for the server to communicate with. Cannot create sockets, but can find and bind to them
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

//looping and binding a socket taken from there
//http://beej.us/guide/bgnet/output/html/multipage/clientserver.html#simpleserver
//loops through the linked list until it finds the first possible socket that we can bind to
//then it connects to the socket and returns the socket that it was connected to
int connectSocket(struct addrinfo * servinfo){
	int sockfd, status;
	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
		printf("Unable to find socket\n");
		exit(1);
	}
	if ((status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
		printf("Unable to connect to socket\n");
		exit(1);
	}
	return sockfd;
}

//creates the chat and sends and recieves the messages while it is active
void createClient(int sockfd, char* handle, char* serverName){
	// create buffers for input and output, size is 500 since it is the limit of characters
	char input[500], output[500];
	//clear the buffers
	memset(input,0,sizeof(input));
	memset(output,0,sizeof(output));
	//set the flags
	//using fgets to get maximum input of less than 500 characters
	//https://stackoverflow.com/questions/7880141/how-do-i-check-length-of-user-input-in-c
	int checkSize = 0, status;
	fgets(input, 500, stdin);
	//while \quit is not called keep sending and recieving messages
	while(1){
		printf("%s> ", handle);
		fgets(input, 500, stdin);
		//use strcmp to compare the input to check if \quit was used
		//needs extra \ before \quit in order to escape
		//https://www.gnu.org/software/libc/manual/html_node/String_002fArray-Comparison.html
		if (strcmp(input, "\\quit\n") == 0)
			break;

		//send the input buffer with the size based on the characters inputted
		//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendrecv
		//http://www.scottklement.com/rpg/socktut/sendrecvapi.html
		//https://linux.die.net/man/2/send
		checkSize = send(sockfd, input, strlen(input) ,0);
		if(checkSize == -1){
				printf("Error when sending data to host\n");
				exit(1);
		}

		//recieve the output buffer with a sending length of 500
		//https://linux.die.net/man/2/recv
		//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendrecv
		status = recv(sockfd, output, 500, 0);
		//if recv means it has an error
		if (status == -1){
			printf("Error when receiving data from host\n");
			exit(1);
		}

		//if recv fails and returns 0 that means server has closed on you
		//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendrecv
		else if (status == 0){
			printf("Connection closed by server\n");
			break;
		}

		//if the client correctly recieves the message, then display it with the format
		else{
			printf("%s> %s\n", serverName, output);
		}

		//clear the buffer
		memset(input,0,sizeof(input));
		memset(output,0,sizeof(output));
	}
	//connection is closed here and a message is printed. Additional message if server closes the connection on you
	close(sockfd);
	printf("Closed Connection\n");
}

//exchange info between the server
//sends the handle name over to the server
//recieves the serverName from the server
//https://linux.die.net/man/3/send
//http://www.gnu.org/software/libc/manual/html_node/Receiving-Data.html#Receiving-Data
//how to use strlen
//https://stackoverflow.com/questions/18649547/how-to-find-the-length-of-argv-in-c
void createHandshake(int sockfd, char* handle, char* serverName){
	int sendMessage = send(sockfd, handle, strlen(handle), 0),
			recvMessage = recv(sockfd, serverName, 11, 0);
}

//http://pubs.opengroup.org/onlinepubs/7908799/xns/syssocket.h.html
int main(int argc, char* argv[]){
	//initialize the variables
	//the character arrays are used to hold the input
	//size of 11 since it is null terminator + 10 name max size
	char handle[10], serverName[10], address[255], port[255];
	//initialize variables needed for finding and connecting to the socket from getaddrinfo
	struct addrinfo* servinfo, hints;
	int sockfd, rv;

	//check if the proper number of arguements grabbed
	if(argc != 3){
		printf("Invalid number of arguments\n");
		exit(1);
	}

	//get the handle from the user
	//%10s is used to truncate the length of the username if it is larger than 10 characters
	//https://stackoverflow.com/questions/7880141/how-do-i-check-length-of-user-input-in-c
	printf("Please enter a handle:");
	scanf("%10s", handle);

	//converts the hostname to a linked list containing an address and port
	//https://msdn.microsoft.com/en-us/library/windows/desktop/ms738520(v=vs.85).aspx
	//http://beej.us/guide/bgnet/output/html/multipage/clientserver.html#simpleserver
	strcpy(address,argv[1]);
	strcpy(port,argv[2]);
	//Setup the hints address info structure which is used by getaddrinfo()
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	//lets us know if the IP/port entered is incorrect
	//if the rv status is not 0 then something went wrong with getting the info
	if((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0){
		fprintf(stderr,"getaddrinfo error: %s\n", gai_strerror(rv));
		exit(1);
	}

	//find and connect to the server/port using connectSocket
	sockfd = connectSocket(servinfo);
	//get the servername and send the handle over to the server
	createHandshake(sockfd, handle, serverName);
	//create the chat client and then free the addrinfo after
	createClient(sockfd, handle, serverName);
	freeaddrinfo(servinfo);
}
