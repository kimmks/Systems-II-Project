#ifndef __CSV_h__
#define __CSV_h__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** parseLine(char *line, int *numCategories, int lineLength);

#define MAX_STRINGS 1000000

// Key value pairs that will hold the compression identifiers.
typedef struct keyVal
{
  char *value;
  int key;
} keyVal_t;

// holds the split up string as a line and the number of words in it, like argv/argc
typedef struct compressed_file
{
	int *data;
	int numCats;
	int numRows;
	keyVal_t *keys;
	int numFreqs;
} compressed_file_t;

#endif
