/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  config.c: Config functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

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
read_services_conf(int cold)
{ 
  conf_curctx.f = fbopen(ServicesState.configfile, "r");
  if(!conf_curctx.f)
  {
    parse_error("Cannot open %s", ServicesState.configfile);
    return;
  }

  conf_cold = cold;
  execute_callback(reset_conf);

  conf_pass = 1;
  conf_curctx.filename = ServicesState.configfile;
  conf_curctx.lineno = 0;
  conf_linebuf[0] = 0;
  yyparse();

  conf_pass = 2;
  execute_callback(switch_conf_pass);
  fbrewind(conf_curctx.f);
  conf_curctx.lineno = 0;
  conf_linebuf[0] = 0;
  yyparse();

  execute_callback(verify_conf);
  conf_pass = 0;
}
