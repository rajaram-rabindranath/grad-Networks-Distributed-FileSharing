/*=============================================================================
 * File Name: client.c
 * Project  : cse589_project1
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : September 7th 2013
 ============================================================================*/

#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <client.h>
#include <stdlib.h>
#include <appMacros.h>
#include <IPListOperations.h>
#include <arpa/inet.h>
#include <commandOperations.h>
#include <netdb.h>
#include <bits/local_lim.h>
#include <globalVars.h>

// file macros
#define FILENAME 25
#define downloadARGS 3
#define DOWNLOAD_MSG_LENGTH 40 // shall send this message repeatedly to peer to get next chunk
#define DOWNLOAD_CMD_SIZE 8

// functions prototypes
int client_shutdown(IPList* head);
int connectToPeer(char** arguments,IPList* head,IPList** last);
int client_terminateConnection(char** arguments,IPList** head,IPList** tail);
int registerWithServer(char** arguments, int sockFD,char* port,IPList** tail);
int download(char** arguments,IPList* peerList);
int getSocket();
void displayPeerList(peerList* list, int listSize);
int parallelDownload(char** arguments,IPList* peerList);

// global variable for easy access
char downloadRequest[DOWNLOAD_MSG_LENGTH]; // making this a global variable so that other functions in this file can also access
struct timeval startTime, endTime;
#define INCOMING_CONNECTIONS_MAX 4

