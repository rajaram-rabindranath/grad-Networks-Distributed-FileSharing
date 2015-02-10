/*=============================================================================
 * File Name: rajaramr_proj1.c
 * Project  : cse589_project1
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : September 7th 2013
 ============================================================================*/

#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h> // for strcmp
#include <stdlib.h>
#include <appMacros.h>
#include <error.h>
#include <client.h>
#include <server.h>


// global variables
char* myIP;
int myPort;
int isServer = FALSE;
char** commandArgs;

//function prototype
void getThisMachineIP();

/*=============================================================================
 * Function Name: main
 * Function Desc: This is the entry point of the application
 * Parameters   :
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
int main(int argc, char** argv)
{
	// invalid argument count
	if(argc != ARG_COUNT)
	{
		printf("	ERROR: Wrong number of cmd arguments.\n");
		usage(argv[ARGS_APP_NAME]);
		return FAILURE;
	}

	char* appMode = argv[ARGS_APP_MODE];
	int listenPort = atoi(argv[ARGS_APP_PORT]);

	/* setting global var to inform client/server the port
	 * to be used. Have to share this with command operations
	 * due to modularization (code being put into commanddops....)
	 * and therefore ease of information access*/
	myPort = listenPort;
	getThisMachineIP();

	// kick start server or client
	switch(*appMode)
	{
		case SERVER:
			isServer = TRUE; // gloabal var helps in decision making viz-a-viz shell command execution
			startServer(listenPort);
			break;
		case CLIENT:
			startClient(argv[ARGS_APP_PORT]);
			break;
		default:
			printf("ERROR: Invalid argument for application mode.\n");
			usage(argv[ARGS_APP_NAME]);
			return FAILURE;
	}
	return SUCCESS;
}

/*=============================================================================
 * Function Name: usage
 * Function Desc: Shall output to the console the application's usage
 * Parameters   : char* appName
 * Return value : void
 ============================================================================*/
void usage(char* appName)
{
	printf("--------------------------------------------------------------------------------\n");
	printf("\tUsage: %s <APP_MODE> <LISTENING_PORT>\n",appName);
	printf("\tAnd valid values for APP_MODE are 's' or 'c' ; Server and Client modes respectively.\n");
	printf("\tNote: APP_MODE value is case sensitive -- only lower case allowed\n");
	printf("---------------------------------------------------------------------------------\n");
}

/*=============================================================================
 * Function Name: getThisMachineIP
 * Function Desc: This function finds the IPAddress of the host (server/client)
 * 				  machine
 * Parameters   : void
 * Return value : void
 ============================================================================*/

void getThisMachineIP()
{
	struct ifaddrs *myAddrs, *iterateAddr;
	struct sockaddr_in* ip;
	char* mode = isServer ? "Server" : "Client";

	if(getifaddrs(&myAddrs) != 0)
	{
		perror("ERROR:");
		printf("%s : failed to get own IP address\n",mode);
		myIP = "CANNOT FIND";
		return;
	}

	iterateAddr = myAddrs;

	// iterate through the address begotten
	while(iterateAddr)
	{
		// THERE IS AN IP ADDRESS & THE INTERFACE IS UP
		if((!iterateAddr->ifa_addr) || ((iterateAddr->ifa_flags & IFF_UP) == 0))
		{
			iterateAddr = iterateAddr->ifa_next;
			continue;
		}
		// extract IP address
		ip = (struct sockaddr_in *)iterateAddr->ifa_addr;
		if(ip->sin_family == AF_INET)
		{
			myIP = (char*) malloc(sizeof(char)*INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &(ip->sin_addr),myIP,INET6_ADDRSTRLEN);
			//break; the first element is loopback ... but we do not want that ... so we have commented out break
		}

		iterateAddr = iterateAddr->ifa_next;
	}
	free(myAddrs);
}
/*=============================================================================
 * End of File
 *============================================================================*/

