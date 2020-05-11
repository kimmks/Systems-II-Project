/*
 * CS 361: Template project driver
 *
 * Name: Andy Malone 
 */

#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "client.h"
#include "server.h"

int cmdline (int, char **, char **, bool *, char **, short int *, bool *);

void
usage (void)
{
  printf ("Usage: net [options] domain\n");
  printf (" Options are:\n");
  printf ("  -w F    Connect to the domain and get file F with HTTP/1.0\n");
  printf ("  -s      Use domain as a server port number\n");
  printf ("  -6      Use IPv6 addresses\n");
  printf ("  -p P    Lookup address for application protocol P\n");
}

int
main (int argc, char **argv)
{
  char *protocol = "http";
  bool ipv6 = false;
  char *webfile = NULL;
  bool server = false;
  short int port = 0;
  if (cmdline (argc, argv, &protocol, &ipv6, &webfile, &port, &server) < 0)
    {
      usage ();
      return EXIT_FAILURE;
    }
  char *domain = argv[optind];

  if (webfile == NULL && !server)
    {
      struct addrinfo *info;
      if (strcmp(protocol, "53") == 0 || strcmp(protocol, "67")  == 0)
        {
          info = get_server_list (domain, protocol, false, ipv6,  server); 
        }
      else 
        {
          info = get_server_list (domain, protocol, true, ipv6,  server); 
        }
      char *servInfo = serv_string (info);
      printf("%s %s: %s\n", domain, protocol, servInfo);
      free(servInfo);
      freeaddrinfo(info);
      // C requirements:
      //   Get the server list for the specified domain and protocol
      //   Given this command-line:
      //     ./net -p http www.jmu.edu
      //   Print the following output:
      //     www.jmu.edu http: TCP IPv4 134.126.10.50
      //   The requested transport-layer protocol should be based on the
      //   requested application-layer protocol. TCP is default, but use
      //   UDP for port numbers 53 and 67.
    }
  else if (webfile != NULL)
    {
      char *file = NULL;
      char *firstLine = web (domain, protocol, webfile, &file, ipv6); 
      printf("Result: %s\n", firstLine);
      //printf("Type: %s\n", file);
      free (file);
      free (firstLine); 
      // B requirements:
      //   Use web() to create a socket and retrieve the file headers.
      //   Print the result of web() as follows:
      //     "Result: HTTP/1.1 200 OK"
      //   If the type is also set, print it as follows:
      //     "Type: text/html; charset=UTF-8"
    }
  else
    {
      // A requirements:
      //   Set up a basic web server on the port specified in server.h.
      //   There are no integration tests for this, but you can test your
      //   code by running this with your port number (see server.h) on
      //   the command-line, then going to stu.cs.jmu.edu:4*** (where 4***
      //   is your port number). Note that this will only work while on
      //   campus.
      port = 4025;
      if (port < 4000 || port > 4100) return EXIT_FAILURE;
      printf("here\n");
      char *request = apache (port, ipv6);

      //printf("here\n");
      //printf ("Request: %s\n", request);
      free (request);
    }

  return EXIT_SUCCESS;
}

/* DO NOT MODIFY THIS FUNCTION */

int
cmdline (int argc, char **argv, char **protocol, bool *ipv6, char **web,
         short int *port, bool *server)
{
  int option;

  while ((option = getopt (argc, argv, "p:6w:o:sh")) != -1)
    {
      switch (option)
        {
          // Change this to merge -p and -o as same flag
        case 'p': *protocol = optarg;
                  break;
        case '6': *ipv6 = true;
                  break;
        case 'w': *web = optarg;
                  break;
        case 'o': *port = (short int) strtol (optarg, NULL, 10);
                  break;
        case 's': *server = true;
                  break;
        case 'h': return -1;
                  break;
        default:  return -1;
        }
    }

  if (optind != argc - 1) return -1;
  if (*web != NULL && *server) return -1;

  return 0;
}
