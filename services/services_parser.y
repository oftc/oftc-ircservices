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

%token  ACCEPT_PASSWORD
%token  ACTION
%token  ADMIN
%token  AFTYPE
%token	T_ALLOW
%token  ANTI_NICK_FLOOD
%token  ANTI_SPAM_EXIT_MESSAGE_TIME
%token  AUTOCONN
%token	T_BLOCK
%token  BURST_AWAY
%token  BURST_TOPICWHO
%token  BYTES KBYTES MBYTES GBYTES TBYTES
%token  CALLER_ID_WAIT
%token  CAN_FLOOD
%token  CAN_IDLE
%token  CHANNEL
%token	CIDR_BITLEN_IPV4
%token	CIDR_BITLEN_IPV6
%token  CIPHER_PREFERENCE
%token  CLASS
%token  COMPRESSED
%token  COMPRESSION_LEVEL
%token  CONNECT
%token  CONNECTFREQ
%token  CRYPTLINK
%token  DEFAULT_CIPHER_PREFERENCE
%token  DEFAULT_FLOODCOUNT
%token  DEFAULT_SPLIT_SERVER_COUNT
%token  DEFAULT_SPLIT_USER_COUNT
%token  DENY
%token  DESCRIPTION
%token  DIE
%token  DISABLE_AUTH
%token  DISABLE_FAKE_CHANNELS
%token  DISABLE_HIDDEN
%token  DISABLE_LOCAL_CHANNELS
%token  DISABLE_REMOTE_COMMANDS
%token  DOT_IN_IP6_ADDR
%token  DOTS_IN_IDENT
%token	DURATION
%token  EGDPOOL_PATH
%token  EMAIL
%token	ENABLE
%token  ENCRYPTED
%token  EXCEED_LIMIT
%token  EXEMPT
%token  FAILED_OPER_NOTICE
%token  FAKENAME
%token  IRCD_FLAGS
%token  FLATTEN_LINKS
%token  FFAILED_OPERLOG
%token  FKILLLOG
%token  FKLINELOG
%token  FGLINELOG
%token  FIOERRLOG
%token  FOPERLOG
%token  FOPERSPYLOG
%token  FUSERLOG
%token  GECOS
%token  GENERAL
%token  GLINE
%token  GLINES
%token  GLINE_EXEMPT
%token  GLINE_LOG
%token  GLINE_TIME
%token  GLINE_MIN_CIDR
%token  GLINE_MIN_CIDR6
%token  GLOBAL_KILL
%token  IRCD_AUTH
%token  NEED_IDENT
%token  HAVENT_READ_CONF
%token  HIDDEN
%token  HIDDEN_ADMIN
%token	HIDDEN_NAME
%token  HIDDEN_OPER
%token  HIDE_SERVER_IPS
%token  HIDE_SERVERS
%token	HIDE_SPOOF_IPS
%token  HOST
%token  HUB
%token  HUB_MASK
%token  IDLETIME
%token  IGNORE_BOGUS_TS
%token  INVISIBLE_ON_CONNECT
%token  IP
%token  KILL
%token	KILL_CHASE_TIME_LIMIT
%token  KLINE
%token  KLINE_EXEMPT
%token  KLINE_REASON
%token  KLINE_WITH_REASON
%token  KNOCK_DELAY
%token  KNOCK_DELAY_CHANNEL
%token  LAZYLINK
%token  LEAF_MASK
%token  LINKS_DELAY
%token  LISTEN
%token  T_LOG
%token  LOGGING
%token  LOG_LEVEL
%token  MAX_ACCEPT
%token  MAX_BANS
%token  MAX_CHANS_PER_USER
%token  MAX_GLOBAL
%token  MAX_IDENT
%token  MAX_LOCAL
%token  MAX_NICK_CHANGES
%token  MAX_NICK_TIME
%token  MAX_NUMBER
%token  MAX_TARGETS
%token  MESSAGE_LOCALE
%token  MIN_NONWILDCARD
%token  MIN_NONWILDCARD_SIMPLE
%token  MODULE
%token  MODULES
%token  NAME
%token  NEED_PASSWORD
%token  NETWORK_DESC
%token  NETWORK_NAME
%token  NICK
%token  NICK_CHANGES
%token  NO_CREATE_ON_SPLIT
%token  NO_JOIN_ON_SPLIT
%token  NO_OPER_FLOOD
%token  NO_TILDE
%token  NOT
%token  NUMBER
%token  NUMBER_PER_IDENT
%token  NUMBER_PER_CIDR
%token  NUMBER_PER_IP
%token  NUMBER_PER_IP_GLOBAL
%token  OPERATOR
%token  OPERS_BYPASS_CALLERID
%token  OPER_LOG
%token  OPER_ONLY_UMODES
%token  OPER_PASS_RESV
%token  OPER_SPY_T
%token  OPER_UMODES
%token  JOIN_FLOOD_COUNT
%token  JOIN_FLOOD_TIME
%token  PACE_WAIT
%token  PACE_WAIT_SIMPLE
%token  PASSWORD
%token  PATH
%token  PING_COOKIE
%token  PING_TIME
%token  PING_WARNING
%token  PORT
%token  QSTRING
%token  QUIET_ON_BAN
%token  REASON
%token  REDIRPORT
%token  REDIRSERV
%token  REGEX_T
%token  REHASH
%token  TREJECT_HOLD_TIME
%token  REMOTE
%token  REMOTEBAN
%token  RESTRICT_CHANNELS
%token  RESTRICTED
%token  RSA_PRIVATE_KEY_FILE
%token  RSA_PUBLIC_KEY_FILE
%token  SSL_CERTIFICATE_FILE
%token  RESV
%token  RESV_EXEMPT
%token  SECONDS MINUTES HOURS DAYS WEEKS
%token  SENDQ
%token  SEND_PASSWORD
%token  SERVERHIDE
%token  SERVERINFO
%token  SERVLINK_PATH
%token  IRCD_SID
%token	TKLINE_EXPIRE_NOTICES
%token  T_SHARED
%token  T_CLUSTER
%token  TYPE
%token  SHORT_MOTD
%token  SILENT
%token  SPOOF
%token  SPOOF_NOTICE
%token  STATS_E_DISABLED
%token  STATS_I_OPER_ONLY
%token  STATS_K_OPER_ONLY
%token  STATS_O_OPER_ONLY
%token  STATS_P_OPER_ONLY
%token  TBOOL
%token  TMASKED
%token  T_REJECT
%token  TS_MAX_DELTA
%token  TS_WARN_DELTA
%token  TWODOTS
%token  T_ALL
%token  T_BOTS
%token  T_SOFTCALLERID
%token  T_CALLERID
%token  T_CCONN
%token  T_CLIENT_FLOOD
%token  T_DEAF
%token  T_DEBUG
%token  T_DRONE
%token  T_EXTERNAL
%token  T_FULL
%token  T_INVISIBLE
%token  T_IPV4
%token  T_IPV6
%token  T_LOCOPS
%token  T_LOGPATH
%token  T_L_CRIT
%token  T_L_DEBUG
%token  T_L_ERROR
%token  T_L_INFO
%token  T_L_NOTICE
%token  T_L_TRACE
%token  T_L_WARN
%token  T_MAX_CLIENTS
%token  T_NCHANGE
%token  T_OPERWALL
%token  T_REJ
%token  T_SERVNOTICE
%token  T_SKILL
%token  T_SPY
%token  T_SSL
%token  T_UMODES
%token  T_UNAUTH
%token  T_UNRESV
%token  T_UNXLINE
%token  T_WALLOP
%token  THROTTLE_TIME
%token  TOPICBURST
%token  TRUE_NO_OPER_FLOOD
%token  TKLINE
%token  TXLINE
%token  TRESV
%token  UNKLINE
%token  USER
%token  USE_EGD
%token  USE_EXCEPT
%token  USE_INVEX
%token  USE_KNOCK
%token  USE_LOGGING
%token  USE_WHOIS_ACTUALLY
%token  VHOST
%token  VHOST6
%token  XLINE
%token  WARN
%token  WARN_NO_NLINE

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

