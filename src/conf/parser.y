/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  parser.y: A set of rules for bison.
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

%{

#include "stdinc.h"
#include "conf/conf.h"

#define YY_NO_UNPUT

struct ConfSection *conf_current_section;

static dlink_list conf_list = {NULL, NULL, 0};
static struct ConfField *conf_field;

void conf_clear_list(int free_items)
{
  dlink_node *ptr, *ptr_next;

  DLINK_FOREACH_SAFE(ptr, ptr_next, conf_list.head)
  {
    if (free_items)
      MyFree(ptr->data);
    dlinkDelete(ptr, &conf_list);
    free_dlink_node(ptr);
  }
}

void conf_add_ident(const char *ident)
{
  char *str;

  DupString(str, ident);
  dlinkAdd(str, make_dlink_node(), &conf_list);
}

void conf_add_number(int number)
{
  dlinkAdd((void *) (long) number, make_dlink_node(), &conf_list);
}

%}

%union {
    int number;
    char *string;
}

%token BOOL
%token BYTES
%token DAYS
%token HOURS
%token KBYTES
%token MBYTES
%token NUMBER
%token IDENTIFIER
%token MINUTES
%token QSTRING
%token SECONDS
%token WEEKS

%type <number> BOOL
%type <number> NUMBER
%type <string> IDENTIFIER
%type <string> QSTRING
%type <number> timespec
%type <number> timespec_
%type <number> sizespec
%type <number> sizespec_

%%

conf: /* empty */
      | conf conf_block
      ;

conf_block: IDENTIFIER
{
  if ((conf_current_section = find_conf_section($1)) != NULL)
  {
    if (conf_pass == conf_current_section->pass &&
        conf_current_section->before != NULL)
      conf_current_section->before();
  }
  else
    yyerror("unknown conf section");
} default_field '{' conf_items '}' ';'
{
  if (conf_current_section != NULL)
  {
    if (conf_pass == conf_current_section->pass &&
        conf_current_section->after != NULL)
      conf_current_section->after();
    conf_current_section = NULL;
  }
};

conf_block: error ';';
conf_block: error '}';

timespec_: { $$ = 0; } | timespec;
timespec: NUMBER SECONDS timespec_ { $$ = $1 + $3; }
	  | NUMBER MINUTES timespec_ { $$ = $1*60 + $3; }
	  | NUMBER HOURS timespec_ { $$ = $1*3600 + $3; }
	  | NUMBER DAYS timespec_ { $$ = $1*86400 + $3; }
	  | NUMBER WEEKS timespec_ { $$ = $1*604800 + $3; };

sizespec_: { $$ = 0; } | sizespec;
sizespec: NUMBER BYTES sizespec_ { $$ = $1 + $3; }
	  | NUMBER KBYTES sizespec_ { $$ = $1*1024 + $3; }
	  | NUMBER MBYTES sizespec_ { $$ = $1*1048576 + $3; };

ident_list_atom: IDENTIFIER { conf_add_ident($1); };
ident_list: ident_list_atom
            | ident_list ',' ident_list_atom
            ;

// This is made so that a NUMBER alone doesn't match the rules, to avoid
// conflicts. The NUMBER -> NLIST conversion is later dealt with by conf_assign

number_atom: NUMBER { conf_add_number($1); };
number_range: NUMBER '.' '.' NUMBER
{
  int i;
  
  if ($4 - $1 < 0 || $4 - $1 >= 100)
    yyerror("invalid range");
  else for (i = $1; i <= $4; i++)
    conf_add_number(i);
};
number_item: number_range | number_atom;
number_items: number_item | number_items ',' number_item;
number_list: number_atom ',' number_items
             | number_range ',' number_items
             | number_range
             ;

conf_items: /* empty */
            | conf_items conf_item
            ;

conf_item: IDENTIFIER {
  conf_field = find_conf_field(conf_current_section, $1);
} '=' value ';';

conf_item: error ';';

value: QSTRING {
         conf_assign(CT_STRING, conf_field, $1);
       } | NUMBER {
         conf_assign(CT_NUMBER, conf_field, &$1);
       } | BOOL {
         conf_assign(CT_BOOL, conf_field, &$1);
       } | timespec {
         conf_assign(CT_TIME, conf_field, &$1);
       } | sizespec {
         conf_assign(CT_SIZE, conf_field, &$1);
       } | ident_list {
         conf_assign(CT_LIST, conf_field, &conf_list);
         conf_clear_list(1);
       } | number_list {
         conf_assign(CT_NLIST, conf_field, &conf_list);
         conf_clear_list(0);
       };

default_field: /* empty */
               | _default_field
               ;

_default_field: {
  conf_field = conf_current_section->def_field;
  if (!conf_field && conf_pass == conf_current_section->pass)
    parse_error("Section %s has no default field", conf_current_section->name);
} value;
