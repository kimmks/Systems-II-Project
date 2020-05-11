#ifndef __client_h__
#define __client_h__

#include <netdb.h>
#include <stdbool.h>
#include "compress.h"
#include "server.h"

char * addr_string (struct addrinfo *);
char * serv_string (struct addrinfo *);
struct addrinfo * get_server_list (const char *, const char *, bool, bool, bool);
char * run_client (const char *, char *, char *, request_t);
void send_data_request (char *request, size_t req_len, int sockfd, request_t);
int read_ack (int sockfd);
void read_data (int sockfd);

#endif
