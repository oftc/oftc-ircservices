/*
 *  services_parser.y: Parses the services configuration file.
 *
 *  Blatently taken from ircd-hyrid - copyright below
 *
 *  Copyright (C) 2005 by the past and present ircd coders, and others.
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
 *  $Id: ircd_parser.y 632 2006-06-01 10:53:00Z michael $
 */

%{

#define YY_NO_UNPUT
#include <sys/types.h>

#include "stdinc.h"

#ifdef HAVE_LIBCRYPTO
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#endif

%}

%union {
  int number;
  char *string;
}

%token  BYTES
%token  CONNECT
%token  DAYS
%token  DATABASE
%token  DBNAME
%token  DESCRIPTION
%token  DRIVER
%token  FLAGS
%token  GBYTES
%token  HOST
%token  HOURS
%token  KBYTES
%token  MBYTES
%token  MINUTES
%token  NAME
%token  NOT
%token  NUMBER
%token  PASSWORD
%token  PORT
%token  PROTOCOL
%token  QSTRING
%token  RSA_PRIVATE_KEY_FILE
%token  SECONDS
%token  SERVICESINFO
%token  SID
%token  SSL_CERTIFICATE_FILE
%token  TBYTES
%token  TWODOTS
%token  USERNAME
%token  VHOST
%token  VHOST6
%token  WEEKS

%type <string> QSTRING
%type <number> NUMBER
%type <number> timespec
%type <number> timespec_
%type <number> sizespec
%type <number> sizespec_

%%
conf:   
        | conf conf_item
        ;

conf_item:        
                servicesinfo_entry
                | database_entry
                | error ';'
                | error '}'
        ;


timespec_: { $$ = 0; } | timespec;
timespec:	NUMBER timespec_
		{
			$$ = $1 + $2;
		}
		| NUMBER SECONDS timespec_
		{
			$$ = $1 + $3;
		}
		| NUMBER MINUTES timespec_
		{
			$$ = $1 * 60 + $3;
		}
		| NUMBER HOURS timespec_
		{
			$$ = $1 * 60 * 60 + $3;
		}
		| NUMBER DAYS timespec_
		{
			$$ = $1 * 60 * 60 * 24 + $3;
		}
		| NUMBER WEEKS timespec_
		{
			$$ = $1 * 60 * 60 * 24 * 7 + $3;
		}
		;

sizespec_:	{ $$ = 0; } | sizespec;
sizespec:	NUMBER sizespec_ { $$ = $1 + $2; }
		| NUMBER BYTES sizespec_ { $$ = $1 + $3; }
		| NUMBER KBYTES sizespec_ { $$ = $1 * 1024 + $3; }
		| NUMBER MBYTES sizespec_ { $$ = $1 * 1024 * 1024 + $3; }
		;


/***************************************************************************
 *  section servicesinfo
 ***************************************************************************/
servicesinfo_entry: SERVICESINFO
  '{' servicesinfo_items '}' ';';

servicesinfo_items:       servicesinfo_items servicesinfo_item |
                          servicesinfo_item ;
servicesinfo_item:        servicesinfo_name | servicesinfo_vhost |
                          servicesinfo_description |
                          servicesinfo_rsa_private_key_file | 
                          servicesinfo_vhost6 | servicesinfo_sid | 
                          servicesinfo_ssl_certificate_file |
                    			error ';' ;

servicesinfo_ssl_certificate_file: SSL_CERTIFICATE_FILE '=' QSTRING ';'
{
#ifdef HAVE_LIBCRYPTO
  if (services_info.ssl_certificate_file)
  {
    MyFree(services_info.ssl_certificate_file);
    services_info.ssl_certificate_file = NULL;
  }

  if ((services_info.rsa_private_key == NULL) ||
      (!RSA_check_key(services_info.rsa_private_key)) ||
      (RSA_size(services_info.rsa_private_key) != 256))
  {
    yyerror("Ignoring config file entry ssl_certificate -- no rsa_private_key");
    break;
  }
  else
  {
    DupString(services_info.ssl_certificate_file, yylval.string);
  }
#endif
};

