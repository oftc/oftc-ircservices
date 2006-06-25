#include "stdinc.h"
#include "conf/conf.h"

extern struct ConfParserContext conf_curctx;
extern int yyparse(); /* defined in y.tab.c */
extern int lineno;
int scount = 0; /* used by yyparse(), etc */

int
conf_fbgets(char *lbuf, int max_size, FBFILE *fb)
{
  char *buff;

  if ((buff = fbgets(lbuf, max_size, fb)) == NULL)
    return(0);

  return(strlen(lbuf));
}

void
read_services_conf(int cold_start)
{ 
  FBFILE *file;

  if ((file = fbopen(CPATH, "r")) == NULL)
  {
    printf("config file not found!\n");
    exit(0);
  }

  conf_curctx.filename = CPATH;
  conf_curctx.f = file;
  conf_curctx.lineno = 1;

  conf_pass = 1;
  yyparse();        /* pick up the classes first */

  fbrewind(file);

  conf_curctx.lineno = 1;
  conf_pass = 2;
  yyparse();          /* Load the values from the conf */

  execute_callback(verify_conf);
}
