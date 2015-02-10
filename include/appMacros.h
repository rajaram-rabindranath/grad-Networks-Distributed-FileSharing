/*
 * applicationMacros.h
 *
 *  Created on: Sep 7, 2013
 *      Author: dev
 */

#ifndef APPLICATIONMACROS_H_
#define APPLICATIONMACROS_H_

#define COMMAND_LENTH_MINSIZE 5

/*
 * The source files are peppered with print logs
 * The DEBUG macro is a compiler switch which when set
 * to '1' shall print all the logs :)
 * Default value of DEBUG is '0'
 * To avoid annoying print statements to console
 */
#define DEBUG 0

// a macro that specifies the command line arguments
#define ARG_COUNT 3

// Following macros specify place holders for commandline args
#define ARGS_APP_NAME 0
#define ARGS_APP_MODE 1
#define ARGS_APP_PORT 2

// Just to match with what the user provides and to use a switch statement
// to kick start server or client
#define SERVER 's'
#define CLIENT 'c'

// client code macros
#define MAX_CONNECTIONS_SERVER 4 // max number of connections Server
#define MAX_CONNECTIONS_CLIENT 5 // includes the server connection yes
#define SOCKET_REUSE_POLICY TRUE // application wide socket reuse policy
#define MSG_MTU 3000 // PACKET SIZE ... upload and therefore download

#endif /* APPLICATIONMACROS_H_ */