/*=============================================================================
 * Function Name: startClient
 * Function Desc: Shall kick start the application in Client mode
 * 				  Shall open sockets to connect server and listen for incoming
 * 				  connections
 * Parameters   : listenPort (port on which the client to listen for incoming
 * 					connection requests)
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
int startClient(char* listenPort)
{
	// variable to maintain peer list

	char* fileData = NULL;
	char msg[MSG_MTU];

	char fileName[FILENAME]; // to hold the filename
	char* tokens[downloadARGS]; // this shall hold the tokens of request for download message
	FILE *uploadfp,*downloadfp; // pointers to file that is being uploaded and downloaded repectively

	struct sockaddr_in myAddress,peerAddress;

	socklen_t peerAddrLen = sizeof(peerAddress);
	fd_set read_fds,read_fdsMirror,write_fds,write_fdsMirror;

	int fdMax = STDIN_FILENO, numOfConnxions = 0, chunkSize = 0; // current count of the # of connections made to the server
	int clientSocket, listeningSocket,newSocket; // sockets
	int commandID; // used to store the commandID returned by command Manager

	// selectedSocket - is the socket selected by Select command
	int socketReuse = SOCKET_REUSE_POLICY,selectedSocket, recvBytes = 0;

	// resetting FD_SETS
	FD_ZERO(&read_fds);// holds FDs for downloads and Peer and IP list from server
	FD_ZERO(&read_fdsMirror);
	FD_ZERO(&write_fds); // holds FDs for file uploads
	FD_ZERO(&write_fdsMirror);

	FD_SET(STDIN_FILENO,&read_fds); // adding console in to read file descp

	IPList *head = NULL, *tail = NULL; // shall be used to effectively manage the SERVER_IP_LIST
	IPList* listofPeers = NULL; // List begotten from server shall be stored in mem pointed to by this
	int countOfPeers =0;

	printf("Client : Boot up!\n");
	printf("type \"help\" to continue....\n");

	/*======================== // FIXME
	 * Following code does:
	 * 1. Get socket
	 * 2. Set socket options
	 *========================*/

	//getMachineIP();

	// creating socket to accept connections from Peers
	listeningSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(listeningSocket == -1)
	{
		printf("Client: could not create Listening socket\n");
		return FAILURE;
	}
	else // no problems in creating socket
	{
		// desperate attempts to have socket terminates at other ends be recognized
		setsockopt(listeningSocket,SOL_SOCKET,SO_REUSEADDR,&socketReuse,sizeof(int));
		setsockopt(listeningSocket,SOL_SOCKET,SO_KEEPALIVE,&socketReuse,sizeof(int));

		myAddress.sin_addr.s_addr = INADDR_ANY;
		myAddress.sin_family = AF_INET;
		myAddress.sin_port = htons(htons(atoi(listenPort)));

		if(bind(listeningSocket,(struct sockaddr*)&myAddress,sizeof(myAddress))== -1)
		{
			printf("Client : Could not bind to my own socket; DAMN!\n");
			close(listeningSocket); // not of any use anymore
			return FAILURE;
		}

		// listen for connetions
		if(listen(listeningSocket,INCOMING_CONNECTIONS_MAX) == -1)
		{
			printf("Client:There is some error in listening\n");
			return FAILURE;
		}

		#if DEBUG
		printf("Client : Listening socket opened and bound!\n");
		#endif

		fdMax = listeningSocket;
		FD_SET(listeningSocket,&read_fds);
	}

	// creating socket to connect with server
	if((clientSocket = getSocket()) == FAILURE)
	{
		printf("Client : Could not create socket to connect to server/n");
		return FAILURE;
	}

	// Since we are going to read from / write to (upload/download respective)
	int countRead = 0, countWrite = 0;

	// SOCKET in ACTION
	while(TRUE)
	{
		FD_SET(STDIN_FILENO,&read_fds);
		FD_SET(listeningSocket,&read_fds);
		read_fdsMirror = read_fds;
		write_fdsMirror = write_fds;

		// blocks until something to read or write
		if (select(fdMax+1, &read_fdsMirror, &write_fdsMirror, NULL, NULL) == -1)
		{
			printf("Client : Select function call returned an error\n");
			return FAILURE;
			continue;
		}
		/*
		 * Some File descriptor is ready with data (or)
		 * need to find that specific FD
		 * hence looping through all FDs
		 */
		for(selectedSocket = 0;selectedSocket<=fdMax;selectedSocket++)
		{
			if(FD_ISSET(selectedSocket,&read_fdsMirror))
			{
				// accept new connections here
				if(selectedSocket == listeningSocket)
				{
					//accept new connection from peers
					if(numOfConnxions == MAX_CONNECTIONS_CLIENT)
					{
						printf("Client: I have exceeded my quota of connections cannot accept this one\n");
						continue;
					}
					memset(&peerAddress, 0, sizeof(peerAddrLen));
					if ((newSocket = accept(listeningSocket,(struct sockaddr*)&peerAddress,&peerAddrLen))==-1)
					{
						printf("Client : Could not get connection from peer\n");
					}

					printf("Client : Have a new connection from a peer\n");

					//get details and set the connection list
					addToIPList(peerAddress,newSocket,0,&tail);
					numOfConnxions++;
					FD_SET(newSocket,&read_fds); // set socket as part of reads
					fdMax = getMaxFD(head);
				}
				else if(selectedSocket ==  clientSocket) // from server
				{
					// receiving data from the server
					if ((recvBytes = recv(selectedSocket, msg, MSG_MTU,0)) <= 0)
					{
						if(recvBytes == 0) // one of the peers / server has said bye bye
						{
							printf("Client: Server has refused connexion or shutdown\n");
							numOfConnxions--;
							if(numOfConnxions <= 0) {head=tail=NULL;}
							FD_CLR(selectedSocket,&read_fds);
						}
						else // recv itself is not working
						{
							perror("Client: recv error:");
						}
					}
					else // IP List from server
					{
						if(!listofPeers) free(listofPeers);
						listofPeers = msg;
						countOfPeers = recvBytes/sizeof(peerList);
						displayPeerList(listofPeers,countOfPeers);
						listofPeers = (peerList*)malloc(sizeof(peerList)*(countOfPeers));
						memcpy(listofPeers,msg,recvBytes);
						memset(msg,0,MSG_MTU); // reset read buffer
					}
				}
				else if(selectedSocket == STDIN_FILENO) // read input from STDIN
				{
					if ((recvBytes = read(selectedSocket, msg, MSG_MTU)) <= 0)perror("Client read error:");
					else// we got some data from a client
					{
						#if DEBUG
						printf("the message from console: %s\n", msg);
						#endif
						if(recvBytes >= COMMAND_LENTH_MINSIZE) // All valid command at least has length(including new line character) > 4
						{
							msg[recvBytes-1] = '\0';

							if(!commandArgs) free(commandArgs); // free this up

							commandID = commandMaster(msg);
							if(commandID > 1)
							{
								int fd;
								switch(commandID)
								{
									case OPCODE_CONNECT:
										if(numOfConnxions >= MAX_CONNECTIONS_CLIENT)
										{
											printf("Client : Too many connections have been made already!\n");
											printf("Client : Cannot cannot carry out request!\n");
										}
										else
										{
											if((fd = connectToPeer(commandArgs,head,&tail))!=FAILURE)
											{
												FD_SET(fd,&read_fds);
												fdMax=getMaxFD(head);
												numOfConnxions++;
											}
										}
										break;
									case OPCODE_DONWLOAD:
										if(download(commandArgs,head) == SUCCESS)
										{
											strcpy(fileName,commandArgs[1]);
											chunkSize = atoi(commandArgs[2]);
											if(!downloadfp)
											{
												downloadfp = fopen(fileName, "w+");
												if(downloadfp == NULL)
												{
													printf("Client : could not open file a new file\n");
												}
											}
										}
										break;
									case OPCODE_REGISTER:
										if(registerWithServer(commandArgs,clientSocket,listenPort,&tail)==SUCCESS)
										{
											if(!head) head=tail;
											numOfConnxions++;
											if(fdMax < clientSocket) fdMax = clientSocket;
											FD_SET(clientSocket,&read_fds);
										}
										break;
									case OPCODE_LIST:
										displayIPList(head);
										break;
									case OPCODE_TERMINATE:
										if((fd = client_terminateConnection(commandArgs,&head,&tail))!= FAILURE)
										{
											FD_CLR(fd,&read_fds);
											fdMax = getMaxFD(head);
											if(fd == clientSocket) clientSocket =getSocket();
											numOfConnxions--;
											if(numOfConnxions <= 0){head=tail=NULL;}

										}
										break;
									case OPCODE_EXIT:
										client_shutdown(head);
										return SUCCESS;
										break;
									default:
										break;
								}
							}
							memset(msg,0,MSG_MTU); // reset buffer
						}
						else // command was not good enough
						{
							printf("Client : got insufficient information\n");
						}
						// no specifications as to how to handle this
					}
				}
				else // download data from peers
				{
					if ((recvBytes = recv(selectedSocket, msg, MSG_MTU, 0)) <= 0)
					{
						if (recvBytes == 0)
						{
							printf("Client: a peer connection had been closed\n");

							memset(&peerAddress,0,peerAddrLen);
							if(getpeername(selectedSocket,(struct sockaddr*)&peerAddress,&peerAddrLen)<0)
							{
								perror("Client : cannot find closed connexion's IP ADDRESS\n");
								continue;
							}

							// to get details of peer who closed the connection
							struct sockaddr_in *s = (struct sockaddr_in *)&peerAddress;
							char* IPAdd = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
							int targetPort = ntohs(ntohs(s->sin_port));
							inet_ntop(AF_INET, &(s->sin_addr), IPAdd, INET6_ADDRSTRLEN);

							printf("Client: Details of the closed connection IP %s\n",IPAdd);

							removeFromIPList(IPAdd,targetPort,&head,&tail);
							numOfConnxions--;

							if(numOfConnxions <= 0){head=tail=NULL;}

							FD_CLR(selectedSocket, &read_fds);
							close(selectedSocket);
							//communicate to others the closing of connection
						}
						else
						{
							printf("Client : There is an error in the recv command\n");
						}
					}
					else// we got some data from a peer
					{
						// Tokenize the message to check if it is a relay
						// of a Download command
						char command[DOWNLOAD_CMD_SIZE];
						memcpy(command,msg,DOWNLOAD_CMD_SIZE);
						if(!(strcmp(command,"DOWNLOAD")) || (!(strcmp(command,"download"))))
						{
							// for tokenizing a counter and an array
							int tokenCount = 0;
							tokens[tokenCount] = strtok(msg," ");

							// get all arguments given & only process 2 args anything more than that discard
							while(tokens[tokenCount] && tokenCount+1 < 3)
							{
								tokens[++tokenCount]=strtok(NULL," ");
							}
							strcpy(fileName,tokens[1]);

							chunkSize = atoi(tokens[2]);

							// first upload
							if(uploadfp == NULL) // been asked to download different file
							{
								uploadfp = fopen(fileName,"r");
								if(!uploadfp)
								{
									printf("the requested file does not exist :%s\n",fileName);
									continue;
								}
							}

							fileData = (char*) malloc(sizeof(char*)*chunkSize);
							countRead = fread(fileData,sizeof(char),chunkSize, uploadfp);

							#if DEBUG
							printf("Client : Number of bytes read =%d",countRead);
							#endif

							if(feof(uploadfp)!=0)
							{
								printf("Client : Sending the last chunk of file %s and Chunk Size =%d\n",fileName,countRead);
								printf("Client : file upload will now stop\n");
								printf("Client : Shall continue to maintain connection\n");
								fclose(uploadfp);
								uploadfp = NULL;
								if (countRead == 0) countRead =1; // when chunk size divides file size exactly
							}
							send(selectedSocket,fileData,countRead,0);
							free(fileData);
						}
						else // The message is a response to Download request
						{
							// Writing download data to file !!
							countWrite = fwrite(msg,sizeof(char),recvBytes,downloadfp);
							fflush(downloadfp);

							#if DEBUG
							printf("the number of bytes written=%d chunkSize=%d",countWrite,chunkSize);
							#endif

							if(countWrite < chunkSize)
							{
								printf("Client : Have finished downloading the file %s\n",fileName);
							    gettimeofday (&endTime, NULL);
							    printf("Time taken to download file: %ld microseconds\n",((endTime.tv_sec - startTime.tv_sec)*1000000L
							    							           +endTime.tv_usec) - startTime.tv_usec);


								fclose(downloadfp);
								downloadfp =  NULL;
							}
							else
							{
								send(selectedSocket,downloadRequest,DOWNLOAD_MSG_LENGTH,0);
							}
						}
						memset(msg,0,MSG_MTU); // reset read buffer
					}
				}
				break; // FIXME -- may have to remove this
			}
			else if(FD_ISSET(selectedSocket,&write_fdsMirror))
			{
				break; // FIXME -- may have to remove this
			}
		}// End of for loop iterating thru all the file descriptors
	} // End of while(TRUE) loop	#if DEBUG
	return SUCCESS;
}

