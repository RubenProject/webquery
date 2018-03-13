#include <stdio.h>
#define MAXBUFFER 1000
// A trivial program that outputs the first argument to the program

main(int argc, char *argv[])
{
  FILE *fp;
  char queryterms[MAXBUFFER];
  fp=fopen("queryterms.txt","r");
  fgets(queryterms,MAXBUFFER,fp);
  fclose(fp);
  printf("\nquery is %s \n\n",queryterms);
}
