/*=============================================================================
 * File Name: IPListOperations.c
 * Project  : cse589_project1
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : September 7th 2013
 ============================================================================*/

#include <error.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <globalVars.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <IPList.h>
#include <bits/local_lim.h>
#include <netdb.h>
#include <appMacros.h>
/*=============================================================================
 * Function Name: addToIPList
 * Function Desc: Adds a new IP entity to the IPList and returns the ptr of the
 * 				  new entity to the calling function
 * Parameters   : char* IPAddress,int fd,int port,IPList** tail
 * Return value : IPList* new
 ============================================================================*/
int addToIPList(struct sockaddr_in address,int fd,int listenPort,IPList** tail)
{
	#if DEBUG
	printf("adding new element to the IP list\n");
	#endif

	char* mode = isServer == TRUE ? "Server" : "Client";

	IPList* new =(IPList*) malloc(sizeof(IPList));
	if(!new)
	{
		printf("Could not create space for new entity.\n");
		return FAILURE;
	}

	new->IPAddress = (char*) malloc(sizeof(char)*INET6_ADDRSTRLEN);
	inet_ntop(AF_INET, &(address.sin_addr), new->IPAddress, INET6_ADDRSTRLEN);

	new->hostname = (char*) malloc(sizeof(char)*HOST_NAME_MAX) ;
	getnameinfo(&address, sizeof(address),new->hostname,HOST_NAME_MAX, NULL, NULL, 0);

	printf("%s : A new connection from %s(%s)\n",mode,new->hostname,new->IPAddress);

	new->port = ntohs(ntohs(address.sin_port));
	new->listenPort = listenPort;
	new->fileDesc = fd;
	new->next = NULL;

	if(*tail == NULL) *tail = new;
	else
	{
		(*tail)->next =new;
		*tail = new;
	}

	return SUCCESS;
}


/*=============================================================================
 * Function Name: removeFromIPList
 * Function Desc: Removes given element from the IPList
 * Parameters   : char* IPAddress ,int port, IPList **head, IPList** tail
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
IPList* removeFromIPList(char* IPAddress,int port, IPList** head,IPList** tail)
{

	IPList* curr = *head;
	IPList* prev = *head;

	#if DEBUG
	printf("the port is =%d",port);
	printf("removing element from IPList\n");
	#endif

	/**
	 * Handle:
	 * 1. First element should be removed
	 * 2. Last element must be removed
	 * 3. Middle element or anything but last element
	 */
	while(curr)
	{
		if((!strcmp(IPAddress,curr->IPAddress)) && (curr->port == port))
		{
			#if DEBUG
			printf("found the element to remove!");
			#endif

			// first element + size of list more than 1
			if(curr == *head && curr->next)
			{
				*head =(IPList*) curr->next; // make head point to 2nd element
			}
			else if(curr->next) // middle element
			{
				prev->next =(IPList*) (curr)->next; // make prev element point to next one
			}
			else if(!curr->next) // last element
			{
				prev->next = NULL;
				*tail = prev ;
			}
			free(curr->hostname);
			free(curr->IPAddress);
			free(curr);
			return *head;
		}

		// iterate
		prev = curr;
		curr =(IPList*) curr->next;
	}
	return *head;
}

/*=============================================================================
 * Function Name: displayList
 * Function Desc: Displays the contents of the given list in the
 * 				  hostname,IP Addtess and Port No. format
 * Parameters   : IPList* head
 * Return value : void
 ============================================================================*/
void displayIPList(IPList* head)
{
	printf("id: Hostname\t\tIP Address\t\tPort No.\n");
	int i= 0;
	int port=0; // we need to switch this
	while(head)
	{
		i++;
		port = (isServer==TRUE) ? head->listenPort:head->port;
		printf("%d %s\t\t%s\t\t%d\n",i,head->hostname,head->IPAddress,port);
		head = (IPList*) head->next;
	}

	#if DEBUG
	printf("the number of connexions is =%d\n",i);
	#endif
}
/*=============================================================================
 * Function Name: freeIPList
 * Function Desc: Frees all the malloced elements of a linked list
 * Parameters   : IPList* , the pointer to the start of the linked list
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
int freeIPList(IPList* listHead)
{
	IPList* toFree = NULL;

	while(listHead != NULL)
	{
		toFree = listHead;
		listHead = (IPList *)listHead->next;

		free(toFree->IPAddress);
		free(toFree->hostname);
		free(toFree);
	}
	return SUCCESS;
}


/*=============================================================================
 * Function Name: getMaxFD
 * Function Desc: Given an IPList, the function fetchs the largest value of
 * 					of the file descriptor
 * Parameters   : IPList* , the pointer to the start of the linked list
 * Return value : int , the biggest value of FD in the list
 ============================================================================*/
int getMaxFD(IPList* head)
{
	IPList* curr = head;
	int max = 0;
	while(curr)
	{
		if(max<curr->fileDesc) max = curr->fileDesc;
		curr = curr->next;
	}
	return max;
}

/*=============================================================================
 * End of File
 *============================================================================*/


