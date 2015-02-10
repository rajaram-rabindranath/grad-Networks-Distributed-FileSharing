/*
 * IPlist.h
 *
 *  Created on: Sep 19, 2013
 *      Author: dev
 */
/*
 this file has been specially created for
 commandOperations.c's "list" function
 that list the contents of the list and since commandOperations.c
 does not need to know all the functions in IPListOperations.c
 */

#ifndef IPLIST_H_
#define IPLIST_H_

typedef struct
{
	char* hostname;//[HOST_NAME_MAX];
	char* IPAddress;// [INET6_ADDRSTRLEN];
	int port;
	int fileDesc;
	int listenPort;
	struct IPList *next;
}IPList;


typedef struct peerList
{
	char hostName[255];
	char IPAddress[255];
	int port;
}peerList;

#endif /* IPLIST_H_ */