servicesinfo_rsa_private_key_file: RSA_PRIVATE_KEY_FILE '=' QSTRING ';'
{
#ifdef HAVE_LIBCRYPTO
  BIO *file;

  if (services_info.rsa_private_key)
  {
    RSA_free(services_info.rsa_private_key);
    services_info.rsa_private_key = NULL;
  }

  if (services_info.rsa_private_key_file)
  {
    MyFree(services_info.rsa_private_key_file);
    services_info.rsa_private_key_file = NULL;
  }

  DupString(services_info.rsa_private_key_file, yylval.string);

  if ((file = BIO_new_file(yylval.string, "r")) == NULL)
  {
    yyerror("Ignoring config file entry rsa_private_key -- file open failed");
    break;
  }

  services_info.rsa_private_key = 
    (RSA *)PEM_read_bio_RSAPrivateKey(file, NULL, 0, NULL);

  if (services_info.rsa_private_key == NULL)
  {
    yyerror("Ignoring config file entry rsa_private_key -- "
        "couldn't extract key");
    break;
  }

  if (!RSA_check_key(services_info.rsa_private_key))
  {
    yyerror("Ignoring config file entry rsa_private_key -- invalid key");
    break;
  }

  /* require 2048 bit (256 byte) key */
  if (RSA_size(services_info.rsa_private_key) != 256)
  {
    yyerror("Ignoring config file entry rsa_private_key -- not 2048 bit");
    break;
  }

  BIO_set_close(file, BIO_CLOSE);
  BIO_free(file);
#endif
};

servicesinfo_name: NAME '=' QSTRING ';' 
{
  if(services_info.name == NULL)
  {
    if(strlen(yylval.string) <= HOSTLEN)
      DupString(services_info.name, yylval.string);
  }
};

servicesinfo_sid: SID '=' QSTRING ';' 
{
  MyFree(services_info.sid);
  DupString(services_info.sid, yylval.string);
};

servicesinfo_description: DESCRIPTION '=' QSTRING ';'
{
  MyFree(services_info.description);
  DupString(services_info.description, yylval.string);
};

servicesinfo_vhost: VHOST '=' QSTRING ';'
{
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;

  if (irc_getaddrinfo(yylval.string, NULL, &hints, &res))
  ;  /* XXX Log Error */
  else
  {
    services_info.specific_ipv4_vhost = 1;

    assert(res != NULL);

    memcpy(&services_info.ip, res->ai_addr, res->ai_addrlen);
    services_info.ip.ss.ss_family = res->ai_family;
    services_info.ip.ss_len = res->ai_addrlen;
    irc_freeaddrinfo(res);

    services_info.specific_ipv4_vhost = 1;
  }
};

servicesinfo_vhost6: VHOST6 '=' QSTRING ';'
{
#ifdef IPV6
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;

  if (irc_getaddrinfo(yylval.string, NULL, &hints, &res))
  ; /* XXX Log Errror */
  else
  {
    assert(res != NULL);

    memcpy(&services_info.ip6, res->ai_addr, res->ai_addrlen);
    services_info.ip6.ss.ss_family = res->ai_family;
    services_info.ip6.ss_len = res->ai_addrlen;
    irc_freeaddrinfo(res);

    services_info.specific_ipv6_vhost = 1;
  }
#endif
};

/***************************************************************************
 *  section database
 ***************************************************************************/
database_entry: DATABASE
  '{' database_items '}' ';';

database_items:       database_items database_item |
                      database_item ;
database_item:        database_driver | database_dbname |
                      database_username | database_password |
                      error ';' ;

database_driver: DRIVER '=' QSTRING ';'
{
  MyFree(database_info.driver);
  DupString(database_info.driver, yylval.string);
};

database_dbname: DBNAME '=' QSTRING ';'
{
  MyFree(database_info.dbname);
  DupString(database_info.dbname, yylval.string);
};

database_username: USERNAME '=' QSTRING ';'
{
  MyFree(database_info.username);
  DupString(database_info.username, yylval.string);
};

database_password: PASSWORD '=' QSTRING ';'
{
  MyFree(database_info.password);
  DupString(database_info.password, yylval.string);
};

/***************************************************************************
 *  section connect 
 ***************************************************************************/
conenct_entry: CONNECT
  '{' connect_items '}' ';';

connect_items:        connect_items connect_item |
                      connect_item ;
connect_item:         connect_name | connect_host | connect_port |
                      connect_flags | connect_protocol |
                      error ';' ;

connect_name: NAME '=' QSTRING ';'
{

};

connect_host: HOST '=' QSTRING ';'
{
};

connect_port: PORT '=' NUMBER ';'
{
};

connect_flags: FLAGS
{
} '='  connect_flags_items ';';

connect_flags_items: connect_flags_items ',' connect_flags_item | connect_flags_item 

connect_flags_item: NOT  { not_atom = 1; } connect_flags_item_atom
      |  { not_atom = 0; } connect_flags_item_atom;

connect_flags_item_atom:
{
};

connect_protocol: PROTOCOL '=' QSTRING ';'
{
}
