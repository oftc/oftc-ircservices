/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  lexer.l: A set of rules for flex.
 *
 *  Copyright (C) 2003 by Piotr Nizynski, Advanced IRC Services Project
 *  Copyright (C) 2005 by the Hybrid Development Team.
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

%option case-insensitive
%option noyywrap
%option nounput

%{

#include "stdinc.h"
#include "conf/conf.h"
#include "y.tab.h"

int conf_include_sptr = 0;

#undef  YY_INPUT
#define YY_INPUT(buf, res, siz) res = conf_yy_input(buf, siz)
/* flex's default is to print a message and exit(0) */
#define YY_FATAL_ERROR(msg) conf_yy_fatal_error(msg)

#define MAX_INCLUDE_DEPTH 10

YY_BUFFER_STATE conf_include_yystack[MAX_INCLUDE_DEPTH];
struct ConfParserContext conf_include_ctxstack[MAX_INCLUDE_DEPTH];

static void conf_include(void);
static int conf_eof(void);
static int conf_yy_fatal_error(const char *msg)
{
  return(0);
}

%}

ws		[ \t]*
include		\.include{ws}(\<.*\>|\".*\")
qstring		\"[^\"]*\"
identifier	[a-zA-Z_][a-zA-Z0-9_:]*

%%

{include}	{ conf_include(); }
"/*"		{
              int c;

              while (1)
              {
                while ((c = yyinput()) != '*' && c != EOF)
                  if (c == '\n')
                  {
                    conf_curctx.lineno++;
                    conf_linebuf[0] = '\0';
                  }
                while (c == '*')
                  c = yyinput();
                if (c == '/')
                  break;
                if (c == EOF)
                {
                  yyerror("EOF in comment");
                  break;
                }
              }
        	}
\n.*		{
              conf_curctx.lineno++;
              strlcpy(conf_linebuf, yytext + 1, CONF_BUFSIZE);
              yyless(1);
            }
{ws}		;
#.*		{
          if (!strncasecmp(yytext, "#include", 8))
          yyerror("You probably meant '.include', ignoring");
		}
[0-9]+		{ yylval.number = atoi(yytext); return NUMBER; }
{qstring}	{
              yytext[yyleng - 1] = '\0';
              yylval.string = yytext + 1;
              return QSTRING;
            }
b           { return BYTES; }
byte        { return BYTES; }
bytes       { return BYTES; }
d           { return DAYS; }
day	        { return DAYS; }
days        { return DAYS; }
false		{ yylval.number = 0; return NUMBER; }
h           { return HOURS; }
hour		{ return HOURS; }
hours		{ return HOURS; }
kb          { return KBYTES; }
kbyte		{ return KBYTES; }
kbytes		{ return KBYTES; }
kilobyte	{ return KBYTES; }
kilobytes	{ return KBYTES; }
mb          { return MBYTES; }
mbyte		{ return MBYTES; }
mbytes		{ return MBYTES; }
megabyte	{ return MBYTES; }
megabytes	{ return MBYTES; }
m           { return MINUTES; }
min         { return MINUTES; }
mins		{ return MINUTES; }
minute		{ return MINUTES; }
minutes		{ return MINUTES; }
no          { yylval.number = 0; return NUMBER; }
off         { yylval.number = 0; return NUMBER; }
on          { yylval.number = 1; return NUMBER; }
s           { return SECONDS; }
sec         { return SECONDS; }
secs		{ return SECONDS; }
second		{ return SECONDS; }
seconds		{ return SECONDS; }
true		{ yylval.number = 1; return NUMBER; }
w           { return WEEKS; }
week		{ return WEEKS; }
weeks		{ return WEEKS; }
yes         { yylval.number = 1; return NUMBER; }
{identifier}	{ yylval.string = yytext; return IDENTIFIER; }
.           { return yytext[0]; }
<<EOF>>		{ if (conf_eof()) yyterminate(); }

%%

/*
 * conf_include()
 *
 * Enters a new configuration file.
 *
 * inputs: none
 * output: none
 */
static void
conf_include(void)
{
  char *fname, *buf;
  FBFILE *f;

  if (conf_include_sptr == MAX_INCLUDE_DEPTH)
  {
    yyerror("includes nested too deep");
    return;
  }

  if ((fname = strchr(yytext, '"')) == NULL)
    *strchr(fname = strchr(yytext, '<') + 1, '>') = '\0';
  else
    *strchr(++fname, '"') = '\0';

  if (fname[0] == '/')
    f = fbopen(fname, "r");
  else
  {
    buf = (char *)MyMalloc(strlen(ETCPATH) + 1 + strlen(fname) + 1);
    sprintf(buf, "%s/%s", ETCPATH, fname);
    f = fbopen(buf, "r");
    MyFree(buf);
  }

  if (f == NULL)
  {
    yyerror("cannot open file to include");
    return;
  }

  /* save current state */
  memcpy(&conf_include_ctxstack[conf_include_sptr], &conf_curctx,
         sizeof(struct ConfParserContext));
  conf_include_yystack[conf_include_sptr++] = YY_CURRENT_BUFFER;

  /* switch lexer context */
  conf_linebuf[0] = '\0';
  conf_curctx.f = f;
  DupString(conf_curctx.filename, fname);
  conf_curctx.lineno = 1;
  yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
}

/*
 * conf_eof()
 *
 * Ends processing of a configuration file.
 *
 * inputs: none
 * output: 1 if the parser should be exited, 0 otherwise
 */
static int
conf_eof(void)
{
  if (conf_include_sptr == 0)
    return 1;

  /* destroy current buffer */
  yy_delete_buffer(YY_CURRENT_BUFFER);
  fbclose(conf_curctx.f);

  /* restore old context */
  conf_linebuf[0] = '\0';
  memcpy(&conf_curctx, &conf_include_ctxstack[--conf_include_sptr],
         sizeof(struct ConfParserContext));
  yy_switch_to_buffer(conf_include_yystack[conf_include_sptr]);

  return 0;
  yy_fatal_error(NULL);  /* use it somewhere to avoid the warning */
}
