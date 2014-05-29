 /*
 * nameserver.c
 */

#include <nameserver.h>
#include <server_linklist.h>
#include <syscall.h>
#include <bwio.h>
#include <utils.h>

void nameServer() {
    bwprintf(COM2, "---NameServer Initialized---\n\r");
    ServerLinklistNode linklistTable[SERVICETABLE_MAX_SIZE];
    ServerLinklistNode *serviceTable[HASH_TABLE_SIZE];
    int i = 0;
    for(; i < HASH_TABLE_SIZE; i++) {
        serviceTable[i] = (ServerLinklistNode *)0;
    }

    int tid;
    int serviceCount = 0;
    char serverName[SERVERNAME_MAX_LENGTH];
    for(;;) {
        int byteSent = Receive(&tid, serverName, SERVERNAME_MAX_LENGTH);

        if (serviceCount == SERVICETABLE_MAX_SIZE - 1) {
            warning("Service Table Full.");
            continue;
        }

        if (byteSent > SERVERNAME_MAX_LENGTH) {
            warning("Service Name Too Long.");
        }

        int hash = computeHash(serverName);
        ServerLinklistNode* list = serviceTable[hash];
        ServerLinklistNode* result = ServerLinklist_find(list, serverName);
        if (result == (ServerLinklistNode *)0) {
            result = (ServerLinklistNode *)(linklistTable + serviceCount);
            serviceCount++;
            ServerLinklistNode_init(result, serverName, tid);
            ServerLinklist_pushFront(list, result);
            serviceTable[hash] = result;
        } else {
            result->tid = tid;
        }

    }

    Exit();
}
