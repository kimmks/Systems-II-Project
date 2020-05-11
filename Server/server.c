#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


#include "client.h"
#include "server.h"
#include "compress.h"
#include "hash.h"
#include "main.h"
#include "../dataindex/treecontrol.h"
#include "../dataindex/treecontrol.c"
#include "../dataindex/tree.h"

bool file_loaded = false;
bool curr_file = true; //TRUE for drugs.csv FALSE for air_quality.csv
char *drug_ptr = NULL;
char *airqual_ptr = NULL;
int file_size = 0;
node trees[8];
int queue_length = 0;
int queue_inspos = 0;
int queue_curpos = 0;
int fdqueue[100];
sem_t current_reqs;
sem_t child_num;
pthread_mutex_t queue_lock; // declare locks
pthread_mutex_t load_lock;
pthread_mutex_t unload_lock;
pthread_mutex_t state_lock;
pthread_mutex_t year_lock;
pthread_mutex_t data_lock;


/*
* trees[0] = State tree for drugs.csv
* trees[1] = Year tree for drugs.csv
* trees[2] = Month tree for drugs.csv
* trees[3] = Data Value for drugs.csv
* trees[4] = StateName for air_quality.csv
* trees[5] = CountyName for air_quality.csv
* trees[6] = ReportYear for air_quality.csv
* trees[7] = Value for air_quality.csv
*/

/*
* Author: Andrew Malone
* Version: 04/13/2019
*/

/* Create a TCP server socket on localhost for port number 4025. 
 * This server is permanent and can be only be shut down remotely.
 * At the moment it reads from a socket and then writes back what was
 * in it. 
 *
 * The sever has a 10 second timeout for connections. It can hold up to
 * 10 pending connections at once. 
 *
 * The server will exit when the string "exit" is sent to it. It will 
 * then close the socket and return. 
 */
char *
run_server ()
{
  for (int i = 0; i < 8; i++)
    {
      trees[i] = NULL;
    } 
  sem_init (&current_reqs, 0, 0);
  sem_init (&child_num, 0, 4);
  pthread_mutex_init (&queue_lock, NULL); // initialize semaphores and locks
  pthread_mutex_init (&load_lock, NULL);
  pthread_mutex_init (&unload_lock, NULL);
  pthread_mutex_init (&state_lock, NULL);
  pthread_mutex_init (&year_lock, NULL);
  pthread_mutex_init (&data_lock, NULL);
 
  char thePort[5];
  snprintf (thePort, 5, "%hd", PORT_NUMBER); // set it on port 4025
  struct addrinfo *server_info = get_server_list (NULL, thePort, true, false, true);
  struct addrinfo *server = NULL;
  int socket_option = 1;
  int socketfd = -1;
  for (server = server_info; server != NULL;server = server->ai_next) // server setup
    {
      if ((socketfd = socket (server->ai_family, server->ai_socktype, 0)) < 0)
        continue;
      setsockopt (socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &socket_option, sizeof(int));
      struct timeval timeout = {10, 0};
      setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof(timeout));
      if (bind (socketfd, server->ai_addr, server->ai_addrlen) == 0)
        break;

      close(socketfd);
      socketfd = -1;
    }
  freeaddrinfo (server_info); 
  if (socketfd < 0) // check if socket succeeded in binding
    {
      printf("Failed to bind socket");
      exit(1);
    }

  listen(socketfd, 10);
  socklen_t addrlen = server->ai_addrlen;
  char *toRet = calloc (20, sizeof(char));
  
  pthread_t child_thread;
  assert (pthread_create (&child_thread, NULL, new_client, NULL) == 0); // create 4 worker threads
  pthread_t child_thread2;
  assert (pthread_create (&child_thread2, NULL, new_client, NULL) == 0);
  pthread_t child_thread3;
  assert (pthread_create (&child_thread3, NULL, new_client, NULL) == 0); 
  pthread_t child_thread4;
  assert (pthread_create (&child_thread4, NULL, new_client, NULL) == 0); 

  while(1) // wait for incoming connections
    {
      struct sockaddr *address = calloc (1, (size_t) addrlen);
      assert (address != NULL);
      int connection;
      if (queue_length < 100) // dont accept connections if the queue is full
        {
          if ((connection = accept (socketfd, address, &addrlen)) >= 0)
            {
              pthread_mutex_lock (&queue_lock);
              fdqueue[queue_inspos] = connection;
              if (queue_inspos != 99) // if queue is max reset
                {
                  queue_inspos++;
                }
              else
                {
                  queue_inspos = 0;
                }
              queue_length++;
              pthread_mutex_unlock (&queue_lock);
              //printf("Con: %d\n", fdqueue[queue_curpos]); 
              sem_wait(&child_num); // wait till a child is good
              sem_post(&current_reqs); // show a new conection 

              //pthread_t child_thread;
              //assert (pthread_create (&child_thread, NULL, new_client, (void*)connection) == 0);
              //pthread_join (child_thread, NULL);

            }
        }
      //close (connection);
      free (address);
      //break;
    }
  shutdown(socketfd, SHUT_RDWR); // cleanup stuff
  close (socketfd);
  return toRet;
}

