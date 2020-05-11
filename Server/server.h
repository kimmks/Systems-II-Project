#ifndef __server_h__
#define __server_h__

#include <stdbool.h>
#include <pthread.h>


typedef enum
{
  LOAD,
	UNLOAD,
	BUILD,
	DESTROY,
	SEARCH,
	UNDEF
}request_t;


char * run_server ();
request_t read_data_request (char*, size_t, int);
void write_ack_message (int, request_t);
void write_err_message (int, request_t);
void write_data_packet (uint32_t, char*, char*, int);
void* new_client (void* args);
#define PORT_NUMBER 4025 

#endif
