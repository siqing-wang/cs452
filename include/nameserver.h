/*
 * nameserver.h
 */

#ifndef __NAMESERVER_H__
#define __NAMESERVER_H__

#define NAMESERVER_TID 1
#define NAMESERVER_SUCCESS 200

#define SERVERNAME_MAX_LENGTH 64

#define NServerMSG_REGAS 0
#define NServerMSG_WHOIS 1

/* Used by name server. */
typedef struct NameserverMessage
{
    int type;
    char serverName[SERVERNAME_MAX_LENGTH];

} NameserverMessage;


void nameServer();

#endif
