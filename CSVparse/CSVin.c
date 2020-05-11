#include "CSVin.h"


/*
  parseLine- this function is called after the user calls the getLine() operation.
  It takes a char * and parses the line on the commas. Special cases for empty
  categories and quotes have been dealt with.

  *line- the line to parse(should be a CSV file line)
  *numCategories- the number of categories inside this line, when dealing
     with the same file this will not change from call to call, however it is
     important in regard to later functions.
  lineLength- the length of the line passed. (use strLen(line) when calling)

  return- a char** that contains each of the cateogories in their own char*
    empty categories are notated by a space character and nothing else.
    the number of categories in the return in stores in the numCategories
    pointer.
*/
char**
parseLine (char *line, int *numCategories, int lineLength)
{
  
  
  int numCommas = 0;
  int commaPositions[lineLength]; // store indexes of commas
  
  bool quotes = false;
  for (int i = 0; i < lineLength; i++) // gets the number of quotes in a line 
    {
      if (line[i] == '\"')
        {
          quotes = !quotes;
        }
      if (line[i] == ',' && !quotes) // quotes special case
        {
          commaPositions[numCommas] = i;
          numCommas++;
        }
    }
  
  int numCats = numCommas + 1;
  *numCategories = numCats;
  char **toRet = (char**) malloc(numCats*sizeof(char*));
  int currComma = 1;
  int distance = 0;
  int currWord = 0;
  
  
  if (commaPositions[0] == 0) // check if the first category is empty 
    {
      char *firstWord = " ";
      toRet[currWord] = firstWord;
      currWord++;
    }
  else 
    {
      char *firstWord = (char*) malloc(commaPositions[0] - 1); 
      for (int i = 0; i < commaPositions[0]; i++) // get first word of line
        {
          firstWord[i] = line[i];
        }
     toRet[currWord] = firstWord;
     currWord++;
     }

  for (int i = commaPositions[0]; i < commaPositions[numCommas - 1] + 1; i++) 
    {  // go through all words except first and last  
           
      if (i == commaPositions[currComma]) // get word
        {
          // update to next comma location
          distance = (commaPositions[currComma] - commaPositions[currComma - 1]);
          if (distance == 1) // empty category case 
            {
              char *space = " ";
              toRet[currWord] = space;
            }
          else
            {
              char *holder = (char*) malloc(distance); // spaces between commas and then allocated array
              int index = 0;
          
              for (int j = commaPositions[currComma-1] + 1; j < commaPositions[currComma]; j++)
                { 
                  //printf("%c", line[j]);
                  holder[index] = line[j]; // copy chars in
                  index++; 
            
                }
              toRet[currWord] = holder;
              //free (holder);
              //holder = NULL;
            }
          distance = 0;          
          currWord++;
          currComma++;
        }
    }

  if ((lineLength - 1) - commaPositions[numCommas-1] == 1) // check if last cat is empty.
    {
      char *lastWord = " ";
      toRet[currWord] = lastWord;
      currWord++;
    }
  else
    {
      char *lastWord = (char*) malloc(lineLength - commaPositions[numCommas-1] - 1); 
      int index = 0;
      for (int i = commaPositions[numCommas-1] + 1; i < lineLength - 1; i++) // get last word of line
        {
          lastWord[index] = line[i];
          index++;
        }
      toRet[currWord] = lastWord;
      currWord++;
    }
  

  return toRet; 
}





