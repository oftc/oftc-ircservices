/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  manager.h: A header for the configuration manager.
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

#ifndef INCLUDED_conf_manager_h
#define INCLUDED_conf_manager_h

#define CONF_BUFSIZE 512

#define CT_NUMBER 0
#define CT_BOOL   1
#define CT_TIME   2
#define CT_SIZE   3
#define CT_STRING 4
#define CT_LIST   5
#define CT_NLIST  6

typedef void CONFS_HANDLER(void);
typedef void CONFF_HANDLER(void *, void *);

struct ConfParserContext {
  FBFILE *f;
  char *filename;
  int lineno;
};

struct ConfField {
  const char *name;
  int type;
  CONFF_HANDLER *handler;
  void *param;
  dlink_node node;
};

struct ConfSection {
  const char *name;
  CONFS_HANDLER *before;
  CONFS_HANDLER *after;
  struct ConfField *def_field;
  int pass;
  dlink_list fields;
  dlink_node node;
};

void init_conf(void);
void yyerror(const char *);
int yylex(void);
int conf_yy_input(char *, int);
void conf_clear_ident_list(void);

EXTERN int conf_pass, conf_cold;
EXTERN struct ConfParserContext conf_curctx;
EXTERN char conf_linebuf[];
EXTERN int conf_include_sptr;
EXTERN struct Callback *reset_conf;
EXTERN struct Callback *verify_conf;
EXTERN struct Callback *switch_conf_pass;

EXTERN void parse_error(const char *, ...);
EXTERN void parse_fatal(const char *, ...);
EXTERN struct ConfSection *find_conf_section(const char *);
EXTERN struct ConfSection *add_conf_section(const char *, int);
EXTERN void delete_conf_section(struct ConfSection *);
EXTERN struct ConfField *find_conf_field(struct ConfSection *, char *);
EXTERN void conf_assign(int, struct ConfField *, void *);
EXTERN CONFF_HANDLER conf_assign_bool;
EXTERN CONFF_HANDLER conf_assign_number;
EXTERN CONFF_HANDLER conf_assign_string;
EXTERN struct ConfField *add_conf_field(struct ConfSection *, const char *,
  int, CONFF_HANDLER *, void *);
EXTERN void delete_conf_field(struct ConfSection *, struct ConfField *);

#endif /* INCLUDED_conf_manager_h */
