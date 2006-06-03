/*
 *  services_lexer.l: Scans the services configuration file for tokens.
 *
 *  Blatently taken from ircd-hybrid - copyright below
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
 *  $Id: ircd_lexer.l 632 2006-06-01 10:53:00Z michael $
 */

%option case-insensitive
%option noyywrap
%option nounput
%option never-interactive

%{
#include "stdinc.h"

#include "y.tab.h"

#undef YY_INPUT
#define YY_FATAL_ERROR(msg) conf_yy_fatal_error(msg)
#define YY_INPUT(buf,result,max_size) \
  if (!(result = conf_fbgets(buf, max_size, conf_fbfile_in))) \
    YY_FATAL_ERROR("input in flex scanner failed"); 
#define MAX_INCLUDE_DEPTH 10


unsigned int lineno = 1;
char linebuf[IRC_BUFSIZE];
char conffilebuf[IRC_BUFSIZE];

static int include_stack_ptr = 0;
static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
static unsigned int lineno_stack[MAX_INCLUDE_DEPTH];
static FBFILE *inc_fbfile_in[MAX_INCLUDE_DEPTH];
static char conffile_stack[MAX_INCLUDE_DEPTH][IRC_BUFSIZE];
static void ccomment(void);
static void cinclude(void);
static int ieof(void);
%}

