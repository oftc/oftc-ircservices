/* TODO: add copyright block */

#ifndef INCLUDED_stdinc_h
#define INCLUDED_stdinc_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <openssl/ssl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "defines.h"
#include "irc_libio.h"

#include "client.h"
#include "connection.h"
#include "chanserv.h"
#include "dbm.h"
#include "hash.h"
#include "packet.h"
#include "msg.h"
#include "parse.h"
#include "send.h"
#include "services.h"
#include "conf/conf.h"
#include "conf.h"
#include "channel_mode.h"
#include "channel.h"
#include "hostmask.h"
#include "language.h"
#include "operserv.h"
#include "interface.h"
#include "nickserv.h"
#include "crypt.h"

#endif /* INCLUDED_stdinc_h */