conf_item:        admin_entry
                | logging_entry
                | oper_entry
		| channel_entry
                | class_entry 
                | listen_entry
                | auth_entry
                | serverinfo_entry
		| serverhide_entry
                | resv_entry
                | shared_entry
		| cluster_entry
                | connect_entry
                | kill_entry
                | deny_entry
		| exempt_entry
		| general_entry
		| gline_entry
                | gecos_entry
                | modules_entry
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
 *  section modules
 ***************************************************************************/
modules_entry: MODULES
  '{' modules_items '}' ';';

modules_items:  modules_items modules_item | modules_item;
modules_item:   modules_module | modules_path | error ';' ;

modules_module: MODULE '=' QSTRING ';'
{
#ifndef STATIC_MODULES /* NOOP in the static case */
#endif
};

modules_path: PATH '=' QSTRING ';'
{
#ifndef STATIC_MODULES
#endif
};

/***************************************************************************
 *  section serverinfo
 ***************************************************************************/
serverinfo_entry: SERVERINFO
  '{' serverinfo_items '}' ';';

serverinfo_items:       serverinfo_items serverinfo_item |
                        serverinfo_item ;
serverinfo_item:        serverinfo_name | serverinfo_vhost |
                        serverinfo_hub | serverinfo_description |
                        serverinfo_network_name | serverinfo_network_desc |
                        serverinfo_max_clients | 
                        serverinfo_rsa_private_key_file | serverinfo_vhost6 |
                        serverinfo_sid | serverinfo_ssl_certificate_file |
			error ';' ;

serverinfo_ssl_certificate_file: SSL_CERTIFICATE_FILE '=' QSTRING ';'
{
#ifdef HAVE_LIBCRYPTO
#endif
};

serverinfo_rsa_private_key_file: RSA_PRIVATE_KEY_FILE '=' QSTRING ';'
{
#ifdef HAVE_LIBCRYPTO
#endif
};

serverinfo_name: NAME '=' QSTRING ';' 
{
};

serverinfo_sid: IRCD_SID '=' QSTRING ';' 
{
};

serverinfo_description: DESCRIPTION '=' QSTRING ';'
{
};

serverinfo_network_name: NETWORK_NAME '=' QSTRING ';'
{
};

serverinfo_network_desc: NETWORK_DESC '=' QSTRING ';'
{
};

serverinfo_vhost: VHOST '=' QSTRING ';'
{
};

