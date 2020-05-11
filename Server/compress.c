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
#include <sys/wait.h>

/*
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
*/
char* compress(char *input, size_t len, uint32_t *retLen) 
{
  //write (STDOUT_FILENO, input, len);
  //printf("%s\n", input);
  FILE *file = fopen ("ID", "w"); // open file to hold data
  fwrite(input, 1, len, file);
  fclose(file);
  int pid = fork();
  if (pid == 0) 
    {
      printf("before exec\n");
      
      char *args[] = {"gzip", "ID", NULL}; // gzip the file
      if (execvp("gzip", args) == -1)
        {
          printf("failed exec\n");
          abort();
        }
      printf("shouldnt be here\n");
      abort();    
    }
  wait(NULL);
  char compID[5]; // change ID to have .gz
  strncpy(compID, "ID", 2);
  strncpy(&compID[2], ".gz", 3);
  FILE *fp = fopen ("ID.gz", "rb");
  if (fp == NULL)
    {
      printf("NullFP\n");
    }
  fseek(fp, 0L, SEEK_END);
  int fileLen = ftell(fp);
  rewind(fp);
  char *buff = malloc(fileLen); // take compressed data out
  fread (buff, fileLen, 1, fp);
  *retLen = fileLen;
  fclose(fp);
  pid = fork();
  if (pid == 0) // remove the file
    {
      char *args[] = {"rm", "ID.gz", NULL};
      execvp("rm", args);
    }
  wait(NULL);
  return buff;

}

char* decompress(char *compressed, size_t len, uint32_t *retlen)
{
  //printf("\nData?: %s\n", compressed);
  char compID[7]; // 4 char for the ID and then three for suffix
  strncpy(compID, "ID", 2);
  strncpy(&compID[2], ".gz", 3); // open with ID.gz
  //printf("\nID: %s\n", compID);
  FILE *file = fopen ("ID.gz", "wb");
  fwrite(compressed, 1, len, file);
  fclose(file);
  int pid = fork();
  if (pid == 0) 
    {
      //printf("before exec\n");
      
      char *args[] = {"gunzip", "ID.gz", NULL}; // decompress data
      if (execvp("gunzip", args) == -1)
        {
          printf("failed exec\n");
          abort();
        }
      printf("shouldnt be here\n");
      abort();    
    }
  wait(NULL);

  FILE *fp = fopen ("ID", "r");
  fseek(fp, 0L, SEEK_END);
  int fileLen = ftell(fp);
  rewind(fp);
  char *buff = malloc(fileLen); // read back the compressed data
  fread (buff, fileLen, 1, fp);
  //for (int i = 0; i < fileLen; i++)
  //  {
  //    printf("%c", buff[i]);
  //  }
  //printf("%s", buff);
  *retlen = fileLen;
  fclose(fp);
  pid = fork();
  if (pid == 0)
    {
      char *args[] = {"rm", "ID", NULL}; // clear the file
      execvp("rm", args);
    }
  return buff; 

}
