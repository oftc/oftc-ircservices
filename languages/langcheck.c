#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

int main(int parc, char *parv[])
{
  FILE *fptr;
  char line[1024+1];
  char *ptr = line;
  int lineno = 1;
  int cleanfile = TRUE;
  int tabline = TRUE;

  if(parc < 2)
  {
    fprintf(stderr, "Usage: %s <filename>\n", parv[0]);
    exit(1);
  }

  if((fptr = fopen(parv[1], "r")) == NULL)
  {
    fprintf(stderr, "Failed to open: %s\n", parv[1]);
    exit(2);
  }

  ptr = fgets(line, 1024, fptr);
  lineno++;
  printf("This language file contains %s", line);

  while((ptr = fgets(line, 1024, fptr)) != NULL)
  {
    if(line[0] == ' ')
    {
      printf("ERROR: Line %d: Line starts with a space\n", lineno);
      cleanfile = FALSE;
    }
    else if(line[0] == '\n')
    {
      printf("ERROR: Line %d: Blank line\n", lineno);
      cleanfile = FALSE;
    }

    if(line[0] == '\t')
      tabline = TRUE;
    else 
    {
      if(!tabline)
        printf("WARNING: Line %d: Previous section had no definition\n", lineno);
      tabline = FALSE;
    }
    lineno++;
  }

  if(cleanfile)
    printf("File %s is clear of errors\n", parv[1]);
  else
    printf("File %s has errors\n", parv[1]);

  fclose(fptr);

  return 0;
}