serverinfo_vhost6: VHOST6 '=' QSTRING ';'
{
#ifdef IPV6
#endif
};

serverinfo_max_clients: T_MAX_CLIENTS '=' NUMBER ';'
{
};

serverinfo_hub: HUB '=' TBOOL ';' 
{
};

/***************************************************************************
 * admin section
 ***************************************************************************/
admin_entry: ADMIN  '{' admin_items '}' ';' ;

admin_items: admin_items admin_item | admin_item;
admin_item:  admin_name | admin_description |
             admin_email | error ';' ;

admin_name: NAME '=' QSTRING ';' 
{
};

admin_email: EMAIL '=' QSTRING ';'
{
};

admin_description: DESCRIPTION '=' QSTRING ';'
{
};

/***************************************************************************
 *  section logging
 ***************************************************************************/
/* XXX */
logging_entry:          LOGGING  '{' logging_items '}' ';' ;

logging_items:          logging_items logging_item |
                        logging_item ;

logging_item:           logging_path | logging_oper_log |
			logging_log_level |
			logging_use_logging | logging_fuserlog |
			logging_foperlog | logging_fglinelog |
			logging_fklinelog | logging_killlog |
			logging_foperspylog | logging_ioerrlog |
			logging_ffailed_operlog |
			error ';' ;

logging_path:           T_LOGPATH '=' QSTRING ';' 
                        {
                        };

logging_oper_log:	OPER_LOG '=' QSTRING ';'
                        {
                        };

logging_fuserlog: FUSERLOG '=' QSTRING ';'
{
};

logging_ffailed_operlog: FFAILED_OPERLOG '=' QSTRING ';'
{
};

logging_foperlog: FOPERLOG '=' QSTRING ';'
{
};

logging_foperspylog: FOPERSPYLOG '=' QSTRING ';'
{
};

logging_fglinelog: FGLINELOG '=' QSTRING ';'
{
};

logging_fklinelog: FKLINELOG '=' QSTRING ';'
{
};

logging_ioerrlog: FIOERRLOG '=' QSTRING ';'
{
};

logging_killlog: FKILLLOG '=' QSTRING ';'
{
};

logging_log_level: LOG_LEVEL '=' T_L_CRIT ';'
{ 
} | LOG_LEVEL '=' T_L_ERROR ';'
{
} | LOG_LEVEL '=' T_L_WARN ';'
{
} | LOG_LEVEL '=' T_L_NOTICE ';'
{
} | LOG_LEVEL '=' T_L_TRACE ';'
{
} | LOG_LEVEL '=' T_L_INFO ';'
{
} | LOG_LEVEL '=' T_L_DEBUG ';'
{
};

logging_use_logging: USE_LOGGING '=' TBOOL ';'
{
};

/***************************************************************************
 * section oper
 ***************************************************************************/
oper_entry: OPERATOR 
{
} oper_name_b '{' oper_items '}' ';'
{
}; 

oper_name_b: | oper_name_t;
oper_items:     oper_items oper_item | oper_item;
oper_item:      oper_name | oper_user | oper_password | oper_hidden_admin |
                oper_hidden_oper | oper_umodes |
		oper_class | oper_global_kill | oper_remote |
                oper_kline | oper_xline | oper_unkline |
		oper_gline | oper_nick_changes | oper_remoteban |
                oper_die | oper_rehash | oper_admin | oper_operwall |
		oper_encrypted | oper_rsa_public_key_file |
                oper_flags | error ';' ;

oper_name: NAME '=' QSTRING ';'
{
};

oper_name_t: QSTRING
{
};

oper_user: USER '=' QSTRING ';'
{
};

oper_password: PASSWORD '=' QSTRING ';'
{
};

oper_encrypted: ENCRYPTED '=' TBOOL ';'
{
};

oper_rsa_public_key_file: RSA_PUBLIC_KEY_FILE '=' QSTRING ';'
{
};

oper_class: CLASS '=' QSTRING ';'
{
};

oper_umodes: T_UMODES
{
} '='  oper_umodes_items ';' ;

oper_umodes_items: oper_umodes_items ',' oper_umodes_item | oper_umodes_item;
oper_umodes_item:  T_BOTS
{
} | T_CCONN
{
} | T_DEAF
{
} | T_DEBUG
{
} | T_FULL
{
} | T_SKILL
{
} | T_NCHANGE
{
} | T_REJ
{
} | T_UNAUTH
{
} | T_SPY
{
} | T_EXTERNAL
{
} | T_OPERWALL
{
} | T_SERVNOTICE
{
} | T_INVISIBLE
{
} | T_WALLOP
{
} | T_SOFTCALLERID
{
} | T_CALLERID
{
} | T_LOCOPS
{
};

oper_global_kill: GLOBAL_KILL '=' TBOOL ';'
{
};

oper_remote: REMOTE '=' TBOOL ';'
{
};

oper_remoteban: REMOTEBAN '=' TBOOL ';'
{
};

oper_kline: KLINE '=' TBOOL ';'
{
};

oper_xline: XLINE '=' TBOOL ';'
{
};

oper_unkline: UNKLINE '=' TBOOL ';'
{
};

oper_gline: GLINE '=' TBOOL ';'
{
};

oper_nick_changes: NICK_CHANGES '=' TBOOL ';'
{
};

oper_die: DIE '=' TBOOL ';'
{
};