/*=============================================================================
 * Function Name: client_shutdown
 * Function Desc: Shuts down all the connections and closes all file descriptors
 * 				  prepares the application for shutdown
 * Parameters   : IPList* head
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int client_shutdown(IPList* head)
{
	IPList* iterate = head;
	printf("Client : going down!\n");
	while(iterate)
	{
		if(shutdown(iterate->fileDesc,SHUT_RDWR)<0)
		{
			perror("Client : Socket shutdown error:");
		}
		close(iterate->fileDesc);
		iterate = iterate->next;
	}
	return SUCCESS;
}

/*=============================================================================
 * Function Name: getSocket
 * Function Desc: Creates a new TCP socket and sends the file descriptor to
 * 					the caller
 * Parameters   : void
 * Return value : FAILURE / SOCKFD
 ============================================================================*/
int getSocket()
{
	int newSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(newSocket == -1)
	{
		printf("Client: could not create socket\n");
		return FAILURE;
	}
	return newSocket;
}

/*=============================================================================
 * Function Name: connectToPeer
 * Function Desc: connects to a PEER whose details are persent in
 * 				  the parameter "arguments". And once connected
 * 				  adds the peer connection to the peer list
 * Parameters   : char** arguments and IPList** peerList
 * Return value : FAILURE / SOCKFD
 ============================================================================*/
