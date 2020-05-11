#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "hash.h"

#define BUFFER_MAX_SIZE 4096

/* Given a struct with server information, return a dynamically allocated
 * string of the IP address in dotted decimal or colon hexadecimal notation.
 * Consult Chapter 4, as well as /usr/include/netdb.h and
 * /usr/include/arpa/inet.h as needed. Use inet_ntop() to get the formatted
 * string based on the address's ai_addr field. If passed server parameter is
 * NULL, return a dynamically allocated copy of "no address information".
 */
char *
addr_string (struct addrinfo *server)
{
  if (!server)
    {
      char *toRet = malloc(28);
      strncpy(toRet, "no address information", 23);
      return toRet;
    }
  if (server->ai_family == AF_INET)
    {
      struct sockaddr_in *info = (struct sockaddr_in *)server->ai_addr;
      char *inaddr =  malloc(INET_ADDRSTRLEN);
      
      assert((inet_ntop(AF_INET, &info->sin_addr, inaddr, INET_ADDRSTRLEN)) != NULL);
      return inaddr;
    }
  else if (server->ai_family == AF_INET6)
    {
      struct sockaddr_in6 *info6 = (struct sockaddr_in6 *)server->ai_addr;
      char *inaddr6 =  malloc(INET6_ADDRSTRLEN);
      
      assert((inet_ntop(AF_INET6, &info6->sin6_addr, inaddr6, INET6_ADDRSTRLEN)) != NULL);
      return inaddr6;
    }   
  return NULL;
}

/* Given the server address info, return a dynamically allocated string with
 * its transport-layer protocol, IP version, and IP address. For instance,
 * querying jmu.edu over TCP/IPv4 should return:
 *   "TCP IPv4 134.126.10.50"
 * Use addr_string() to get the formatted address string, concatenate it to
 * the string to return, then free the result from addr_string(). If the
 * passed server parameter is NULL, return a dynamically allocated copy of
 * "no address information".
 */
char *
serv_string (struct addrinfo *server)
{
  if (server == NULL)
    {
      char *toRet = malloc(28);
      strncpy(toRet, "no address information", 23);
      return toRet; 
    }
  char *servInfo = (char *) malloc(10 + INET6_ADDRSTRLEN);
  if (server->ai_protocol == IPPROTO_TCP || server->ai_socktype == SOCK_STREAM)
    {
      strncpy(servInfo, "TCP ", 4);
    }
  else if (server->ai_protocol == IPPROTO_UDP || server->ai_socktype == SOCK_DGRAM)
    {
      strncpy(servInfo, "UDP ", 4);
    }
  if (server->ai_family == AF_INET6)
    {
      strncpy(&servInfo[4], "IPv6 ", 5);
    }
  else if (server->ai_family == AF_INET)
    {
      strncpy(&servInfo[4], "IPv4 ", 5);
    }
  char *ipaddr = addr_string(server);
  strncpy(&servInfo[9], ipaddr, INET6_ADDRSTRLEN);
  free(ipaddr);
  return servInfo;
}

/* Given a hostname string, use getaddrinfo() to query DNS for the specified
 * protocol parameters. Boolean values indicate whether or not to use IPv6
 * (as opposed to IPv4) or TCP (as opposed to UDP). The protocol should be
 * a string constant such as "http" or "ftp", or a string of a custom port
 * number such as "4100". Return the pointer to the linked list of server
 * results. If the server parameter is true, set the ai_flags value to
 * AI_PASSIVE (used to create a server socket instead of a client socket).
 */
struct addrinfo *
get_server_list (const char *hostname, const char *proto, bool tcp, bool ipv6,
                 bool server)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  if (ipv6)
    {
      hints.ai_family = AF_INET6;
    }
  else 
    {
      hints.ai_family = AF_INET;
    }

  if (server)
    {
      hints.ai_flags = AI_PASSIVE;
    }

  if (tcp)
    {
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
    }
  else 
    {
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = IPPROTO_UDP;
    }
  
  struct addrinfo *toRet = NULL;
  getaddrinfo(hostname, proto, &hints, &toRet);
  return toRet;
}

 /* Runner for the client. Will try and open a connection with the host
  * hostname and the protocol proto. In this case the hostname should
  * be localhost or stu. The protocol should always be 4025 for the
  * project. The request will be written over the socket and processed
  * by the server. 
  */