oper_rehash: REHASH '=' TBOOL ';'
{
};

oper_admin: ADMIN '=' TBOOL ';'
{
};

oper_hidden_admin: HIDDEN_ADMIN '=' TBOOL ';'
{
};

oper_hidden_oper: HIDDEN_OPER '=' TBOOL ';'
{
};

oper_operwall: T_OPERWALL '=' TBOOL ';'
{
};

oper_flags: IRCD_FLAGS
{
} '='  oper_flags_items ';';

oper_flags_items: oper_flags_items ',' oper_flags_item | oper_flags_item;
oper_flags_item: NOT {  } oper_flags_item_atom
		| {  } oper_flags_item_atom;

oper_flags_item_atom: GLOBAL_KILL
{
} | REMOTE
{
} | KLINE
{
} | UNKLINE
{
} | XLINE
{
} | GLINE
{
} | DIE
{
} | REHASH
{
} | ADMIN
{
} | HIDDEN_ADMIN
{
} | NICK_CHANGES
{
} | T_OPERWALL
{
} | OPER_SPY_T
{
} | HIDDEN_OPER
{
} | REMOTEBAN
{
} | ENCRYPTED
{
};


/***************************************************************************
 *  section class
 ***************************************************************************/
class_entry: CLASS
{
} class_name_b '{' class_items '}' ';'
{
};

class_name_b: | class_name_t;

class_items:    class_items class_item | class_item;
class_item:     class_name |
		class_cidr_bitlen_ipv4 | class_cidr_bitlen_ipv6 |
                class_ping_time |
		class_ping_warning |
		class_number_per_cidr |
                class_number_per_ip |
                class_connectfreq |
                class_max_number |
		class_max_global |
		class_max_local |
		class_max_ident |
                class_sendq |
		error ';' ;

class_name: NAME '=' QSTRING ';' 
{
};

class_name_t: QSTRING
{
};

class_ping_time: PING_TIME '=' timespec ';'
{
};

class_ping_warning: PING_WARNING '=' timespec ';'
{
};

class_number_per_ip: NUMBER_PER_IP '=' NUMBER ';'
{
};

class_connectfreq: CONNECTFREQ '=' timespec ';'
{
};

class_max_number: MAX_NUMBER '=' NUMBER ';'
{
};

class_max_global: MAX_GLOBAL '=' NUMBER ';'
{
};

class_max_local: MAX_LOCAL '=' NUMBER ';'
{
};

class_max_ident: MAX_IDENT '=' NUMBER ';'
{
};

class_sendq: SENDQ '=' sizespec ';'
{
};

class_cidr_bitlen_ipv4: CIDR_BITLEN_IPV4 '=' NUMBER ';'
{
};

class_cidr_bitlen_ipv6: CIDR_BITLEN_IPV6 '=' NUMBER ';'
{
};

class_number_per_cidr: NUMBER_PER_CIDR '=' NUMBER ';'
{
};

/***************************************************************************
 *  section listen
 ***************************************************************************/
listen_entry: LISTEN
{
} '{' listen_items '}' ';'
{
};

listen_flags: IRCD_FLAGS
{
} '='  listen_flags_items ';';

listen_flags_items: listen_flags_items ',' listen_flags_item | listen_flags_item;
listen_flags_item: T_SSL
{
} | HIDDEN
{
};

listen_items:   listen_items listen_item | listen_item;
listen_item:    listen_port | listen_flags | listen_address | listen_host | error ';';

listen_port: PORT '=' port_items {  } ';';

port_items: port_items ',' port_item | port_item;

port_item: NUMBER
{
} | NUMBER TWODOTS NUMBER
{
};

listen_address: IP '=' QSTRING ';'
{
};

listen_host: HOST '=' QSTRING ';'
{
};

/***************************************************************************
 *  section auth
 ***************************************************************************/
auth_entry: IRCD_AUTH
{
} '{' auth_items '}' ';'
{
}; 

auth_items:     auth_items auth_item | auth_item;
auth_item:      auth_user | auth_passwd | auth_class | auth_flags |
                auth_kline_exempt | auth_need_ident |
                auth_exceed_limit | auth_no_tilde | auth_gline_exempt |
		auth_spoof | auth_spoof_notice |
                auth_redir_serv | auth_redir_port | auth_can_flood |
                auth_need_password | auth_encrypted | error ';' ;

auth_user: USER '=' QSTRING ';'
{
};

/* XXX - IP/IPV6 tags don't exist anymore - put IP/IPV6 into user. */

auth_passwd: PASSWORD '=' QSTRING ';'
{
};

auth_spoof_notice: SPOOF_NOTICE '=' TBOOL ';'
{
};

auth_class: CLASS '=' QSTRING ';'
{
};

auth_encrypted: ENCRYPTED '=' TBOOL ';'
{
};

auth_flags: IRCD_FLAGS
{
} '='  auth_flags_items ';';

auth_flags_items: auth_flags_items ',' auth_flags_item | auth_flags_item;
auth_flags_item: NOT {  } auth_flags_item_atom
		| {  } auth_flags_item_atom;

auth_flags_item_atom: SPOOF_NOTICE
{
} | EXCEED_LIMIT
{
} | KLINE_EXEMPT
{
} | NEED_IDENT
{
} | CAN_FLOOD
{
} | CAN_IDLE
{
} | NO_TILDE
{
} | GLINE_EXEMPT
{
} | RESV_EXEMPT
{
} | NEED_PASSWORD
{
};

