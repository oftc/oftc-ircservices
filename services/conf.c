#include "stdinc.h"

FBFILE *conf_fbfile_in;
extern char linebuf[];

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

  printf("Oh, dear. %s %s\n", msg, newlinebuf);
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

