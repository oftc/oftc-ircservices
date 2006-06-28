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

/*
 * split_nuh
 *
 * inputs - pointer to original mask (modified in place)
 *    - pointer to pointer where nick should go
 *    - pointer to pointer where user should go
 *    - pointer to pointer where host should go
 * output - NONE
 * side effects - mask is modified in place
 *      If nick pointer is NULL, ignore writing to it
 *      this allows us to use this function elsewhere.
 *
 * mask       nick  user  host
 * ---------------------- ------- ------- ------
 * Dianora!db@db.net    Dianora db  db.net
 * Dianora      Dianora * *
 * db.net                       *       *       db.net
 * OR if nick pointer is NULL
 * Dianora      - * Dianora
 * Dianora!     Dianora * *
 * Dianora!@      Dianora * *
 * Dianora!db     Dianora db  *
 * Dianora!@db.net    Dianora * db.net
 * db@db.net      * db  db.net
 * !@       * * *
 * @        * * *
 * !        * * *
 */
void
split_nuh(struct split_nuh_item *const iptr)
{
  char *p = NULL, *q = NULL;

  if (iptr->nickptr)
    strlcpy(iptr->nickptr, "*", iptr->nicksize);
  if (iptr->userptr)
    strlcpy(iptr->userptr, "*", iptr->usersize);
  if (iptr->hostptr)
    strlcpy(iptr->hostptr, "*", iptr->hostsize);

  if ((p = strchr(iptr->nuhmask, '!'))) {
    *p = '\0';

    if (iptr->nickptr && *iptr->nuhmask != '\0')
      strlcpy(iptr->nickptr, iptr->nuhmask, iptr->nicksize);

    if ((q = strchr(++p, '@'))) {
      *q++ = '\0';

      if (*p != '\0')
        strlcpy(iptr->userptr, p, iptr->usersize);

      if (*q != '\0')
        strlcpy(iptr->hostptr, q, iptr->hostsize);
    }
    else {
      if (*p != '\0')
        strlcpy(iptr->userptr, p, iptr->usersize);
    }
  }
  else {
    /* No ! found so lets look for a user@host */
    if ((p = strchr(iptr->nuhmask, '@'))) {
      /* if found a @ */
      *p++ = '\0';

      if (*iptr->nuhmask != '\0')
        strlcpy(iptr->userptr, iptr->nuhmask, iptr->usersize);

      if (*p != '\0')
        strlcpy(iptr->hostptr, p, iptr->hostsize);
    }
    else {
      /* no @ found */
      if (!iptr->nickptr || strpbrk(iptr->nuhmask, ".:"))
        strlcpy(iptr->hostptr, iptr->nuhmask, iptr->hostsize);
      else
        strlcpy(iptr->nickptr, iptr->nuhmask, iptr->nicksize);
    }
  }
}

