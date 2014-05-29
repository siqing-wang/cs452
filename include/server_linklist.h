/*
 *  server_linklist.h
 *      Implements a linklist data structure
 */

#ifndef __SERVER_LINKLIST_H__
#define __SERVER_LINKLIST_H__

#include <message.h>

typedef struct ServerLinklistNode {
	char serverName[SERVERNAME_MAX_LENGTH];
	int tid;
    struct ServerLinklistNode* next;
} ServerLinklistNode;

void ServerLinklistNode_init(ServerLinklistNode *node, char *serverName, int tid);
void ServerLinklist_pushFront(ServerLinklistNode *list, ServerLinklistNode *node);
ServerLinklistNode* ServerLinklist_find(ServerLinklistNode *list, char *serverName);

#endif