char *
run_client (const char *hostname, char *proto, char *request, request_t type)
{
  char buffer[BUFFER_MAX_SIZE];  // Should not need more that this
  memset (buffer, 0, sizeof (buffer));
  struct addrinfo *serv_list = get_server_list(hostname, proto, true, false, false);
  int socketfd = -1;
  struct addrinfo *serv = NULL;
  for (serv = serv_list; serv != NULL; serv = serv->ai_next) // connect to socket
    {
      if ((socketfd = socket(serv->ai_family, serv->ai_socktype, 0)) < 0)
        continue;
      if (connect (socketfd, serv->ai_addr, serv->ai_addrlen) == 0)
        {
          printf("Connected.\n\n");
          break;
        }
      close (socketfd);
      socketfd = -1;
    }
  freeaddrinfo (serv_list);
  if (socketfd < 0) 
    {
      printf("Failed to connect\n");
      exit(1); // check if socket did not attach propely
    }
  printf ("Sending: %s\n\n", request); //writing the request. 
  size_t len = strlen(request);
  send_data_request (request, len, socketfd, type);  
  int ack = read_ack (socketfd); 
  if (type == SEARCH && ack == 0)
    { 
      read_data (socketfd);
    }
  return NULL;
}

/*
* Send the data request to the server.
*/
void
send_data_request (char *request, size_t req_len, int sockfd, request_t type)
{
  //TODO: Add in the fucntionality of the data format.
  if (req_len < 4 || req_len > 83)
    {
      printf("Invalid sending size, Must be between 6-83 characters\n");
      exit(EXIT_FAILURE);
    }
  char *hold = NULL;
  switch (type)
    {
      case SEARCH: 
        hold = malloc (req_len + 7);
        strncpy (hold, "SEARCH ", 7);
        strncpy (&hold[7], request, req_len);
        write (sockfd, hold, req_len+7);
        break;
      case LOAD:
        hold = malloc (req_len + 5);
        strncpy (hold, "LOAD ", 5);
        strncpy (&hold[5], request, req_len);
        write (sockfd, hold, req_len+5);
        break;
      case UNLOAD:
        write (sockfd, "UNLOAD", 6);
        break;
      case BUILD:
        hold = malloc (req_len + 6);
        strncpy (hold, "BUILD ", 6);
        strncpy (&hold[6], request, req_len);
        write (sockfd, hold, req_len+6);
        break;
      case DESTROY:
        hold = malloc (req_len + 8);
        strncpy (hold, "DESTROY ", 8);
        strncpy (&hold[8], request, req_len);
        write (sockfd, hold, req_len+8);
        break; 
      default: 
        break;
    }
  if (hold)
    {
      free (hold);
      hold = NULL;
    }
  //write (sockfd, request, req_len);
}

/*
* Read in and print the acknowledgment form the server.
* Return 1 if its a failed search, 0 on anything else.
*/
int
read_ack (int sockfd)
{
  char recieved[36];
  read (sockfd, recieved, 36);
  printf("Recieved: %s\n\n", recieved);
  if (recieved[0] == 'F')
    {
      return 1;
    } 
  return 0;
}

/*
* Read in the entire data packet and interpret it.
*/
void
read_data (int sockfd)
{
  char *hash = malloc(64); // read in the hash
  read (sockfd, hash, 64);
  printf("Hash Val: %s\n", hash);
  free(hash);
  hash = NULL;
  /*
  char ID[4]; // read in the 4 letter id
  read (sockfd, ID, 4);
  printf("ID: %s\n", ID);
  */
  char size[8]; // get the size of the data sent
  read (sockfd, size, 8);
  uint32_t sz = atoi(size);
  printf("Size: %u\n", sz);

  char data[sz]; // get the compressed data and decompress it.
  read (sockfd, data, sz);
  uint32_t retLen = 0;
  char *out = decompress(data, sz, &retLen);
  printf("Data: ");
  for (int i = 0; i < retLen; i++)
    {
      printf("%c", out[i]);
    }
  printf("\n\n");
  char *hashed_data = doHash(out, retLen);

  printf("Hashed Data: %s\n", hashed_data); 
  free(out);
}

