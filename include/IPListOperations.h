/*
 * IPListOperations.h
 *
 *  Created on: Sep 7, 2013
 *      Author: dev
 */

#ifndef IPLISTOPERATIONS_H_
#define IPLISTOPERATIONS_H_

#include <IPList.h>

// function prototypes
int addToIPList(struct sockaddr_in address,int fd,int listenPort,IPList** tail);
IPList* removeFromIPList(char* IPAddress,int port,IPList** head,IPList** tail);
void displayIPList(IPList* head);
int freeIPList(IPList* listHead);
int getMaxFD(IPList* head);

#endif /* IPLISTOPERATIONS_H_ */
