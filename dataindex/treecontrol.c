#include "treecontrol.h"
#include "./CSVIndexer/csvindexer.c"
#include "tree.c"
#include "../CSVparse/CSVin.c"
#include <string.h>
#include <stdlib.h>
/**
  TreeControl - acts as a controller for all tree functions.
**/

/**
  Opens the file with the given filename, memory maps it using mmap,
  stores a reference to the beginning of the mapped memory region,
  and indexes the memory-mapped regions.
  Returns a pointer to the memory mapped region, or NULL if errors occurred.
**/
char* load_file(char *filename, int *file_size) {
    
    int fd = open(filename, O_RDONLY);
    if (fd <= -1) {
	return NULL;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) <= -1) {
	    return NULL;
    }
    
    char *mapAddr = mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    *file_size = file_info.st_size;
    printf("%d\n", *file_size);

    close(fd);
    return mapAddr;
}

/**
  Destroys the tree with the given root node.
**/
int destroy_tree(node root) {
  if (root == NULL) {
    return -1;
  }
  deleteTree(root);
  return 0;
}

/**
  Searches the tree with the given root for the given value
  and returns a string that represents the raw record from the
  mapped data set.
  IMPORTANT NOTE: THE RECORD(S) RETURNED ARE DYNAMICALLY ALLOCATED AND 
  MUST BE FREED BY THE CALLER.
**/
char *search_tree(node root, char *mapAddr, int file_size, char *value) {
  char *record = NULL;
  char *copyAddr = mapAddr;
  int *offsets;

  if (root != NULL && value != NULL && copyAddr != NULL) {

    //Index file first
    int OFFSET_SIZE = get_num_records(copyAddr, file_size);
    if (OFFSET_SIZE <= -1) {
	return NULL;
    }

    offsets = (int *)calloc(OFFSET_SIZE, sizeof(int));
    if (index_mapped_file(copyAddr, file_size, offsets, OFFSET_SIZE) <= -1) {
	return NULL;
    } 

    int record_num = -1;
    int rec_size = -1;
    char *record_set = ""; //The collection point for all records to be returned.
    int recset_size = 0; //The size of the record_set.
    char *set_addition = ""; //The buffer to hold the new record to add to the set.
    
    node rec_node = search(root, value); //Access the "first" node found.
    if (rec_node == NULL) { //If the value is not found in the tree.
      int msg_size = strlen("No records were found with a value of .")+strlen(value)+1;
      char *msg = (char *)calloc(msg_size, sizeof(char));
      strncat(msg, "No records were found with a value of ", strlen("No records were found with a value of ")+1);
      strncat(msg, value, strlen(value)+1);
      strncat(msg, ".", 1);
      return msg;
    }
    //Record the node originally returned by search
    record_num = rec_node->record_num;
    rec_size = offsets[record_num+1] - offsets[record_num];

    set_addition = (char *)calloc(rec_size+1, sizeof(char));
    snprintf(set_addition, rec_size+1, "%s", &copyAddr[offsets[record_num]]);

    recset_size = strlen(set_addition)+1;
    record_set = (char *)malloc(recset_size);
    strncat(record_set, set_addition, strlen(set_addition)+1);
    free(set_addition);
    set_addition = NULL;
    
    Dup duplicate_list = rec_node->dups;
    if (duplicate_list != NULL) {  //Are there duplicates?
      while (duplicate_list->next != NULL) { //While there are more records with the same key value
        /**
        For every duplicate node to the one found by search,
           access the record in the mmap and contatenate it onto
           "record".
        **/
         record_num = duplicate_list->record_num;
         rec_size = offsets[duplicate_list->record_num+1] - offsets[duplicate_list->record_num];
         //Should use snprintf or strncat here?
         set_addition = (char *)calloc(rec_size+2, sizeof(char));
         snprintf(set_addition, rec_size+1, "%s", &copyAddr[offsets[duplicate_list->record_num]]);

	 recset_size += strlen(set_addition)+2;
	 record_set = (char *)realloc(record_set, recset_size+1);
         strncat(record_set, set_addition, strlen(set_addition)+1); 
         free(set_addition);
         set_addition = NULL;
       
       duplicate_list = duplicate_list->next;
      }
    }
    record = (char *)calloc(recset_size+1, sizeof(char));
    snprintf(record, recset_size+1, "%s", record_set);
    free(offsets);
    offsets = NULL;
  }

 
  return record;
}

/**
  Builds the tree from the loaded data set based on the 
  field name.
  Returns a "node" type that is the root of the new tree, or NULL if an error occurred.
**/
node build_tree(char *field_name, int file_size, char *mapAddr) {
 
  if (mapAddr == NULL) {
    return NULL;
  }

  char *mmapCopy = mapAddr;
  //Index file first.
  int OFFSET_SIZE = get_num_records(mmapCopy, file_size);
  if (OFFSET_SIZE <= -1) {
    return NULL;
  }

  int *offsets = (int *)calloc(OFFSET_SIZE, sizeof(int));
  if (index_mapped_file(mmapCopy, file_size, offsets, OFFSET_SIZE) <= -1) {
    return NULL;
  }  

  printf("Starting build of tree based on field %s\n", field_name);

  node root = NULL; //"node" is a pointer type to a node_t struct defined in tree.h.
  int cat_num = -1;

  if (strncmp(field_name, "StateName", strlen("StateName")) == 0) {
      cat_num = 5;
    } else if (strncmp(field_name, "CountyName", strlen("CountyName")) == 0) {
      cat_num = 7;
    } else if (strncmp(field_name, "ReportYear", strlen("ReportYear")) == 0) {
      cat_num = 8;
    } else if (strncmp(field_name, "Value", strlen("Value")) == 0) {
      cat_num = 9;
    } else if (strncmp(field_name, "State", strlen("State")) == 0) {
      cat_num = 0;
    } else if (strncmp(field_name, "Month", strlen("Month")) == 0) {
      cat_num = 3;
    } else if (strncmp(field_name, "Year", strlen("Year")) == 0) {
      cat_num = 2;
    } else if (strncmp(field_name, "Data Value", strlen("Data Value")) == 0) {
      cat_num = 5;
    } else {
      return NULL; //Invalid field_name passed.
    }

  for (int i = 2; i < OFFSET_SIZE; i++) {
    // Retrieve record string.
    int rec_size = offsets[i+1] - offsets[i];
    char *record = malloc(sizeof(char)*(rec_size+1));
    snprintf(record, rec_size, "%s", &mmapCopy[offsets[i]]);

    // Parse and convert values.
    int record_num = i;
    int num_cats = 0;
    char **rec_values = parseLine(record, &num_cats, strlen(record));
    char *value = rec_values[cat_num];
    
    // Insert value and byte offset into tree
    root = insert(root, value, record_num);

    free(record);
    record = NULL;
  }
  // At the end of this method being called, "root" should be the root of the new tree.
  free(offsets);
  offsets = NULL;
  return root;
}

/**
  Unloads the current data set (unmaps the memory region)
  and returns 0 if no errors occurred or 
  no data set was loaded in the first place.
**/
int unload_file(char *mapAddr, int file_size) {
  if (mapAddr == NULL) {
    return -1;
  }
  if (munmap(mapAddr, file_size) < 0) {
    return -1;
  }

  return 0;
}

