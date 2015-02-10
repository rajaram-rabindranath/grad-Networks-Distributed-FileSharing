/*=============================================================================
 * File Name: server.c
 * Project  : cse589_project1
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : September 7th 2013
 ============================================================================*/


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <server.h>
#include <time.h>
#include <appMacros.h>
#include <IPListOperations.h>
#include <globalVars.h>
#include <commandOperations.h>
#include <stdlib.h>
#include <arpa/inet.h>

int server_shutdown(IPList* head);
int server_terminateConnection(char** arguments,IPList** head, IPList** tail);
int sendListOfPeers(IPList* first,int numOfConnxions);

/*=============================================================================
 * Function Name: startServer
 * Function Desc:
 * Parameters   : Port number
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int startServer(int port)
{

	fd_set read_fds, write_fds, read_fdsMirror, write_fdsMirror;

	int serverSocket,newSocket;

	// structs to store my address, client addresses
	struct sockaddr_in myAddress,clientAddress;
	struct sockaddr_storage getPeerInfo;
	socklen_t clientAddrLen = sizeof(clientAddress);
	int socketReuse = 1;// APPLICATION's SOCKET_REUSE_POLICY;

	/*
	 * 1 fdMax,
	 * 2 capture # bytes read and
	 * 3 current count of the # of connections
	 */
	int fdMax = STDIN_FILENO, recvBytes,numOfConnxions = 0, selectedSocket;
	IPList *first = NULL, *last = NULL;
	char msg[MSG_MTU];


	printf("Server : running!...\n");
	printf("type \"help\" to continue....\n");


	// resetting all file descriptor sets
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&write_fdsMirror);
	FD_ZERO(&read_fdsMirror);

	/*=======================================
	 * Following code does, this:
	 * 1. Gets socket
	 * 2. Sets socket options
	 * 3. Binds the socket
	 * 4. Starts Listening on that socket
	 * 5. Uses select
	 *======================================*/


	// get socket
	serverSocket= socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(serverSocket == -1)
	{
		printf("Server:Could not create socket on own machine bye bye\n");
		return FAILURE;
	}

	fdMax = serverSocket;
	setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,&socketReuse,sizeof(int));

	myAddress.sin_addr.s_addr = INADDR_ANY;
	myAddress.sin_family = AF_INET;
	myAddress.sin_port = port;



	// bind
	if(bind(serverSocket,(struct sockaddr*)&myAddress,sizeof(myAddress))== -1)
	{
		printf("Server:Could not bind to my own socket; DAMN!\n");
		close(serverSocket); // not of any use anymore
		return FAILURE;
	}

	#if DEBUG
	printf("binding done on %d\n",port);
	#endif

	// listen for connetions
	if(listen(serverSocket,MAX_CONNECTIONS_SERVER) == -1)
	{
		printf("Server:There is some error in listening\n");
		return FAILURE;
	}

	#if DEBUG
	printf("Listening Started!");
	#endif

	FD_SET(STDIN_FILENO,&read_fds);
	FD_SET(serverSocket,&read_fds);

	while(TRUE)
	{
		read_fdsMirror = read_fds;
		write_fdsMirror = write_fds;

		// blocks until something to read or write
		if (select(fdMax+1, &read_fdsMirror,NULL, NULL, NULL) == -1)
		{
			perror("Server : Select error");
			continue;
		}

		for(selectedSocket = 0;selectedSocket<=fdMax;selectedSocket++)
		{
			if(FD_ISSET(selectedSocket,&read_fdsMirror))
			{
				// new connection + accept connections max conn threshold not reached
				if(selectedSocket ==  serverSocket)
				{
					printf("Server : Have a new Client!\n");
					memset(&clientAddress, 0, sizeof(clientAddrLen));
					if ((newSocket = accept(serverSocket,(struct sockaddr*)&clientAddress,&clientAddrLen))==-1)
					{
						printf("Server:Not able to accept connection from client\n");
					}

					// have only MAX_CONNECTIONS
					if (numOfConnxions >= MAX_CONNECTIONS_SERVER)
					{
						close(newSocket);
						printf("Server : Can accept only 4 connections, sorry\n");
						continue;
					}

					memset(msg,0,MSG_MTU);
					// receive clients listening port number
					recvBytes = recv(newSocket,msg,MSG_MTU,0);
					setsockopt(newSocket,SOL_SOCKET,SO_REUSEADDR,&socketReuse,sizeof(int));

					addToIPList(clientAddress,newSocket,atoi(msg),&last);
					numOfConnxions++;

					// construct peer list to be sent
					// send SERVER_IP_LIST to the new client
					sendListOfPeers(first,numOfConnxions);

					if(!first) first = last;

					FD_SET(newSocket,&read_fds);
					if(newSocket > fdMax) fdMax = newSocket;

					#if DEBUG
					printf("Server : Have a new client!\n");
					#endif
				}
				else if(selectedSocket == STDIN_FILENO) // read input from STDIN
				{
					memset(msg,0,MSG_MTU);
					if ((recvBytes = read(selectedSocket, msg, MSG_MTU)) <= 0)perror("Server read error:");
					else// we got some data from a client
					{
						#if DEBUG
						printf("recv bytes is =%d\n",recvBytes);
						#endif
						if(recvBytes >= COMMAND_LENTH_MINSIZE) // All valid command at least has length(including new line character) > 4
						{
							msg[recvBytes-1] = '\0';
							if(!commandArgs) free(commandArgs); // free this up

							int commandID = commandMaster(msg);
							if(commandID > 1)
							{
								int fd;
								switch(commandID)
								{
									case OPCODE_LIST:
										displayIPList(first);
										break;
									case OPCODE_TERMINATE:
										if((fd = server_terminateConnection(commandArgs,&first,&last))!= FAILURE)
										{
											FD_CLR(fd,&read_fds);
											FD_CLR(fd,&write_fds);
											fdMax = getMaxFD(first);
											numOfConnxions--;
											if(numOfConnxions == 0){last=first=NULL;}
										}
										break;
									case OPCODE_EXIT:
										server_shutdown(first);
										return SUCCESS;
										break;
									default:
										break;
								}
							}
							memset(msg,0,MSG_MTU);
						}
						else
						{
							printf("Server : got insufficient information\n");
						}
					}
				}
				else // Data from clients
				{
					if ((recvBytes = recv(selectedSocket, msg, MSG_MTU, 0)) <= 0)
					{
						if (recvBytes == 0)
						{
							printf("Server : A connection has been closed\n");
							if(getpeername(selectedSocket,(struct sockaddr*)&getPeerInfo,&clientAddrLen)<0)
							{
								perror("Server : cannot find closed connexion's IP ADDRESS\n");
								continue;
							}

							struct sockaddr_in *s = (struct sockaddr_in *)&getPeerInfo;

							char* IPAdd = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
							int targetPort = ntohs(ntohs(s->sin_port));
							inet_ntop(AF_INET, &(s->sin_addr), IPAdd, INET6_ADDRSTRLEN);

							printf("Server: a connection had been closed by IP %s\n",IPAdd);

							removeFromIPList(IPAdd,targetPort,&first,&last);

							FD_CLR(selectedSocket,&read_fds);
							close(selectedSocket);
							numOfConnxions--;

							if(numOfConnxions == 0){last=first=NULL;}
							else
							{
								// Broad cast current peer list
								fdMax = getMaxFD(first);
								sendListOfPeers(first,numOfConnxions);
								displayIPList(first);
							}
						}
						else
						{
							printf("Server : There is an error in the recv command\n");
						}
					}
					else// we got some data from a client
					{
						printf("the message from client: %s\n", msg);
						memset(msg,0,MSG_MTU); // reset read buffer
					}
				}
				break;
			}
			else if(FD_ISSET(selectedSocket,&write_fdsMirror)) // need not handle this case // selected socket belongs to write_fds set of file descriptors
			{
				printf("for some reason this is getting triggered\n");
			}
		} // End of for loop iterating thru fds
	} // End of while(TRUE) loop
	close(serverSocket);
	return SUCCESS;
}

