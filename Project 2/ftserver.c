//Brandon Lo
//CS372 Project 2
//ftserver.c
//11/26/17
//Create a server for clients to connect to and send files/list what is in the directory

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

//global variable to keep track of the number of proceses started
int numChild = 0;
//pauses the PID to allow child processes to continue before parent finishes work
//https://stackoverflow.com/questions/19461744/make-parent-wait-for-all-child-processes
//https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
static void pausePID(int sig){
	while (waitpid(-1, NULL, WNOHANG) > 0);
	numChild--;
}

//function that converts hostname to IP since you can use flip1 etc... as input.
//using hostent to get host by name
//http://man7.org/linux/man-pages/man3/gethostbyname.3.html
int convertHost(char *hostname , char *ip){
    struct hostent *currentHost;
    struct in_addr **addressList;
    int i = 0;

		//using gethostbyname to get host by name
		//https://stackoverflow.com/questions/5444197/converting-host-to-ip-by-sockaddr-in-gethostname-etc
    if ((currentHost = gethostbyname(hostname)) == NULL){
        herror("gethostbyname");
        return 1;
    }

    addressList = (struct in_addr **) currentHost->h_addr_list;

		//grab the IP form the network struct
		//using inet_ntoa to convert string to IP
    // https://stackoverflow.com/questions/5328070/how-to-convert-string-to-ip-address-and-vice-versa
    for(i = 0; addressList[i] != NULL; i++){
        strcpy(ip ,inet_ntoa(*addressList[i]));
        return 0;
    }
    return 1;
}

//get the files in the directory
//https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
void getDir(char *dirList){
  char hostName[50];
  gethostname(hostName, 50);
  strcat(dirList, hostName);
  strcat(dirList, ",");
	DIR *currDir;
	//how to use direent and loop through and find the directories
	//https://en.wikibooks.org/wiki/C_Programming/dirent.h
  struct dirent *entry;
	int index;
	//use of opendir in order to get the directory stream
	//http://man7.org/linux/man-pages/man3/opendir.3.html
  currDir = opendir(".");
  if (currDir){
  	int currDirLen = 0, currDirListLen = 0;
		//while loop that loops through the directory and adds them to the list
    while ((entry = readdir(currDir)) != NULL){
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
        	currDirLen = strnlen(entry->d_name, NAME_MAX);
		  		currDirListLen = strnlen(dirList, 1024);
					//if the current directory name is too large(garbage values inserted as name)
		  		if ((currDirListLen + currDirLen) > 1024)
		  			break;
		  		//add the current file to the dirList and then seperate with ,
					strcat(dirList, entry->d_name);
					strcat(dirList, ",");
		   		}
  		}
	//close the directory after opening the stream
  closedir(currDir);
	}
	//removes the last comma of the list or else it would end as [1,2,3,4,] instead of properly [1,2,3,4]
	index = strnlen(dirList, 1024) - 1;
	dirList[index] = 0;
}

//send the directory list to the client
void postDir(int sockfd){
	char dirList[1024];
	int sendSize = 1024;
	//make sure that there is no garbage values in the dirList so use bzero to clear it
	//https://www.mkssoftware.com/docs/man3/bzero.3.asp
	bzero(dirList, 1024);
	getDir(dirList);
	//use send() to check if the file was sent. If the value doesn't match the len then it did not sent.
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendrecv
	if (send(sockfd, dirList, sendSize, 0) < 0)
		printf("Error: Failed to send directory.\n");
}

//outputs the reponse depending on the client command recieved
void outputResponse(char *command, char *fileName, int port){
	if (strcmp(command, "-l") == 0)
		printf("List directory requested on port %d.\n", port);
	if (strcmp(command, "-g") == 0)
		printf("File \"%s\" requested on port %d.\n", fileName, port);
}

//outputs the reponse depending on the status of the file being sent
void outputFResponse(int socket, char *response){
	char buffer[2];
	//make sure that there is no garbage values in the buffer so use bzero to clear it
	//https://www.mkssoftware.com/docs/man3/bzero.3.asp
	bzero(buffer, 2);
	strncpy(buffer, response, 1);

	//use send() to check if the file was sent. If the value doesn't match the len then it did not sent.
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendrecv
	if (send(socket, buffer, 1, 0) < 0){
		printf("ERROR: Failed to send client the following message: %s", response);
		exit(1);
	}
}