/*
* Read in the data request and store it for later parsing.
*/
request_t
read_data_request (char *buffer, size_t req_len, int sockfd)
{
  ssize_t in = read (sockfd, buffer, req_len);
  char type = buffer[0];
  request_t toRet = UNDEF;
  switch (type)
    {
      case 'S':
        toRet = SEARCH;
        break;
      case 'L':
        toRet = LOAD;
        break;
      case 'U':
        toRet = UNLOAD;
        break;
      case 'B':
        toRet = BUILD;
        break;
      case 'D':
        toRet = DESTROY;
        break;
      default:
        break;
    }
  return toRet;
  //printf("NumRead:%ld\n", in);
}

/*
* Write the proper data acknoledgement message back to the client.
*/
void
write_ack_message (int sockfd, request_t type)
{
  //search request
  char *message = NULL;
  if (type == SEARCH)
    {
      message = malloc (30);
      strncpy (message, "Data Search Request RECEIVED", 30);
      //char *message = "Data Search Request RECEIVED";
      write (sockfd, message, 30);
      free (message);
    }
  else if (type == LOAD)
    {
      char *message = "Data Set Load Request RECEIVED";
      write (sockfd, message, 31);
    }
  else if (type == UNLOAD)
    {
      char *message = "Data Set Unload Request RECEIVED";
      write (sockfd, message, 33);
    }
  else if (type == BUILD)
    {
      char *message = "Data Index Build Request RECEIVED";
      write (sockfd, message, 34);
    } 
  else if (type == DESTROY)
    {
      char *message = "Data Index Destroy Request RECEIVED";
      write (sockfd, message, 36);
    }   
}

void
write_err_message (int sockfd, request_t type)
{
  //search request
  if (type == SEARCH)
    {
      char *message = "FAILED Data Search";
      write (sockfd, message, 18);
    }
  else if (type == LOAD)
    {
      char *message = "Data Set Load FAILED";
      write (sockfd, message, 20);
    }
  else if (type == UNLOAD)
    {
      char *message = "Data Set Unload FAILED";
      write (sockfd, message, 22);
    }
  else if (type == BUILD)
    {
      char *message = "Data Index Build FAILED";
      write (sockfd, message, 23);
    } 
  else if (type == DESTROY)
    {
      char *message = "Data Index Destroy FAILED";
      write (sockfd, "Data Index Destroy FAILED", 25);
    }   
}


/*
* Put the data packet into the proper form and then write it into the socket.
*/
void
write_data_packet (uint32_t size, char *comp_data, char *hash, int sockfd)
{
  /*
  printf("\nCompressed Data: ");
  for (int i = 0; i < size; i++)
    {
      printf("%c", comp_data[i]);
    }
  printf("\n\n");
  */
  char *full_packet = malloc(72 + size);
  strncpy (full_packet, hash, 64); // put hash in
  //strncpy (&full_packet[64], ID, 4); // put ID in
  snprintf (&full_packet[64], 9, "%08u", size); // put size in
  for (int i = 0; i < size; i++)
    {
      full_packet[72+i] = comp_data[i]; // put all data in
    }
  //printf("Full Packet: %s\n", full_packet);
  write (sockfd, full_packet, (72+size)); // send it over the socket.
  free(full_packet);
}