auth_kline_exempt: KLINE_EXEMPT '=' TBOOL ';'
{
};

auth_need_ident: NEED_IDENT '=' TBOOL ';'
{
};

auth_exceed_limit: EXCEED_LIMIT '=' TBOOL ';'
{
};

auth_can_flood: CAN_FLOOD '=' TBOOL ';'
{
};

auth_no_tilde: NO_TILDE '=' TBOOL ';' 
{
};

auth_gline_exempt: GLINE_EXEMPT '=' TBOOL ';' 
{
};

/* XXX - need check for illegal hostnames here */
auth_spoof: SPOOF '=' QSTRING ';' 
{
};

auth_redir_serv: REDIRSERV '=' QSTRING ';'
{
};

auth_redir_port: REDIRPORT '=' NUMBER ';'
{
};

auth_need_password: NEED_PASSWORD '=' TBOOL ';'
{
};


/***************************************************************************
 *  section resv
 ***************************************************************************/
resv_entry: RESV
{
} '{' resv_items '}' ';'
{
};

resv_items:	resv_items resv_item | resv_item;
resv_item:	resv_creason | resv_channel | resv_nick | error ';' ;

resv_creason: REASON '=' QSTRING ';'
{
};

resv_channel: CHANNEL '=' QSTRING ';'
{
};

resv_nick: NICK '=' QSTRING ';'
{
};

/***************************************************************************
 *  section shared, for sharing remote klines etc.
 ***************************************************************************/
shared_entry: T_SHARED
{
} '{' shared_items '}' ';'
{
};

shared_items: shared_items shared_item | shared_item;
shared_item:  shared_name | shared_user | shared_type | error ';' ;

shared_name: NAME '=' QSTRING ';'
{
};

shared_user: USER '=' QSTRING ';'
{
};

shared_type: TYPE
{
} '=' shared_types ';' ;

shared_types: shared_types ',' shared_type_item | shared_type_item;
shared_type_item: KLINE
{
} | TKLINE
{
} | UNKLINE
{
} | XLINE
{
} | TXLINE
{
} | T_UNXLINE
{
} | RESV
{
} | TRESV
{
} | T_UNRESV
{
} | T_LOCOPS
{
} | T_ALL
{
};

/***************************************************************************
 *  section cluster
 ***************************************************************************/
cluster_entry: T_CLUSTER
{
} '{' cluster_items '}' ';'
{
};

cluster_items:	cluster_items cluster_item | cluster_item;
cluster_item:	cluster_name | cluster_type | error ';' ;

cluster_name: NAME '=' QSTRING ';'
{
};

cluster_type: TYPE
{
} '=' cluster_types ';' ;

cluster_types:	cluster_types ',' cluster_type_item | cluster_type_item;
cluster_type_item: KLINE
{
} | TKLINE
{
} | UNKLINE
{
} | XLINE
{
} | TXLINE
{
} | T_UNXLINE
{
} | RESV
{
} | TRESV
{
} | T_UNRESV
{
} | T_LOCOPS
{
} | T_ALL
{
};

/***************************************************************************
 *  section connect
 ***************************************************************************/
connect_entry: CONNECT   
{
} connect_name_b '{' connect_items '}' ';'
{
};

connect_name_b: | connect_name_t;
connect_items:  connect_items connect_item | connect_item;
connect_item:   connect_name | connect_host | connect_vhost |
		connect_send_password | connect_accept_password |
		connect_aftype | connect_port |
 		connect_fakename | connect_flags | connect_hub_mask | 
		connect_leaf_mask | connect_class | connect_auto |
		connect_encrypted | connect_compressed | connect_cryptlink |
		connect_rsa_public_key_file | connect_cipher_preference |
                connect_topicburst | error ';' ;

connect_name: NAME '=' QSTRING ';'
{
};

connect_name_t: QSTRING
{
};

connect_host: HOST '=' QSTRING ';' 
{
};

connect_vhost: VHOST '=' QSTRING ';' 
{
};
 
connect_send_password: SEND_PASSWORD '=' QSTRING ';'
{
};

connect_accept_password: ACCEPT_PASSWORD '=' QSTRING ';'
{
};

connect_port: PORT '=' NUMBER ';'
{
};

connect_aftype: AFTYPE '=' T_IPV4 ';'
{
} | AFTYPE '=' T_IPV6 ';'
{
};

connect_fakename: FAKENAME '=' QSTRING ';'
{
};

connect_flags: IRCD_FLAGS
{
} '='  connect_flags_items ';';

connect_flags_items: connect_flags_items ',' connect_flags_item | connect_flags_item;
connect_flags_item: NOT  {  } connect_flags_item_atom
			|  {  } connect_flags_item_atom;

connect_flags_item_atom: LAZYLINK
{
} | COMPRESSED
{
} | CRYPTLINK
{
} | AUTOCONN
{
} | BURST_AWAY
{
} | TOPICBURST
{
}
;

connect_rsa_public_key_file: RSA_PUBLIC_KEY_FILE '=' QSTRING ';'
{
};

connect_encrypted: ENCRYPTED '=' TBOOL ';'
{
};

