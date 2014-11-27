#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
   char *input;
   char *data;
   char name[64];
   char pass[64];
   int i = 0;
   int j = 0;

   printf("Content-type: text/html\n\n");
   printf("The following is query reuslt:<br><br>");
   
   data = getenv("QUERY_STRING");

   printf("data is -----\n");
   
   return 0;
}