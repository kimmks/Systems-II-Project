#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/**
  Header file for csvindexer.c
**/
int OFFSET_SIZE;

int get_num_records (char *mapAddr, size_t file_size);

int index_mapped_file (char *mapAddr, size_t file_size,
		     int offsets[OFFSET_SIZE], int offsets_size);