connect_cryptlink: CRYPTLINK '=' TBOOL ';'
{
};

connect_compressed: COMPRESSED '=' TBOOL ';'
{
};

connect_auto: AUTOCONN '=' TBOOL ';'
{
};

connect_topicburst: TOPICBURST '=' TBOOL ';'
{
};

connect_hub_mask: HUB_MASK '=' QSTRING ';' 
{
};

connect_leaf_mask: LEAF_MASK '=' QSTRING ';' 
{
};

connect_class: CLASS '=' QSTRING ';'
{
};

connect_cipher_preference: CIPHER_PREFERENCE '=' QSTRING ';'
{
};

/***************************************************************************
 *  section kill
 ***************************************************************************/
kill_entry: KILL
{
} '{' kill_items '}' ';'
{
}; 

kill_type: TYPE
{
} '='  kill_type_items ';';

kill_type_items: kill_type_items ',' kill_type_item | kill_type_item;
kill_type_item: REGEX_T
{
};

kill_items:     kill_items kill_item | kill_item;
kill_item:      kill_user | kill_reason | kill_type | error;

kill_user: USER '=' QSTRING ';'
{
};

kill_reason: REASON '=' QSTRING ';' 
{
};

/***************************************************************************
 *  section deny
 ***************************************************************************/
deny_entry: DENY 
{
} '{' deny_items '}' ';'
{
}; 

deny_items:     deny_items deny_item | deny_item;
deny_item:      deny_ip | deny_reason | error;

deny_ip: IP '=' QSTRING ';'
{
};

deny_reason: REASON '=' QSTRING ';' 
{
};

/***************************************************************************
 *  section exempt
 ***************************************************************************/
exempt_entry: EXEMPT '{' exempt_items '}' ';';

exempt_items:     exempt_items exempt_item | exempt_item;
exempt_item:      exempt_ip | error;

exempt_ip: IP '=' QSTRING ';'
{
};

/***************************************************************************
 *  section gecos
 ***************************************************************************/
gecos_entry: GECOS
{
} '{' gecos_items '}' ';'
{
};

gecos_flags: TYPE
{
} '='  gecos_flags_items ';';

gecos_flags_items: gecos_flags_items ',' gecos_flags_item | gecos_flags_item;
gecos_flags_item: REGEX_T
{
};

gecos_items: gecos_items gecos_item | gecos_item;
gecos_item:  gecos_name | gecos_reason | gecos_flags | error;

gecos_name: NAME '=' QSTRING ';' 
{
};

gecos_reason: REASON '=' QSTRING ';' 
{
};

/***************************************************************************
 *  section general
 ***************************************************************************/
general_entry: GENERAL
  '{' general_items '}' ';';

general_items:      general_items general_item | general_item;
general_item:       general_hide_spoof_ips | general_ignore_bogus_ts |
		    general_failed_oper_notice | general_anti_nick_flood |
		    general_max_nick_time | general_max_nick_changes |
		    general_max_accept | general_anti_spam_exit_message_time |
                    general_ts_warn_delta | general_ts_max_delta |
                    general_kill_chase_time_limit | general_kline_with_reason |
                    general_kline_reason | general_invisible_on_connect |
                    general_warn_no_nline | general_dots_in_ident |
                    general_stats_o_oper_only | general_stats_k_oper_only |
                    general_pace_wait | general_stats_i_oper_only |
                    general_pace_wait_simple | general_stats_P_oper_only |
                    general_short_motd | general_no_oper_flood |
                    general_true_no_oper_flood | general_oper_pass_resv |
                    general_idletime | general_message_locale |
                    general_oper_only_umodes | general_max_targets |
                    general_use_egd | general_egdpool_path |
                    general_oper_umodes | general_caller_id_wait |
                    general_opers_bypass_callerid | general_default_floodcount |
                    general_min_nonwildcard | general_min_nonwildcard_simple |
                    general_servlink_path | general_disable_remote_commands |
                    general_default_cipher_preference |
                    general_compression_level | general_client_flood |
                    general_throttle_time | general_havent_read_conf |
                    general_dot_in_ip6_addr | general_ping_cookie |
                    general_disable_auth | general_burst_away |
		    general_tkline_expire_notices | general_gline_min_cidr |
                    general_gline_min_cidr6 | general_use_whois_actually |
		    general_reject_hold_time | general_stats_e_disabled |
		    error;



general_gline_min_cidr: GLINE_MIN_CIDR '=' NUMBER ';'
{
};

general_gline_min_cidr6: GLINE_MIN_CIDR6 '=' NUMBER ';'
{
};

general_burst_away: BURST_AWAY '=' TBOOL ';'
{
};

general_use_whois_actually: USE_WHOIS_ACTUALLY '=' TBOOL ';'
{
};

general_reject_hold_time: TREJECT_HOLD_TIME '=' timespec ';'
{
};

general_tkline_expire_notices: TKLINE_EXPIRE_NOTICES '=' TBOOL ';'
{
};

general_kill_chase_time_limit: KILL_CHASE_TIME_LIMIT '=' NUMBER ';'
{
};

general_hide_spoof_ips: HIDE_SPOOF_IPS '=' TBOOL ';'
{
};

general_ignore_bogus_ts: IGNORE_BOGUS_TS '=' TBOOL ';'
{
};