//grabs the commands forom the client program
void getCommands(int socket, char *commands, char *fileName, int *dataPort, char *clientHostName){
	char buffer[50];
	char dataBuffer[50];
	char currChar = ' ';
	int curSum, index, i = 0;

	//make sure that there is no garbage values in the buffer so use bzero to clear it
	//https://www.mkssoftware.com/docs/man3/bzero.3.asp
	bzero(buffer, 50);
	recv(socket, buffer, 1024, 0);
	//find the comma in the string and thend copy it to the buffer
	curSum = GetNumCommas(buffer, 50);
	index = findComma(buffer, 50, 1);
	strncpy(commands, buffer, index);
	//once the comma is found then make sure the buffer is clear and then find the port
	bzero(dataBuffer, 50);
	strncpy(dataBuffer, buffer + index + 1, 50);
	//convert the port from a string to an int
	//https://www.tutorialspoint.com/c_standard_library/c_function_atoi.htm
	(*dataPort) = atoi(dataBuffer);
	//check if the -l command or -g command is used
	if (curSum == 2){
		int index = findComma(buffer, 50, 2);
		strncpy(clientHostName, buffer + index + 1, 50);
	}
	//grabs file name if 3 parameters are ;passed
	if (curSum == 3){
		int start, end;
		//looks through the buffer list to get the filename
		start = findComma(buffer, 50, 2);
		end = findComma(buffer, 50, 3);
		//copy the filename to the buffer
		strncpy(fileName, buffer + start + 1, end - start - 1);
		strncpy(clientHostName, buffer + end + 1, 50);
	}
	//takes out the newline character from the hostname
	while(currChar != 0){
		if (i == 50)
			break;
		currChar = clientHostName[i];
		if (currChar == '\n'){
			clientHostName[i] = 0;
			break;
		}
		i++;
	}
}

//iterates through the char list in order to find the comma indexes
int findComma(char *list, int strLen, int target){
	int i = 0, index = -1, curSum = 0;
	char currChar = ' ';
	//while loop to iterate through the list
	while(currChar != '\n'){
		currChar = list[i++];
		if (currChar == ',')
			curSum++;
		//-1 to index since lists start from an index of 0
		//breaks from the while loop if target is found
		if (curSum == target){
			index = i - 1;
			break;
		}
		//error check if the current Character exceeds the list
		if (i >= strLen)
			break;
	}
	return index;
}

//iterates through the char list in order to find the number of commas
int GetNumCommas(char *list, int strLen){
	int i = 0,curSum = 0;
	char currChar = ' ';
	//while loop to iterate through the list
	while(currChar != '\n'){
		currChar = list[i++];
		if (currChar == ',')
			curSum++;
		//breaks from loop if it exceeds list length instead of a target
		if (i >= strLen)
			break;
	}
	return curSum;
}

//sends the file to the client program
void postFile(int socket, int pointer){
	char sendBuffer[1024];
	int readSize;
	//if the pointer points to the file, if it is empty then return an error
	if (pointer == 0){
		fprintf(stderr, "ERROR: File not found.");
		exit(1);
	}
	//make sure that there is no garbage values in the buffer so use bzero to clear it
	//https://www.mkssoftware.com/docs/man3/bzero.3.asp
	bzero(sendBuffer, 1024);
	while ((readSize = read(pointer, sendBuffer, 1024)) > 0){
		//check to make sure that the file was sent correctly
		if (send(socket, sendBuffer, readSize, 0) < 0){
			fprintf(stderr, "ERROR: Failed to send file.");
			exit(1);
		}
		//clear the buffer before the next file is sent
		bzero(sendBuffer, 1024);
	}
}

//sends the requested file to the server
void SendFileToServer(int sockfd, char *fileName){
	//use of fileno to send files through the server
	//https://stackoverflow.com/questions/11952898/c-send-and-receive-file
	FILE * transferFilePointer;
	transferFilePointer = fopen(fileName, "r+");
	int fd = fileno(transferFilePointer);
	postFile(sockfd, fd);
	fclose(transferFilePointer);
}