int connectToPeer(char** arguments,IPList* head,IPList** last)
{
	// checking if we have the right number of arguments
	int i =0;
	for(i=0;i<3;i++)
	{
		if(arguments[i] == NULL)
		{
			printf("Client : The arguments given are not sufficient\n");
			return FAILURE;
		}
	}

	char* peerIP = arguments[1];
	int peerPort = atoi(arguments[2]);


	/*
	 * Check if:
	 * Connected to Server
	 * Request is to connect to Server
	 * Request is to connect to self
	 * Invalidate the requests and return FAILURE
	 */
	if(!head)
	{
		printf("Client : Need to connect to the server and then initiate connections to peer\n");
		return FAILURE;
	}
	else if(!strcmp(peerIP,head->IPAddress))
	{
		printf("Client : request to connect to SERVER! this is an invalid request!\n");
		return FAILURE;
	}
	else if(!strcmp(peerIP,myIP))
	{
		printf("Client: Request to connect to self! an invalid request\n");
		return FAILURE;
	}

	// check if request is for a duplicate connection : invalidate request
	while(head)
	{
		if(!strcmp(arguments[1],head->IPAddress))
		{
			printf("Client : Connection already exist! Discarding the request\n");
			return FAILURE;
		}
		head = head->next;
	}

	// this a valid request
	struct in_addr peerIP_struct;
	struct sockaddr_in peerDetails;
	int sockFD;

	inet_aton(peerIP,&peerIP_struct);
	peerDetails.sin_addr = peerIP_struct;
	peerDetails.sin_family = AF_INET;
	peerDetails.sin_port = htons(htons(peerPort));

	 if((sockFD = getSocket()) == FAILURE)
	 {
		 printf("Client : Could not get a socket to connect to peer - please try again!");
		 return FAILURE;
	 }

	if(connect(sockFD,(struct sockaddr*)&peerDetails,sizeof(peerDetails))<0)
	{
		perror("Client : could not connect to server");
		close(sockFD);
		return FAILURE;
	}

	printf("Client : Made a connection to IPAddress %s",peerIP);

	addToIPList(peerDetails,sockFD,0,last);

	return sockFD;
}