general_disable_remote_commands: DISABLE_REMOTE_COMMANDS '=' TBOOL ';'
{
};

general_failed_oper_notice: FAILED_OPER_NOTICE '=' TBOOL ';'
{
};

general_anti_nick_flood: ANTI_NICK_FLOOD '=' TBOOL ';'
{
};

general_max_nick_time: MAX_NICK_TIME '=' timespec ';'
{
};

general_max_nick_changes: MAX_NICK_CHANGES '=' NUMBER ';'
{
};

general_max_accept: MAX_ACCEPT '=' NUMBER ';'
{
};

general_anti_spam_exit_message_time: ANTI_SPAM_EXIT_MESSAGE_TIME '=' timespec ';'
{
};

general_ts_warn_delta: TS_WARN_DELTA '=' timespec ';'
{
};

general_ts_max_delta: TS_MAX_DELTA '=' timespec ';'
{
};

general_havent_read_conf: HAVENT_READ_CONF '=' NUMBER ';'
{
};

general_kline_with_reason: KLINE_WITH_REASON '=' TBOOL ';'
{
};

general_kline_reason: KLINE_REASON '=' QSTRING ';'
{
};

general_invisible_on_connect: INVISIBLE_ON_CONNECT '=' TBOOL ';'
{
};

general_warn_no_nline: WARN_NO_NLINE '=' TBOOL ';'
{
};

general_stats_e_disabled: STATS_E_DISABLED '=' TBOOL ';'
{
};

general_stats_o_oper_only: STATS_O_OPER_ONLY '=' TBOOL ';'
{
};

general_stats_P_oper_only: STATS_P_OPER_ONLY '=' TBOOL ';'
{
};

general_stats_k_oper_only: STATS_K_OPER_ONLY '=' TBOOL ';'
{
} | STATS_K_OPER_ONLY '=' TMASKED ';'
{
};

general_stats_i_oper_only: STATS_I_OPER_ONLY '=' TBOOL ';'
{
} | STATS_I_OPER_ONLY '=' TMASKED ';'
{
};

general_pace_wait: PACE_WAIT '=' timespec ';'
{
};

general_caller_id_wait: CALLER_ID_WAIT '=' timespec ';'
{
};

general_opers_bypass_callerid: OPERS_BYPASS_CALLERID '=' TBOOL ';'
{
};

general_pace_wait_simple: PACE_WAIT_SIMPLE '=' timespec ';'
{
};

general_short_motd: SHORT_MOTD '=' TBOOL ';'
{
};
  
general_no_oper_flood: NO_OPER_FLOOD '=' TBOOL ';'
{
};

general_true_no_oper_flood: TRUE_NO_OPER_FLOOD '=' TBOOL ';'
{
};

general_oper_pass_resv: OPER_PASS_RESV '=' TBOOL ';'
{
};

general_message_locale: MESSAGE_LOCALE '=' QSTRING ';'
{
};

general_idletime: IDLETIME '=' timespec ';'
{
};

general_dots_in_ident: DOTS_IN_IDENT '=' NUMBER ';'
{
};

general_max_targets: MAX_TARGETS '=' NUMBER ';'
{
};

general_servlink_path: SERVLINK_PATH '=' QSTRING ';'
{
};

general_default_cipher_preference: DEFAULT_CIPHER_PREFERENCE '=' QSTRING ';'
{
};

general_compression_level: COMPRESSION_LEVEL '=' NUMBER ';'
{
};

general_use_egd: USE_EGD '=' TBOOL ';'
{
};

general_egdpool_path: EGDPOOL_PATH '=' QSTRING ';'
{
};

general_ping_cookie: PING_COOKIE '=' TBOOL ';'
{
};

general_disable_auth: DISABLE_AUTH '=' TBOOL ';'
{
};

general_throttle_time: THROTTLE_TIME '=' timespec ';'
{
};

general_oper_umodes: OPER_UMODES
{
} '='  umode_oitems ';' ;

umode_oitems:    umode_oitems ',' umode_oitem | umode_oitem;
umode_oitem:     T_BOTS
{
} | T_CCONN
{
} | T_DEAF
{
} | T_DEBUG
{
} | T_FULL
{
} | T_SKILL
{
} | T_NCHANGE
{
} | T_REJ
{
} | T_UNAUTH
{
} | T_SPY
{
} | T_EXTERNAL
{
} | T_OPERWALL
{
} | T_SERVNOTICE
{
} | T_INVISIBLE
{
} | T_WALLOP
{
} | T_SOFTCALLERID
{
} | T_CALLERID
{
} | T_LOCOPS
{
};

general_oper_only_umodes: OPER_ONLY_UMODES 
{
} '='  umode_items ';' ;

umode_items:	umode_items ',' umode_item | umode_item;
umode_item:	T_BOTS 
{
} | T_CCONN
{
} | T_DEAF
{
} | T_DEBUG
{
} | T_FULL
{ 
} | T_SKILL
{
} | T_NCHANGE
{
} | T_REJ
{
} | T_UNAUTH
{
} | T_SPY
{
} | T_EXTERNAL
{
} | T_OPERWALL
{
} | T_SERVNOTICE
{
} | T_INVISIBLE
{
} | T_WALLOP
{
} | T_SOFTCALLERID
{
} | T_CALLERID
{
} | T_LOCOPS
{
};

