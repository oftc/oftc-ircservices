#include "stdinc.h"

FBFILE *conf_fbfile_in;
extern char linebuf[];
extern char conffilebuf[IRC_BUFSIZE];
extern int yyparse(); /* defined in y.tab.c */
extern int lineno;
int scount = 0; /* used by yyparse(), etc */
int ypass  = 1; /* used by yyparse()      */

/* yyerror()
 *
 * inputs - message from parser
 * output - NONE
 * side effects - message to opers and log file entry is made
 */
void
yyerror(const char *msg)
{
  char newlinebuf[BUFSIZ];

  strip_tabs(newlinebuf, (const unsigned char *)linebuf, strlen(linebuf));

  printf("%s %s\n", msg, newlinebuf);
}

int
conf_fbgets(char *lbuf, int max_size, FBFILE *fb)
{
  char *buff;

  if ((buff = fbgets(lbuf, max_size, fb)) == NULL)
    return(0);

  return(strlen(lbuf));
}

int
conf_yy_fatal_error(const char *msg)
{
  return(0);
}

void
read_services_conf(int cold_start)
{ 
  char *filename = CPATH;

  conf_fbfile_in = NULL;
  
  strlcpy(conffilebuf, filename, sizeof(conffilebuf));

  if((conf_fbfile_in = fbopen(filename, "r")) == NULL)
  {
    if(cold_start)
    {
      printf("Failed in reading configuration file %s", filename);
      exit(-1);
    }
  }
  
  scount = lineno = 0;
  yyparse();
}