WS        [[:blank:]]*
DIGIT     [[:digit:]]+
COMMENT   ("//"|"#").*
qstring   \"[^\"\n]*[\"\n]
include   \.include{WS}(\<.*\>|\".*\")

%%
{include}       { cinclude(); }
"/*"	        { ccomment(); }

\n.*	        { strcpy(linebuf, yytext+1); ++lineno; yyless(1); }

{WS}            ;
{COMMENT}       ;

{DIGIT}         { yylval.number = atoi(yytext); return NUMBER; }

{qstring}	{ if (yytext[yyleng-2] == '\\')
		    {
		      yyless(yyleng-1); /* return last quote */
		      yymore();		/* append next string */
		    }
		  else
		    {
		      yylval.string = yytext+1;
		      if(yylval.string[yyleng-2] != '"')
			ilog(L_ERROR, "Unterminated character string");
		      else
			{
			  int i,j;

			  yylval.string[yyleng-2] = '\0'; /* remove close
							   *  quote 
							   */
			  
			  for (j=i=0 ;yylval.string[i] != '\0'; i++,j++)
			    {
			      if (yylval.string[i] != '\\')
				{
				  yylval.string[j] = yylval.string[i];
				}
			      else
				{
				  i++;
				  if (yylval.string[i] == '\0') /* XXX 
								 * should not
								 * happen
								 */
				    {
				      ilog(L_ERROR,
					   "Unterminated character string");
				      break;
				    }
				  yylval.string[j] = yylval.string[i];
				}
			    }
			  yylval.string[j] = '\0';
			  return QSTRING;
			}
		    }
		}

~		{ return NOT; }
database              { return DATABASE;              }
dbname                { return DBNAME;                }
description           { return DESCRIPTION;           }
driver                { return DRIVER;                }
name                  { return NAME;                  }
password              { return PASSWORD;              }
rsa_private_key_file  { return RSA_PRIVATE_KEY_FILE;  }
servicesinfo	        { return SERVICESINFO;          }
sid                   { return SID;                   }
ssl_certificate_file  { return SSL_CERTIFICATE_FILE;  }
username              { return USERNAME;              }
vhost                 { return VHOST;                 }
vhost6                { return VHOST6;                }

weeks			    { return WEEKS; }
week		  	  { return WEEKS; }
days			    { return DAYS; }
day			      { return DAYS; }
hours			    { return HOURS; }
hour			    { return HOURS; }
minutes			  { return MINUTES; }
minute			  { return MINUTES; }
seconds			  { return SECONDS; }
second			  { return SECONDS; }

bytes			    { return BYTES; }
byte			    { return BYTES; }
kilobytes		  { return KBYTES; }
kilobyte		  { return KBYTES; }
kbytes			  { return KBYTES; }
kbyte			    { return KBYTES; }
kb			      { return KBYTES; }
megabytes		  { return MBYTES; }
megabyte	  	{ return MBYTES; }
mbytes			  { return MBYTES; }
mbyte			    { return MBYTES; }
mb			      { return MBYTES; }
gigabytes		  { return GBYTES; }
gigabyte		  { return GBYTES; }
gbytes			  { return GBYTES; }
gbyte			    { return GBYTES; }
gb			      { return GBYTES; }
terabytes		  { return TBYTES; }
terabyte		  { return TBYTES; }
tbytes			  { return TBYTES; }
tbyte			    { return TBYTES; }
tb			      { return TBYTES; }
\.\.          { return TWODOTS; }

.		          { return yytext[0]; }
<<EOF>>       { if (ieof()) yyterminate(); }

%%

/* C-comment ignoring routine -kre*/
static void
ccomment(void)
{
  int c = 0;

  /* log(L_NOTICE, "got comment"); */
  while (1)
  {
    while ((c = input()) != '*' && c != EOF)
      if (c == '\n')
        ++lineno;

    if (c == '*')
    {
      while ((c = input()) == '*')
        /* Nothing */ ;
      if (c == '/')
        break;
      else if (c == '\n')
        ++lineno;
    }

    if (c == EOF)
    {
      YY_FATAL_ERROR("EOF in comment");
      /* XXX hack alert this disables
       * the stupid unused function warning
       * gcc generates 
       */
      if (1 == 0)
        yy_fatal_error("EOF in comment");
      break;
    }
  }
}

/* C-style .includes. This function will properly swap input conf buffers,
 * and lineno -kre */
static void
cinclude(void)
{
  char *p = NULL;

  if ((p = strchr(yytext, '<')) == NULL)
    *strchr(p = strchr(yytext, '"') + 1, '"') = '\0';
  else
    *strchr(++p, '>') = '\0';

  /* log(L_NOTICE, "got include %s!", c); */

  /* do stacking and co. */ 
  if (include_stack_ptr >= MAX_INCLUDE_DEPTH)
    ilog(L_ERROR, "Includes nested too deep in %s", p);
  else
  {
    FBFILE *tmp_fbfile_in = NULL;
    char filenamebuf[IRC_BUFSIZE];

    if (*p == '/')  /* if it is an absolute path */
      snprintf(filenamebuf, sizeof(filenamebuf), "%s", p);
    else
      snprintf(filenamebuf, sizeof(filenamebuf), "%s/%s", ETCPATH, p);

    tmp_fbfile_in = fbopen(filenamebuf, "r");
    
    if (tmp_fbfile_in == NULL)
    {
      ilog(L_ERROR, "Unable to read configuration file '%s': %s",
           filenamebuf, strerror(errno));
      return;
    }

    lineno_stack[include_stack_ptr] = lineno;
    lineno = 1;
    inc_fbfile_in[include_stack_ptr] = conf_fbfile_in;
    strlcpy(conffile_stack[include_stack_ptr], conffilebuf, IRC_BUFSIZE);
    include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
    conf_fbfile_in = tmp_fbfile_in;
    snprintf(conffilebuf, sizeof(conffilebuf), "%s", p);
    yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
  }
}

/* This is function that will be called on EOF in conf file. It will
 * apropriately close conf if it not main conf and swap input buffers -kre
 * */
static int
ieof(void)
{
  /* log(L_NOTICE, "return from include stack!"); */
  if (include_stack_ptr)
    fbclose(conf_fbfile_in);
  if (--include_stack_ptr < 0)
  {
    /* log(L_NOTICE, "terminating lexer"); */
    /* We will now exit the lexer - restore init values if we get /rehash
     * later and reenter lexer -kre */
    include_stack_ptr = 0;
    lineno = 1;
    return 1;
  }

  /* switch buffer */
  /* log(L_NOTICE, "deleting include_stack_ptr=%d", include_stack_ptr); */
  yy_delete_buffer(YY_CURRENT_BUFFER);
  lineno = lineno_stack[include_stack_ptr];
  conf_fbfile_in = inc_fbfile_in[include_stack_ptr];
  strlcpy(conffilebuf, conffile_stack[include_stack_ptr], sizeof(conffilebuf)); 
  yy_switch_to_buffer(include_stack[include_stack_ptr]);

  return 0;
}