general_min_nonwildcard: MIN_NONWILDCARD '=' NUMBER ';'
{
};

general_min_nonwildcard_simple: MIN_NONWILDCARD_SIMPLE '=' NUMBER ';'
{
};

general_default_floodcount: DEFAULT_FLOODCOUNT '=' NUMBER ';'
{
};

general_client_flood: T_CLIENT_FLOOD '=' sizespec ';'
{
};

general_dot_in_ip6_addr: DOT_IN_IP6_ADDR '=' TBOOL ';'
{
};

/*************************************************************************** 
 *  section glines
 ***************************************************************************/
gline_entry: GLINES
{
} '{' gline_items '}' ';'
{
};

gline_items:        gline_items gline_item | gline_item;
gline_item:         gline_enable | 
                    gline_duration |
		    gline_logging |
                    gline_user |
                    gline_server | 
                    gline_action |
                    error;

gline_enable: ENABLE '=' TBOOL ';'
{
};

gline_duration: DURATION '=' timespec ';'
{
};

gline_logging: LOGGING
{
} '=' gline_logging_types ';';
gline_logging_types:	 gline_logging_types ',' gline_logging_type_item | gline_logging_type_item;
gline_logging_type_item: T_REJECT
{
} | T_BLOCK
{
};

gline_user: USER '=' QSTRING ';'
{
};

gline_server: NAME '=' QSTRING ';'
{
};

gline_action: ACTION
{
} '=' gdeny_types ';'
{
};

gdeny_types: gdeny_types ',' gdeny_type_item | gdeny_type_item;
gdeny_type_item: T_REJECT
{
} | T_BLOCK
{
};

/***************************************************************************
 *  section channel
 ***************************************************************************/
channel_entry: CHANNEL
  '{' channel_items '}' ';';

channel_items:      channel_items channel_item | channel_item;
channel_item:       channel_disable_local_channels | channel_use_except |
                    channel_use_invex | channel_use_knock |
                    channel_max_bans | channel_knock_delay |
                    channel_knock_delay_channel | channel_max_chans_per_user |
                    channel_quiet_on_ban | channel_default_split_user_count |
                    channel_default_split_server_count |
                    channel_no_create_on_split | channel_restrict_channels |
                    channel_no_join_on_split | channel_burst_topicwho |
                    channel_jflood_count | channel_jflood_time |
                    channel_disable_fake_channels | error;

channel_disable_fake_channels: DISABLE_FAKE_CHANNELS '=' TBOOL ';'
{
};

channel_restrict_channels: RESTRICT_CHANNELS '=' TBOOL ';'
{
};

channel_disable_local_channels: DISABLE_LOCAL_CHANNELS '=' TBOOL ';'
{
};

channel_use_except: USE_EXCEPT '=' TBOOL ';'
{
};

channel_use_invex: USE_INVEX '=' TBOOL ';'
{
};

channel_use_knock: USE_KNOCK '=' TBOOL ';'
{
};

channel_knock_delay: KNOCK_DELAY '=' timespec ';'
{
};

channel_knock_delay_channel: KNOCK_DELAY_CHANNEL '=' timespec ';'
{
};

channel_max_chans_per_user: MAX_CHANS_PER_USER '=' NUMBER ';'
{
};

channel_quiet_on_ban: QUIET_ON_BAN '=' TBOOL ';'
{
};

channel_max_bans: MAX_BANS '=' NUMBER ';'
{
};

channel_default_split_user_count: DEFAULT_SPLIT_USER_COUNT '=' NUMBER ';'
{
};

channel_default_split_server_count: DEFAULT_SPLIT_SERVER_COUNT '=' NUMBER ';'
{
};

channel_no_create_on_split: NO_CREATE_ON_SPLIT '=' TBOOL ';'
{
};

channel_no_join_on_split: NO_JOIN_ON_SPLIT '=' TBOOL ';'
{
};

channel_burst_topicwho: BURST_TOPICWHO '=' TBOOL ';'
{
};

channel_jflood_count: JOIN_FLOOD_COUNT '=' NUMBER ';'
{
};

channel_jflood_time: JOIN_FLOOD_TIME '=' timespec ';'
{
};

/***************************************************************************
 *  section serverhide
 ***************************************************************************/
serverhide_entry: SERVERHIDE
  '{' serverhide_items '}' ';';

serverhide_items:   serverhide_items serverhide_item | serverhide_item;
serverhide_item:    serverhide_flatten_links | serverhide_hide_servers |
		    serverhide_links_delay |
		    serverhide_disable_hidden |
		    serverhide_hidden | serverhide_hidden_name |
		    serverhide_hide_server_ips |
                    error;

serverhide_flatten_links: FLATTEN_LINKS '=' TBOOL ';'
{
};

serverhide_hide_servers: HIDE_SERVERS '=' TBOOL ';'
{
};

serverhide_hidden_name: HIDDEN_NAME '=' QSTRING ';'
{
};

serverhide_links_delay: LINKS_DELAY '=' timespec ';'
{
};

serverhide_hidden: HIDDEN '=' TBOOL ';'
{
};

serverhide_disable_hidden: DISABLE_HIDDEN '=' TBOOL ';'
{
};

serverhide_hide_server_ips: HIDE_SERVER_IPS '=' TBOOL ';'
{
};