/**
* Start a new thread for communication with the client.
*/
void*
new_client (void *args)
{
  while (1) // loop and take socket file descriptors from queue
    {
  //int sockfd = (int) args;
  
  sem_wait (&current_reqs); // wait until there is a connection
  //sem_wait (&child_num);
  pthread_mutex_lock (&queue_lock);
  int sockfd = fdqueue[queue_curpos];
  if (queue_curpos != 99) // if its at max reset to front of queue
    {
      queue_curpos++;
    }
  else
    {
      queue_curpos = 0;
    }
  queue_length--;
  pthread_mutex_unlock (&queue_lock);
  //printf("SockFd: %d\n", sockfd); // get the socket fd
  
  char *data_req = malloc (83);
  memset(data_req, 0, 83);
  request_t type = read_data_request (data_req, 83, sockfd); // get the data request
  printf("\nRequest: %s\n", data_req);
  //free (data_req);
  //data_req = NULL;

  //write_ack_message (sockfd, type);
  if (type == UNLOAD)
    {
      pthread_mutex_lock (&unload_lock);
      if (!file_loaded)
        {
          write_err_message (sockfd, type);
        }
      else 
        {
          if (curr_file)
            {
              unload_file(drug_ptr, file_size);
              drug_ptr = NULL;
            }
          else
            { 
              unload_file(airqual_ptr, file_size);
              airqual_ptr = NULL;
            }         
          file_loaded = false;
          write_ack_message (sockfd, type);
        } 
      pthread_mutex_unlock (&unload_lock);
    }
  else if (type == LOAD)
    {
      char *data_file_name = malloc(15);
      strncpy(data_file_name, &data_req[5], 15);
      pthread_mutex_lock (&load_lock); 
      if (((strncmp("drugs.csv", data_file_name, 9) != 0) && (strncmp("air_quality.csv", data_file_name, 15) != 0)) || file_loaded)
        {
          write_err_message (sockfd, type);
        }
      else
        {
          if (data_file_name[0] == 'd')
            {
              //printf("\nFIle: %s\n", data_file_name); 
              curr_file = true; // set to drugs.csv
              drug_ptr = load_file(data_file_name, &file_size);
              file_loaded = true;
              //printf("\nOh no: %s\n", drug_ptr);
            }
          else
            {
              printf("\nFIle: %s\n", data_file_name); 
              curr_file = false; // set to air_quality.csv
              airqual_ptr = load_file(data_file_name, &file_size);
              file_loaded = true;
            }
          //free (data_file_name);
          //data_file_name = NULL;
          write_ack_message (sockfd, type); 
        }
      pthread_mutex_unlock (&load_lock);
      free (data_file_name);
      data_file_name = NULL;
    }
  else if (type == BUILD)
    {
      char *field_name = malloc(10);
      strncpy(field_name, &data_req[6], 10); 
      printf("\nField: %s\n", field_name);
      bool already_loaded = false;
      if (file_loaded) // Is there a file loaded 
        {
          if (curr_file) // drugs.csv case
            {
              char fieldID = field_name[0];
              switch (fieldID)
                {
                  case 'S': 
                    if (pthread_mutex_trylock (&state_lock) == 0) // try to aquire it if not exit
                      {
                        if (trees[0] == NULL) // state case
                          {
                            trees[0] = build_tree(field_name, file_size, drug_ptr);
                            printf("Sucess Building\n");
                          }
                        else
                          {
                            already_loaded = true;
                          }  
                        pthread_mutex_unlock (&state_lock); 
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break;
                  case 'Y':
                    if (pthread_mutex_trylock (&year_lock) == 0)
                      {
                        if (trees[1] == NULL) // Year case
                          {
                            trees[1] = build_tree(field_name, file_size, drug_ptr);
                            printf("Sucess Building\n");
                          }
                        else
                          {
                            already_loaded = true;
                          }  
                        pthread_mutex_unlock (&year_lock);
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break; 
                  case 'M':
                    if (trees[2] == NULL) // Month case
                      {
                        trees[2] = build_tree(field_name, file_size, drug_ptr);
                        printf("Sucess Building\n");
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break;
                  case 'D':
                    if (pthread_mutex_trylock (&data_lock) == 0)
                      {
                        if (trees[3] == NULL) // Year case
                          {
                            trees[3] = build_tree(field_name, file_size, drug_ptr);
                            printf("Sucess Building\n");
                          }
                        else
                          {
                            already_loaded = true;
                          }  
                        pthread_mutex_unlock (&data_lock);
                      }
                    else
                      {
                        already_loaded = true;
                      } 
                    break;  
                  default:
                    already_loaded = true;
                    break;
                }
            }
          else // Dealing with air_quality.csv
            {
              char fieldID = field_name[0]; 
              switch (fieldID)
                {
                  case 'S': 
                    if (trees[4] == NULL) // StateName case
                      {
                        trees[4] = build_tree(field_name, file_size, airqual_ptr);
                        printf("Sucess Building\n");
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break;
                  case 'C':
                    if (trees[5] == NULL) // CountyName case
                      {
                        trees[5] = build_tree(field_name, file_size, airqual_ptr);
                        printf("Sucess Building\n");
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break; 
                  case 'R':
                    if (trees[6] == NULL) // ReportYear case
                      {
                        trees[6] = build_tree(field_name, file_size, airqual_ptr);
                        printf("Sucess Building\n");
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break; 
                  case 'V':
                    if (trees[7] == NULL) // Value case
                      {
                        printf("Value Tree\n");
                        trees[7] = build_tree(field_name, file_size, airqual_ptr);
                        printf("Sucess Building\n");
                      }
                    else
                      {
                        already_loaded = true;
                      }
                    break;  
                  default:
                    already_loaded = true;
                    break;
                }
            }
          if (!already_loaded) // check if tree already exists
            {
              write_ack_message (sockfd, type); 
            }
          else 
            {
              write_err_message (sockfd, type);
            }
        }
      else // No file loaded in
        {
          write_err_message (sockfd, type);
        }
      free (field_name);
    }
  else if (type == DESTROY)
    {
      char *field_name = malloc(10);
      strncpy(field_name, &data_req[8], 10); 
      printf("\nField: %s\n", field_name);
      int gone = 0;
      if (file_loaded)
        {
          if (curr_file) // drugs.csv case
            {
              char fieldID = field_name[0];
              switch (fieldID)
                {
                  case 'S': 
                    if (pthread_mutex_trylock(&state_lock) == 0) // try to aquire if not exit.
                      {
                        gone = destroy_tree(trees[0]);
                        trees[0] = NULL;
                        printf("Success Destroying\n");
                        pthread_mutex_unlock(&state_lock);
                      }
                    else 
                      {
                        gone = -1;
                      }
                    break;
                  case 'Y': 
                    if (pthread_mutex_trylock(&year_lock) == 0)
                      {
                        gone = destroy_tree(trees[1]);
                        trees[1] = NULL;
                        printf("Success Destroying\n");
                        pthread_mutex_unlock(&year_lock);
                      }
                    else 
                      {
                        gone = -1;
                      } 
                    break;
                  case 'M': 
                    gone = destroy_tree(trees[2]);
                    trees[2] = NULL;
                    printf("Success Destroying\n");
                    break;
                  case 'D': 
                    if (pthread_mutex_trylock(&data_lock) == 0)
                      {
                        gone = destroy_tree(trees[3]);
                        trees[3] = NULL;
                        printf("Success Destroying\n");
                        pthread_mutex_unlock(&data_lock);
                      }
                    else 
                      {
                        gone = -1;
                      }  
                    break;
                  default:
                    gone = -1;
                    break;
                }
            }
          else // air_quality.csv case
            {
              char fieldID = field_name[0];
              switch (fieldID)
                {
                  case 'S': 
                    gone = destroy_tree(trees[4]);
                    trees[4] = NULL;
                    printf("Success Destroying\n");
                    break;
                  case 'C': 
                    gone = destroy_tree(trees[5]);
                    trees[5] = NULL;
                    printf("Success Destroying %d\n", gone);
                    break;
                  case 'R': 
                    gone = destroy_tree(trees[6]);
                    trees[6] = NULL;
                    printf("Success Destroying\n");
                    break;
                  case 'V': 
                    gone = destroy_tree(trees[7]);
                    trees[7] = NULL;
                    printf("Success Destroying\n");
                    break;
                  default:
                    gone = -1;
                    break;
                } 
            }
          if (gone == -1) // tree was already NULL
            {
              write_err_message (sockfd, type);
            }
          else
            {
              write_ack_message (sockfd, type); 
            }
        }
      else // cant destroy a tree with no file
        {
          write_err_message (sockfd, type);
        } 
      free (field_name);
    }
  else if (type == SEARCH) // Data search case
    {
      
      char *full_search_req = malloc(44);
      strncpy(full_search_req, &data_req[7], 44);
      size_t search_len = strlen(full_search_req);
      char search_field[44];
      char *search_val = malloc(23);
      bool equals_found = false;
      int val_index = 0;
      for (int i = 0; i < search_len; i++) // parse input for field and value
        {
          if (!equals_found)
            {
              if (full_search_req[i] != '=')
                {
                  search_field[i] = full_search_req[i];
                }
              else
                {
                  equals_found = true;
                } 
            }
          else
            {
              search_val[val_index] = full_search_req[i];
              val_index++;
            }
        }
      printf("\nField: %s  Val: %s\n", search_field, search_val);
      
      char *search_res = NULL;
     
      if (curr_file)
        { 
          if (strncmp(search_field, "State", 5) == 0)
            {
              printf("\nReached Search\n");
              search_res = search_tree (trees[0], drug_ptr, file_size, search_val);
              printf("\nSearch After \n");
            }
          else if (strncmp(search_field, "Year", 4) == 0)
            {
              printf("Year search\n");
              search_res = search_tree (trees[1], drug_ptr, file_size, search_val);
            }
          else if (strncmp(search_field, "Month", 5) == 0)
            {
              printf("Month search\n");
              search_res = search_tree (trees[2], drug_ptr, file_size, search_val);
            }
          else if (strncmp(search_field, "Data Value", 10) == 0)
            {
              printf("Data Value search\n");
              search_res = search_tree (trees[3], drug_ptr, file_size, search_val);
            }
        }
      else 
        {
          if (strncmp(search_field, "StateName", 9) == 0)
            {
              printf("\nStateValue Search\n");
              search_res = search_tree (trees[4], airqual_ptr, file_size, search_val);
              printf("\nSearch After \n");
            }
          else if (strncmp(search_field, "ReportYear", 10) == 0)
            {
              printf("StateValue search\n");
              search_res = search_tree (trees[6], airqual_ptr, file_size, search_val);
            }
          else if (strncmp(search_field, "CountyName", 10) == 0)
            {
              printf("CountyName search\n");
              search_res = search_tree (trees[5], airqual_ptr, file_size, search_val);
            }
          else if (strncmp(search_field, "Value", 5) == 0)
            {
              printf("Value search\n");
              search_res = search_tree (trees[7], airqual_ptr, file_size, search_val);
            } 
        }

      
      if (search_res == NULL || !file_loaded || !equals_found) // check for invlids
        {
          write_err_message (sockfd, type);
        }
      else
        {
          write_ack_message (sockfd, type);  
      
      //char *data = malloc(62);
      //strncpy(data, "datadatadata11111222223333344444555556666677777888889999900000", 62);
          size_t data_len = strlen(search_res);
          printf("\nData Length: %lu\n", data_len);
          char *hash = doHash(search_res, data_len);
          printf("\nHash: %s\n", hash);
          uint32_t comp_len = 0;
      

          char *comp_data_hold = compress(search_res, data_len, &comp_len);
      
      
          write_data_packet(comp_len, comp_data_hold, hash, sockfd);
            //write_ack_message (sockfd, type);
            // send the whole data
            // packet
          free(hash);
          free(comp_data_hold);
          hash = NULL;
          comp_data_hold = NULL;
        }
      if (search_res)
        {
          free(search_res);
        }
      free(search_val);
      free(full_search_req);
     
      //free(data);
      
      search_res = NULL;
      search_val = NULL;
      full_search_req = NULL;
     

      //data = NULL;
    }
  free (data_req);
  data_req = NULL;

  close (sockfd);
  sem_post (&child_num);
  //pthread_exit (NULL);
    }
}

