#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
  Algorithm to map a CSV file to memory and then track through the
  file, storing the byte offset where each new record begins.
**/
#include "csvindexer.h"

/**
 Tracks through the mapped file once to count the number of records, and
 returns that number for later use in indexing the mapped file.
**/
int 
get_num_records(char *mapAddr, size_t file_size)
{
    int	num_records = -1;
    if (mapAddr == NULL) {
	return num_records;
    }
    for (int i = 0; i < file_size; i++) {
	if (mapAddr[i] == '\n' || mapAddr[i] == '\r') {
	    num_records++;
	}
    }
    return num_records + 1;
}

/**
  Tracks through the mapped file space and fills in the offsets
  array with the byte offset for each record, based on encountering new line characters.

**/
int
index_mapped_file(char *mapAddr, size_t file_size, int offsets[OFFSET_SIZE],
		  int offsets_size)
{

    if (mapAddr == NULL || offsets == NULL) {
	return -1;
    }
    offsets[0] = 0;
    int	offsetIndex = 1;
    for (int i = 0; i < file_size; i++) {
	if (mapAddr[i] == '\n' || mapAddr[i] == '\r') {
	    i++;
	    offsets[offsetIndex] = i;
	    offsetIndex++;
	}
    }

    return 0;
}