//gets the requests and forks the processes and starts up the sockets
void getRequest(int commandSocket, char *currentHost){
	int dataPort = -1;
	char commands[50];
	char fileName[50];
	char clientHostName[50];
	char IP[100];
	int dataSocket;
	struct sockaddr_in remote_addr;
	//make sure that there is no garbage values in the buffer so use bzero to clear it
	//https://www.mkssoftware.com/docs/man3/bzero.3.asp
	bzero(commands, 50);
	bzero(fileName, 50);
	bzero(clientHostName, 50);
	//grabs the commands and outputs the reponses
	getCommands(commandSocket, commands, fileName, &dataPort, clientHostName);
	outputResponse(commands, fileName, dataPort);
	close(commandSocket);
	//converts the hostname to IP depending on what was entered from the client
	convertHost(currentHost, IP);

	//checks if the file descriptor was opened
	//http://beej.us/net2/bgnet.html
	if ((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("Error: Failed to obtain socket descriptor.\n");
		exit(2);
	}
	//format taken from here
	//http://beej.us/net2/bgnet.html
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(dataPort);
	//fixed errors and convert from text to binary in order to store it in a struct
	//http://man7.org/linux/man-pages/man3/inet_pton.3.html
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
	inet_pton(AF_INET, IP, &remote_addr.sin_addr);
	bzero(&(remote_addr.sin_zero), 8);

	//check if the socket could be connected to
	if (connect(dataSocket, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1){
		printf("Error: could not contact client on port %d\n", dataPort);
		exit(1);
	}
	//depending on command run the function
	//runs the list command to list what is in the directory
	if (strcmp(commands, "-l") == 0){
		printf("Sending directory contents to %s:%d.\n", clientHostName, dataPort);
		postDir(dataSocket);
	}
	//else if the client wants to get a file
	else if (strcmp(commands, "-g") == 0){
		//check if file exists, then run the function to send the file and output the response on the server
		if(access(fileName, F_OK) != -1){
			outputFResponse(dataSocket, "s");
			printf("Sending \"%s\" to %s:%d\n", fileName, currentHost, dataPort);
			SendFileToServer(dataSocket, fileName);
		}
		//else if the file does not exist on the server, then ouput the repsonse
		else{
			printf("File not found. Sending error message to %s:%d\n", currentHost, dataPort);
			outputFResponse(dataSocket, "e");
		}
	}
	//close the socket after use and exit the child process
	close(dataSocket);
	exit(0);
}

//main function
int main (int argc, char *argv[]){
	int sockfd, newsockfd, sin_size, pid;
	struct sockaddr_in addr_local;
	struct sockaddr_in addr_remote;
	struct sigaction sa;
	char currentHost[50];
	//make sure that the correct number of arguements used.
	if (argc < 2){
		printf("Usage: ftserver <port>\n");
		exit(1);
	}
	//format for initiating sockets taken from here
	//http://beej.us/guide/bgipc/output/html/multipage/signals.html
	//http://www.tutorialspoint.com/unix_sockets/socket_server_example.htm

	int portNumber = atoi(argv[1]);
	//make sure that the currentHost buffer is clear before use
	bzero(currentHost, 50);

	//get the file descriptor and make sure it is working
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		exit(1);
	}

	//once the file descriptor is grabbed, then you can fill the address struct
	//Protocol family, portnumber, local adress are grabbed and the rest is cleared
	addr_local.sin_family = AF_INET;
	//need to convert from host byte order to network byte order
	addr_local.sin_port = htons(portNumber);
	addr_local.sin_addr.s_addr = INADDR_ANY;
	bzero(&(addr_local.sin_zero), 8);

	//once the struct is correctly filled, then attempt to connect.
	//if unable to connect then output error
	if ( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 ){
		fprintf(stderr, "ERROR: Failed to bind Port. Please select another port. (errno = %d)\n", errno);
		exit(1);
	}
	//listen to the port
	if (listen(sockfd, 5) == -1){
		fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		exit(1);
	}
	//output message if successful binding
	else{
		printf ("Server open on %d\n", portNumber);
	}
	//use signal handler in order to prevent zombie children processes
	//http://www.microhowto.info/howto/reap_zombie_processes_using_a_sigchld_handler.html
	sa.sa_handler = pausePID;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}
	//while loop that runs unless there is an error
	//grabs the client connections and commands
	while (1){
		sin_size = sizeof(struct sockaddr_in);
		//grabs the socket descriptors
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1){
			fprintf(stderr, "ERROR: Obtaining new socket descriptor. (errno = %d)\n", errno);
			exit(1);
		}
		//grab host by name and convert to usable format
		//format from beej
		//http://beej.us/guide/bgnet/output/html/multipage/gethostbynameman.html
		else{
			struct hostent *he;
			struct in_addr ipv4addr;
			inet_pton(AF_INET, inet_ntoa(addr_remote.sin_addr), &ipv4addr);
			he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
			strncpy(currentHost, he->h_name, 50);
			printf("Connection from %s\n", currentHost);
		}
		//create a child process and fork
		numChild++;
		pid = fork();
		if (pid < 0){
			perror("ERROR on fork");
			exit(1);
		}
		//if the pid is 0 then it is the client process
		if (pid == 0){
			close(sockfd);
			getRequest(newsockfd, currentHost);
			exit(0);
		}
		//close the child once done
		else{
			close(newsockfd);
		}
	}
}
