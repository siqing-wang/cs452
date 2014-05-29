/*
 *	server_linklist.c
 */

#include <server_linklist.h>
#include <utils.h>

void ServerLinklistNode_init(ServerLinklistNode *node, char *serverName, int tid) {
    stringCopy(node->serverName, serverName, SERVERNAME_MAX_LENGTH);
    node->tid = tid;
    node->next = (ServerLinklistNode*)0;
}

void ServerLinklist_pushFront(ServerLinklistNode *list, ServerLinklistNode *node) {
    node->next = list;
}

ServerLinklistNode* ServerLinklist_find(ServerLinklistNode *list, char *serverName) {
    for(;;) {
        if (list == (ServerLinklistNode*)0) {
            return list;
        } else if (stringEquals(list->serverName, serverName)) {
            return list;
        }
        list = list->next;
    }
}