/*=============================================================================
 * Function Name: client_terminateConnection
 * Function Desc: Terminates the chosen connection. Function receives the
 * 					details of the connection to be terminated in "arguments"
 * 					and post connection termination updates the peer list
 * Parameters   : char** arguments and IPList* peerList
 * Return value : Socket FD of the connection that has been terminated
 ============================================================================*/
int client_terminateConnection(char** arguments,IPList** head,IPList** tail)
{
	// checking if the right number of arguments have been given
	int i =0;
	for(;i<2;i++)
	{
		if(arguments[i] == NULL)
		{
			printf("Client : The arguments given are not sufficient\n");
			return FAILURE;
		}
	}
	i = 1;

	int fd = 0;
	int connID = atoi(arguments[1]);
	IPList* iterate = *head;
	while(iterate)
	{
		if(i == connID)
		{
			if(shutdown(iterate->fileDesc,SHUT_RDWR)<0)
			{
				perror("Client : Socket shutdown error:");
				return FAILURE;
			}
			close(iterate->fileDesc);
			fd = iterate->fileDesc;
			printf("Client : Have terminated the connection to %s(%s)\n",iterate->hostname,iterate->IPAddress);
			removeFromIPList(iterate->IPAddress,iterate->port,head,tail);
			return fd;
		}
		iterate = iterate->next;
		i++;
	}
	printf("Client: Connection ID could not found\n");
	return FAILURE;
}

