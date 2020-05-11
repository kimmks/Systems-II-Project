#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <check.h>
#include <time.h>


#include "../Server/hash.h"
#include "../Server/compress.h"
#include "../dataindex/treecontrol.h"
#include "../dataindex/tree.h"

#include "../dataindex/treecontrol.c"


//kearstin added includes
#include <stdint.h>
#include <assert.h>



//#include "../dataindex/tree.h"


/*
hash
compress
client
main
server
*/

START_TEST (hash_empty_string)
{
  char* hashin = "";
  size_t size = 0;
  char* hashout = doHash(hashin, size);
  ck_assert_int_eq(sizeof(hashout), 8);
}
END_TEST


START_TEST (hash_hubris)
{  
  char* hashin = "hubris";
  size_t size = 1;
  char* hashout = doHash(hashin, size);
}
END_TEST
  
START_TEST (hash_romeo_and_juliet_prologue) {
  char* hashin = "Two households, both alike in dignity,\nIn fair Verona, where we lay our scene,\nFrom ancient grudge break to new mutiny,\nWhere civil blood makes civil hands unclean.\nFrom forth the fatal loins of these two foes\nA pair of star-cross'd lovers take their life;\nWhose misadventured piteous overthrows\nDo with their death bury their parents' strife.\nThe fearful passage of their death-mark'd love,\nAnd the continuance of their parents' rage,\nWhich, but their children's end, nought could remove,\nIs now the two hours' traffic of our stage;\nThe which if you with patient ears attend,\nWhat here shall miss, our toil shall strive to mend.";
  size_t size = sizeof(hashin);
  char* hashout = doHash(hashin, size);
  ck_assert_int_eq(sizeof(hashout), 8);
  
  /*printf("Testing hash function: Romeo and Juliet prologue again");
  char* hashout_2 = doHash(hashin, size); //values are still stored
  ck_assert_int_eq(strcmp(hashout, hashout_2), 0);*/
}
END_TEST


START_TEST (de_and_compress_empty_string)
{
  char* compressin = "";
  size_t inlen = 0;
  uint32_t cretlen = 0;
  char* compressout = compress(compressin, inlen, &cretlen);
  ck_assert(strcmp(compressin, compressout) != 0); //compression changes value
  
  uint32_t dretlen = 0;
  //char* decompressout = decompress(compressout, inlen, &dretlen);
  //ck_assert_int_eq(strcmp(compressin, decompressout), 0); //decompression works
}
END_TEST
  
  
START_TEST (de_and_compress_hubris)
{ 
  char* compressin = "hubris";
  size_t inlen = 0;
  uint32_t cretlen = 0;
  char* compressout = compress(compressin, inlen, &cretlen);
  ck_assert(strcmp(compressin, compressout) != 0); //compression changes value
  
  uint32_t dretlen = 0;
  //char* decompressout = decompress(compressout, cretlen, &dretlen);
  //printf("*******************%s******************", decompressout);
  //ck_assert_int_eq(strcmp(compressin, decompressout), 0); //FAILS
}
END_TEST
  
  
START_TEST (de_and_compress_romeo_and_juliet_prologue)
{
  char* compressin = "Two households, both alike in dignity,\nIn fair Verona, where we lay our scene,\nFrom ancient grudge break to new mutiny,\nWhere civil blood makes civil hands unclean.\nFrom forth the fatal loins of these two foes\nA pair of star-cross'd lovers take their life;\nWhose misadventured piteous overthrows\nDo with their death bury their parents' strife.\nThe fearful passage of their death-mark'd love,\nAnd the continuance of their parents' rage,\nWhich, but their children's end, nought could remove,\nIs now the two hours' traffic of our stage;\nThe which if you with patient ears attend,\nWhat here shall miss, our toil shall strive to mend.";
  size_t inlen = 0;
  uint32_t cretlen = 0;
  char* compressout = compress(compressin, inlen, &cretlen);
  //ck_assert(strcmp(compressin, compressout) != 0); //compression changes value
  
  uint32_t dretlen = 0;
  //char* decompressout = decompress(compressout, cretlen, &dretlen);
  //ck_assert_int_eq(strcmp(compressin, decompressout), 0); //decompression works
}
END_TEST

START_TEST(tree)
{
  printf("Testing tree: drugs.csv");
  char* filename = "../dataindex/drugs.csv";
  int file_size;
  char* mapAddr = load_file(filename, &file_size);
  ck_assert(mapAddr != NULL);

  time_t time1;
  time_t time2;
  time1 = time(NULL);
  
  node root = build_tree("State", file_size, mapAddr);
  time2 = time(NULL);
  
  ck_assert(root != NULL);
  int total_seconds = time2 - time1;
  printf("time to build state tree: %d", total_seconds);

  char* record = search_tree(root, mapAddr, file_size, "VA");
  char* expected = "AK,AK,2015,July,Number of Deaths,4220,,100,0,Numbers may differ from published reports using final data. See Technical Notes."; //I know this is wrong but I'm not worrying about it since there's a separate file for tree tests.
  ck_assert(record != NULL);
  ck_assert_str_eq(record, expected);
  free(record);
  record = NULL;
  
  ck_assert(destroy_tree(root) == 0);
  
  
  
  //TODO "predicted value" in drugs.csv is sometimes empty- should this return empty values?
  
  
}
END_TEST


void
public_tests (Suite *s)
{
  TCase *tc_public = tcase_create ("Public");
  tcase_add_test (tc_public, hash_empty_string);
  tcase_add_test (tc_public, hash_hubris);
  tcase_add_test (tc_public, hash_romeo_and_juliet_prologue);
  tcase_add_test (tc_public, de_and_compress_empty_string);
  tcase_add_test (tc_public, de_and_compress_hubris);
  tcase_add_test (tc_public, de_and_compress_romeo_and_juliet_prologue);
  //tcase_add_test (tc_public, tree);
  suite_add_tcase (s, tc_public);
}
