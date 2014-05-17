/*
 * request.h
 */

#ifndef __REQUEST_H__
#define __REQUEST_H__

typedef struct Request
{
	/* data */
	int id;
} Request;

void request_handle(Request *request);

#endif
