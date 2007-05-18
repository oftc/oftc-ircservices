/*
 *  database.h: Defines database{} conf section.
 *
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

#ifndef INCLUDED_conf_database_h
#define INCLUDED_conf_database_h

#include <yada.h>

struct DatabaseConf
{
  char *driver;
  char *dbname;
  char *hostname;
  char *username;
  char *password;
  int port;
  yada_t *yada;
};

EXTERN struct DatabaseConf Database;

#ifdef IN_CONF_C
void init_database(void);
void cleanup_database(void);
#endif

#endif /* INCLUDED_conf_database_h */