/*=============================================================================
 * Function Name: registerWithServer
 * Function Desc: Registers with the SERVER, using argument provided by the
 * 					user
 * Parameters   : arguments for command,int Socket File descriptor,
 * 					listeningPort for this client, IPList
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int registerWithServer(char** arguments, int sockFD,char* listeningPort,IPList** tail)
{
	// checking if we have the right number of arguments
	int i =0;
	for(i=0;i<3;i++)
	{
		if(arguments[i] == NULL)
		{
			printf("Client : The arguments given are not sufficient\n");
			return FAILURE;
		}
	}

	char* serverIP = arguments[1];
	int serverPort = atoi(arguments[2]);
	struct in_addr sIP;
	struct sockaddr_in serverDetails;
	// need to check if IPAddress is valid or not

	// setting destination details
	inet_aton(serverIP,&sIP);
	serverDetails.sin_addr = sIP;
	serverDetails.sin_family = AF_INET;
	serverDetails.sin_port = htons(htons(serverPort));

	if(connect(sockFD,(struct sockaddr*)&serverDetails,sizeof(serverDetails))<0)
	{
		perror("Client : could not connect to server");
		return FAILURE;
	}

	// send listening socket information to the server
	send(sockFD,listeningPort,10,0);
	addToIPList(serverDetails,sockFD,0,tail);
	return SUCCESS;
}
/*=============================================================================
 * Function Name: download
 * Function Desc: Downloads file from one peer
 * Parameters   : Command arguments given by the user, IPList of peers
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int download(char** arguments,IPList* peerList)
{

	// checking if we have the right number of arguments
	int i =0;
	for(i=0;i<3;i++)
	{
		if(arguments[i] == NULL)
		{
			printf("Client : The arguments given are not sufficient\n");
			return FAILURE;
		}
	}

	IPList* peer = peerList->next;

	// checking if we have peers
	if(!peer)
	{
		printf("Client : there are no peers to do the download!");
		return FAILURE;
	}


	char downloadMSG[DOWNLOAD_MSG_LENGTH];

	#if DEBUG
	for(i=0;i<3;i++)
	{
		printf("%s at %d\n",arguments[i],i);
	}
	#endif


	// reconstructing user request to be relayed to peer
	memset(downloadMSG,0,DOWNLOAD_MSG_LENGTH);
	strcpy(downloadMSG,arguments[0]);
	strcat(downloadMSG, " ");
	strcat(downloadMSG,arguments[1]);
	strcat(downloadMSG, " ");
	strcat(downloadMSG,arguments[2]);

	strcpy(downloadRequest,downloadMSG);

	// sending request for download to first connected Peer
	gettimeofday (&startTime, NULL);
	send(peer->fileDesc,downloadMSG,DOWNLOAD_MSG_LENGTH,0);

	return SUCCESS;
}
/*=============================================================================
 * Function Name: parallelDownload
 * Function Desc:
 * Parameters   :
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int parallelDownload(char** arguments,IPList* peerList)
{
	return SUCCESS;
	IPList* peer = peerList->next;
	int messageSize = 80;
	char msg[messageSize];
	int i =0;

	if(!peer) printf("Client: can't download have not connected to any peer\n");

	#if DEBUG
	for(i=0;i<3;i++)
	{
		printf("%s at %d\n",arguments[i],i);
	}
	#endif

	for(;i<3;i++)
	{
		if(arguments[i] == NULL){break;}
	}
	if(i<2)
	{
		printf("Client : The arguments given are not sufficient\n");
		return FAILURE;
	}
	memset(msg,0,messageSize);
	strcpy(msg,arguments[0]);
	strcat(msg, " ");
	strcat(msg,arguments[1]);
	strcat(msg, " ");
	strcat(msg,arguments[2]);

	while(peer)
	{
		send(peer->fileDesc,msg,messageSize,0);
		peer = peer->next;
	}
	return SUCCESS;
}

/*=============================================================================
 * Function Name: displayPeerList
 * Function Desc:
 * Parameters   :
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
void displayPeerList(peerList* list,int listSize)
{

	int i=0;
	printf("Client : Server has sent the updated peer list\n");
	printf("Hostname\t\tIP Address\t\tPort No.\n");
	for(;i<listSize;i++)
	{
		printf("%s\t\t%s\t\t%d\n",list[i].hostName,list[i].IPAddress,list[i].port);
	}
}
/*=============================================================================
 * End of File
 *============================================================================*/