/*=============================================================================
 * Function Name: server_shutdown
 * Function Desc: Closes all connections and exits the application
 * Parameters   : IPList* head , to close all connexions
 * Return value : SUCCESS
 ============================================================================*/
int server_shutdown(IPList* head)
{
	IPList* iterate = head;
	int connID =1;
	printf("Server : going down!\n");
	while(iterate)
	{

		if(shutdown(iterate->fileDesc,SHUT_RDWR)<0)
		{
			perror("Client : Socket shutdown error:");
			printf("Client : from connection %d",connID);
		}
		close(iterate->fileDesc);
		iterate = iterate->next;
		connID++;
	}
	return SUCCESS;
}

/*=============================================================================
 * Function Name: server_terminateConnection
 * Function Desc: Closes the connexion that the user has specified
 * Parameters   : char** arguments,IPList** head, IPList** tail
 * Return value : SOCK FD / FAILURE
 ============================================================================*/
int server_terminateConnection(char** arguments,IPList** head, IPList** tail)
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
			printf("Server : Have terminated the connection to %s(%s)\n",iterate->hostname,iterate->IPAddress);
			removeFromIPList(iterate->IPAddress,iterate->port,head,tail);
			return fd;
		}
		iterate = iterate->next;
		i++;
	}
	printf("Server: Connection ID could not found\n");
	return FAILURE;
}

/*=============================================================================
 * Function Name: sendListOfPeers
 * Function Desc: This functions compiles the list of peers and packs them
 * 					in an array of structs -- peerList and send the same to all
 * 					connexions
 * Parameters   : IPList* and size of the list
 * Return value : int
 ============================================================================*/
int sendListOfPeers(IPList* first,int size)
{
	peerList listofPeers[size];
	IPList *head = first;
	int i =0;
	while(head)
	{
		strcpy (listofPeers[i].hostName, head->hostname);
		strcpy (listofPeers[i].IPAddress, head->IPAddress);
		listofPeers[i].port = head->listenPort;
		head = head->next;
		i++;
	}

	head =first;
	while(head) // send peer list to all registered clients
	{
		send(head->fileDesc,listofPeers,sizeof(listofPeers),0);
		head = head->next;
	}
	return SUCCESS;
}
/*=============================================================================
 * End of File
 *============================================================================*/
