 /*
 * nameserver.c
 */

#include <nameserver.h>
#include <syscall.h>
#include <utils.h>

#define SERVICETABLE_MAX_SIZE 32

/*
 * server hashtable data structure.
 */
typedef struct NSHashNode {
    char serverName[SERVERNAME_MAX_LENGTH];
    int tid;
    struct NSHashNode* next;
} NSHashNode;

void NSHashNode_init(NSHashNode *node, char *serverName, int tid) {
    stringCopy(node->serverName, serverName, SERVERNAME_MAX_LENGTH);
    node->tid = tid;
    node->next = (NSHashNode*)0;
}

void ServerLinklist_pushFront(NSHashNode *list, NSHashNode *node) {
    node->next = list;
}

NSHashNode* ServerLinklist_find(NSHashNode *list, char *serverName) {
    for(;;) {
        if (list == (NSHashNode*)0) {
            return list;
        } else if (stringEquals(list->serverName, serverName)) {
            return list;
        }
        list = list->next;
    }
}

typedef struct NSHash {
    int serviceCount;
    NSHashNode linklistTable[SERVICETABLE_MAX_SIZE];
    NSHashNode *serviceTable[HASH_TABLE_SIZE];
} NSHash;

void NSHash_init(NSHash *hashtable) {
    hashtable->serviceCount = 0;
    int i = 0;
    for(; i < HASH_TABLE_SIZE; i++) {
        (hashtable->serviceTable)[i] = (NSHashNode *)0;
    }
}

NSHashNode* NSHash_get(NSHash *hashtable, char *name) {
    int hash = computeHash(name);
    NSHashNode* list = hashtable->serviceTable[hash];
    return (NSHashNode*)ServerLinklist_find(list, name);
}

int NSHash_insert(NSHash *hashtable, char *name, int tid) {
    int hash = computeHash(name);
    NSHashNode* list = hashtable->serviceTable[hash];
    if (hashtable->serviceCount >= SERVICETABLE_MAX_SIZE) {
        return 0;
    }
    NSHashNode* newEntry = (NSHashNode *)(hashtable->linklistTable + hashtable->serviceCount);

    NSHashNode_init(newEntry, name, tid);
    ServerLinklist_pushFront(list, newEntry);
    hashtable->serviceTable[hash] = newEntry;
    hashtable->serviceCount++;
    return 1;
}

/*
 * NameServer
 */
void nameServer() {

    NSHash hashtable;
    NSHash_init(&hashtable);

    int tid;
    NameserverMessage message;
    char *name;
    for(;;) {
        int byteSent = Receive(&tid, &message, sizeof(NameserverMessage));
        if (byteSent > sizeof(NameserverMessage)) {
            warning("Nameserver: Message Request overflowed. Ignored.");
            continue;
        }

        NSHashNode* result = NSHash_get(&hashtable, name);
        int resultTid = -1;
        if (result != (NSHashNode *)0) {
            resultTid =  result->tid;
        }
        switch (message.type) {
            int syscallResult;
            int successCode = NAMESERVER_SUCCESS;
            case NServerMSG_REGAS :
                if (result != (NSHashNode *)0) {
                    result->tid = tid;
                } else {
                    if (!NSHash_insert(&hashtable, name, tid)) {
                        warning("Service Table Full.");
                    }
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
