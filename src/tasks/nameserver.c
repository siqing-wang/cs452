 /*
 * nameserver.c
 */

#include <nameserver.h>
#include <server_linklist.h>
#include <syscall.h>
#include <utils.h>

#define SERVICETABLE_MAX_SIZE 32

void nameServer() {
    ServerLinklistNode linklistTable[SERVICETABLE_MAX_SIZE];
    ServerLinklistNode *serviceTable[HASH_TABLE_SIZE];
    int i = 0;
    for(; i < HASH_TABLE_SIZE; i++) {
        serviceTable[i] = (ServerLinklistNode *)0;
    }

    int tid;
    int serviceCount = 0;
    NameserverMessage message;
    char *serverName;
    for(;;) {
        int byteSent = Receive(&tid, &message, sizeof(NameserverMessage));
        if (byteSent > sizeof(NameserverMessage)) {
            warning("Nameserver: Message Request overflowed. Ignored.");
            continue;
        }

        serverName = message.serverName;
        int hash = computeHash(serverName);
        ServerLinklistNode* list = serviceTable[hash];
        ServerLinklistNode* result = ServerLinklist_find(list, serverName);

        int resultTid = -1;
        if (result != (ServerLinklistNode *)0) {
            resultTid = result->tid;
        }

        switch (message.type) {
            int syscallResult;
            int successCode = NAMESERVER_SUCCESS;
            case NServerMSG_REGAS :
                if (result != (ServerLinklistNode *)0) {
                    result->tid = tid;
                } else if (serviceCount < SERVICETABLE_MAX_SIZE - 1) {
                    result = (ServerLinklistNode *)(linklistTable + serviceCount);
                    serviceCount++;
                    ServerLinklistNode_init(result, serverName, tid);
                    ServerLinklist_pushFront(list, result);
                    serviceTable[hash] = result;
                } else {
                    warning("Service Table Full.");
                }
                syscallResult = Reply(tid, &successCode, sizeof(int));
                break;
            case NServerMSG_WHOIS :
                syscallResult = Reply(tid, &resultTid, sizeof(int));
                break;
            default :
                warning("Unknown Nameserver Message Type.");
        }
    }

    Exit();
}
