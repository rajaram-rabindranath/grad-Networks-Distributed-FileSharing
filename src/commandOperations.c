/*=============================================================================
 * File Name: client.c
 * Project  : cse589_project1
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : September 18th 2013
 ============================================================================*/
#include <error.h>
#include <IPList.h>
#include <globalVars.h>
#include <stdio.h>
#include <stdlib.h>
#include <appMacros.h>
#include <string.h>
#include <commandOperations.h>

// function prototypes
int help();
int creator();

#define MAX_COMMAND_ARGS 3 // maximum argument a user can input on a console (includes command)
#define COMMAND_COUNT 10 // total number of commands

// COMMANDS at different indices .. shall be of great help
char* commands[COMMAND_COUNT] = {"HELP","MYIP","MYPORT","CREATOR","REGISTER","CONNECT","LIST","TERMINATE","EXIT","DOWNLOAD"};
char* commandsLower[COMMAND_COUNT] = {"help","myip","myport","creator","register","connect","list","terminate","exit","download"};

/*=============================================================================
 * Function Name: commandMaster
 * Function Desc: Takes what the user enters on the console as input and
 * 					parses the input to make sense of the same
 * Parameters   : char* command
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int commandMaster(char* command)
{
	#if DEBUG
	printf("the console input is %s",command);
	#endif

	// strtok modifies the input string therefore copying to another mem location
	char* commandCopy = malloc(sizeof(char)*(strlen(command)));
	strcpy(commandCopy,command);

	int tokenCount = 0, i =0;
	char** tokens = (char**) malloc(MAX_COMMAND_ARGS*sizeof(char*));
	int commandID=0;
	char* mode = isServer ? "Server":"Client";
	tokens[tokenCount] = strtok(commandCopy," ");

	// check if command is valid
	if(tokenCount == 0)
	{
	   for(commandID=0;commandID<COMMAND_COUNT;commandID++)
	   {
		   if((!(strcmp(tokens[tokenCount],commands[commandID])))||(!(strcmp(tokens[tokenCount],commandsLower[commandID]))))
		   {
			   #if DEBUG
			   printf("command found!\n");
			   #endif
			   break;
		   }
	   }
	   if(commandID == COMMAND_COUNT) // cannot find command in command_library
	   {
		   printf("%s : Invalid command %s\n",mode,tokens[tokenCount]);
		   free(commandCopy);
		   // should free tokens --- FIXME
		   return FAILURE;
	   }
	}

	// check if server should process those command
	if(isServer && (commandID == OPCODE_CONNECT || commandID == OPCODE_REGISTER || commandID == OPCODE_DONWLOAD))
	{
		printf("%s : This request are not handled by me\n",mode);
		free(commandCopy);
		// should free tokens --- FIXME
		return FAILURE;
	}

	// get all arguments given & only process 2 args anything more than that discard
	while(tokens[tokenCount] && tokenCount+1 < MAX_COMMAND_ARGS)
	{
		tokens[++tokenCount]=strtok(NULL," ");
	}

	#if 0
	printf("the value of command =%s and index is%d\n",tokens[tokenCount],tokenCount);
	for(i=0;i<=tokenCount;i++)
	{
		printf("user input is %s at %d\n",tokens[i],i);
	}
	#endif

	// call appropriate functions
	switch(commandID)
	{
		case OPCODE_HELP:
			help();
			break;
		case OPCODE_CREATOR:
			creator();
			break;
		case OPCODE_MYIP:
			printf("%s : IPAddress of the machine I am running on=%s\n",mode,myIP);
			break;
		case OPCODE_MYPORT:
			printf("%s : Port I am using =%d\n",mode,myPort);
			break;
		default:
			commandArgs = tokens;
			return(commandID);
			break;
	}
	free(commandCopy);
	return SUCCESS;
}

/*=============================================================================
 * Function Name: help
 * Function Desc: Lists the different commands at that are at the users disposal
 * Parameters   : void
 * Return value : SUCCESS
 ============================================================================*/
int help()
{
	printf("All commands are case sensitive, Following are the list of available commands\n");
	printf("\tHELP    - Displays list of valid commands\n");
	printf("\tCREATOR - Displays details of the Author\n");
	printf("\tMYPORT  - Displays port used by process\n");
	printf("\tMYIP    - Displays IP address of machine on which this process is running\n");
	printf("\tList - Displays a numbered list\n");
	if(!isServer)
	{
		printf("\tCONNECT <DEST> <PORT> - Connects to <DEST> at port <PORT>\n");
		printf("\tREGISTER <SERVERIP> <PORT> - Registers with server at given IPAddress and port\n");
		printf("\tDOWNLOAD <file_name> <file_chunk_size_in_bytes>\n");
	}
	printf("\tTERMINATE <connection id.> - Terminates the connection with <id> begotten using list command\n");
	printf("\tEXIT - Shutdown\n");
	return SUCCESS;
}

/*=============================================================================
 * Function Name: creator
 * Function Desc: Displays Full Name, UB IT name and UB email id of the creator
 * Parameters   : Port number
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int creator(void)
{
	printf("Full Name : Rajaram Rabindranath\n");
	printf("UB IT Name: rajaramr\n");
	printf("UB email  : rajaramr@buffalo.edu\n");
	return SUCCESS;
}
/*=============================================================================
 * End of File
 *============================================================================*/

