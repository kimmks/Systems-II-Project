#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

/**
  Header file for treecontrol.c
**/

char* load_file(char *filename, int *file_size);

int destroy_tree(node root);

char *search_tree(node root, char *mapAddr, int file_size, char *value);

node build_tree(char *field_name, int file_size, char *mapAddr);

int unload_file(char *mapAddr, int file_size);
