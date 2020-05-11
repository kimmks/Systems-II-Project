/*
 * CS 361: Server project driver
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
#include <sys/wait.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include "client.h"
#include "server.h"
#include "compress.h"
#include "main.h"
#include "hash.h"

//int cmdline (int, char **, char **, bool *, bool *);
//void* new_client (void* args);
/*
 * Usage for this driver.
 */
/*
void
usage (void)
{
  printf ("Usage: net [options]\n");
  printf (" Options are:\n");
  printf ("  -c            Run as a client and send requests to the server\n");
  printf ("  -S [Request]  Send a search request to the server\n");
  printf ("  -l <FileName> Request for the server to load the specified file\n");
  printf ("\n   -The only valid files at the moment are: 'drugs.csv' and 'air_quality.csv'\n\n");
  printf ("  -b [Field]    Makes the server build a searchable tree based on the field given\n");
  printf ("  -d [Field]    Makes the server remove a searchable tree based on the field given\n");
  printf ("\n   -The valid fields for  building/destroying at the moment are:\n");
  printf ("                    'Year', 'State', 'Month', 'Data Value'\n\n");
  printf ("  -s            Use domain as a server on port 4025\n");
  //printf ("  -x [requests]  Run a stress test request number of times\n");// @andy
}
*/

int
main (int argc, char **argv)
{
  char *port_number = "4025";
  char *message = "test";
  bool client = false;
  bool server = false;
  request_t req_type = UNDEF;
  //bool stressTest = false; @andy
  //int stressRequests = 0; @andy
  short int port = 0;
  if (cmdline (argc, argv, &message, &client, &server, &req_type) < 0)
  //if (cmdline (argc, argv, &message, &client, &server, &stressTest,
  //&stressRequests) < 0) @andy
    {
      char *args[] = {"cat", "help_message.txt", NULL};
      if (execvp("cat", args) == -1)
        {
          printf("exec failed\n");
        }
      //usage ();
      return EXIT_FAILURE;
    }
  if (client)//if (client || stressTest) @andy
    {
      /* @andy
      if (stressTest) 
        {
        int i;
        for (i = 0; i < stressRequests; i++)
          {
            //TODO create randomly generated request(message)
            run_client("localhost", port_number, message);
          }
        }
        else {
      */
      run_client("localhost", port_number, message, req_type);
      printf("Connection ended.\n");
    }
  else // server option.
    {
      port = 4025;
      if (port < 4000 || port > 4100) return EXIT_FAILURE;
      char *request = run_server ();
      free (request);
    }
  return EXIT_SUCCESS;
}

/**
 * Command line parser.
 **/
int
cmdline (int argc, char **argv, char **message, bool *client, bool *server, request_t *type)
//cmdline (int argc, char **argv, char **message, bool *client, bool *server, bool *stressTest, int *stressRequests) @andy
{
  int option;
  int client_ops = 0;
  bool lflag = false;
  bool bflag = false;
  bool dflag = false;
  while ((option = getopt (argc, argv, "hcsS:l:ub:d:")) != -1)
    {
      switch (option)
        {
        case 'h': return -1;
        case 'c': *client = true;
                  break;
        case 's': *server = true;
                  break;
        case 'S': client_ops++;
                  *message = optarg;
                  *type = SEARCH;
                  break;
        case 'l': client_ops++;
                  *message = optarg;
                  *type = LOAD;
                  lflag = true;
                  break;
        case 'u': client_ops++;
                  *type = UNLOAD;
                  *message = "UNLOAD";
                  break;
        case 'b': client_ops++;
                  *type = BUILD;
                  *message = optarg;
                  bflag = true;
                  break;
        case 'd': client_ops++;
                  *type = DESTROY;
                  *message = optarg;
                  dflag = true;
                  break;
        /* @andy
        case 'x': *stressTest = true;
                  //TODO ensure optarg is an int/can be cast to an int
                  stressRequests = (int) optarg;
                  break;
        */
        default:  return -1;
        }
    }
  if ((*client && *type == UNDEF) || (*client && client_ops != 1))
    {
      return -1;
    }
  if (lflag)
    {
      /*
      if (((strncmp("drugs.csv", *message, 9) != 0) && (strncmp("air_quality.csv", *message, 15) != 0)))
        {
          return -1;
        }
      else
        {
          size_t len = strlen(*message);
          if (len != 9 && len != 15)
            {
              return -1;
            } 
        }
        */
    }
  
  if (bflag || dflag)
    {
      //char valid_fields[][] = {"state", "month", "year", "dataValue", "stateName", "countyName", "reportYear", "value"};
      bool invalid_drugs = false;
      bool invalid_air = false;
      if (((strncmp("State", *message, 5) != 0) && (strncmp("Month", *message, 5) != 0)) && (strncmp("Year", *message, 4) != 0) && (strncmp("Data Value", *message, 10) != 0))
        {
          invalid_drugs = true;
        }
      if (((strncmp("StateName", *message, 9) != 0) && (strncmp("CountyName", *message, 10) != 0)) && (strncmp("ReportYear", *message, 10) != 0) && (strncmp("Value", *message, 5) != 0))
        {
          invalid_air = true;
        } 
      if (invalid_drugs && invalid_air)
        {
          return -1;
        }
    }
    
  /* @andy
  if (*client && *server || *server && *stressTest) 
    {
    return -1;
    }
  */

  return 0;
}




