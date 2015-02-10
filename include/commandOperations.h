/*
 * commandOperations.h
 *
 *  Created on: Sep 19, 2013
 *      Author: dev
 */

#ifndef COMMANDOPERATIONS_H_
#define COMMANDOPERATIONS_H_

int commandMaster(char* msg);

// Each command shall have a code associated with it
#define OPCODE_HELP 0
#define OPCODE_MYIP 1
#define OPCODE_MYPORT 2
#define OPCODE_CREATOR 3
#define OPCODE_REGISTER 4
#define OPCODE_CONNECT 5
#define OPCODE_LIST 6
#define OPCODE_TERMINATE 7
#define OPCODE_EXIT 8
#define OPCODE_DONWLOAD 9

#endif /* COMMANDOPERATIONS_H_ */
