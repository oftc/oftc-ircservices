#include "setup.h"
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
#include "dbm.h"
#include "hash.h"
#include "packet.h"
#include "msg.h"
#include "parse.h"
#include "send.h"
#include "services.h"

#define TRUE 1
#define FALSE 0

#ifndef _WIN32
# define EXTERN extern
#else
# ifdef IN_IRCD
#  define EXTERN extern __declspec(dllexport)
# else
#  define EXTERN extern __declspec(dllimport)
# endif
# define _modinit   __declspec(dllexport) _modinit
# define _moddeinit __declspec(dllexport) _moddeinit
# define _version   __declspec(dllexport) _version
#endif

#ifdef HAVE_STRTOK_R
# define strtoken(x, y, z) strtok_r(y, z, x)
#endif

#include "conf/conf.h"
#include "conf.h"
#include "channel_mode.h"
#include "channel.h"
#include "hostmask.h"
