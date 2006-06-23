/*
 *  modules.h: A header for the modules functions.
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
 *  $Id: modules.h 606 2006-06-08 22:35:55Z stu $
 */

#ifndef INCLUDED_modules_h
#define INCLUDED_modules_h

#ifdef HAVE_SHL_LOAD
#include <dl.h>
#endif
#if !defined(STATIC_MODULES) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#ifndef STATIC_MODULES
struct module
{
  dlink_node node;
  char *name;
  const char *version;
  void *handle;
  void *address;
  int core;
  void (*modremove)(void);
};

struct module_path
{
  dlink_node node;
  char path[PATH_MAX + 1];
};

/* add a path */
extern void mod_add_path(const char *);
extern void mod_clear_paths(void);

/* load all modules */
extern void load_all_modules(int);

/* load core modules */
extern void load_core_modules(int);

/* Add this module to list of modules to be loaded from conf */
extern void add_conf_module(const char *);
/* load all modules listed in conf */
extern void load_conf_modules(void);

extern void _modinit(void);
extern void _moddeinit(void);

extern int unload_one_module(char *, int);
extern int load_one_module(char *, int);
extern int load_a_module(char *, int, int);
extern dlink_node *findmodule_byname(const char *);
extern void modules_init(void);

#else /* STATIC_MODULES */

extern message_t accept_msgtab;
extern message_t admin_msgtab;
extern message_t away_msgtab;
extern message_t bmask_msgtab;
extern message_t cap_msgtab;
extern message_t capab_msgtab;
extern message_t cburst_msgtab;
#ifdef HAVE_LIBCRYPTO
extern message_t challenge_msgtab;
extern message_t cryptlink_msgtab;
#endif
extern message_t cjoin_msgtab;
extern message_t close_msgtab;
extern message_t connect_msgtab;
extern message_t die_msgtab;
extern message_t drop_msgtab;
extern message_t eob_msgtab;
extern message_t error_msgtab;
extern message_t etrace_msgtab;
extern message_t gline_msgtab;
extern message_t hash_msgtab;
extern message_t ungline_msgtab;
extern message_t info_msgtab;
extern message_t invite_msgtab;
extern message_t ison_msgtab;
extern message_t join_msgtab;
extern message_t kick_msgtab;
extern message_t kill_msgtab;
extern message_t kline_msgtab;
extern message_t unkline_msgtab;
extern message_t dline_msgtab;
extern message_t undline_msgtab;
extern message_t knock_msgtab;
extern message_t knockll_msgtab;
extern message_t links_msgtab;
extern message_t list_msgtab;
extern message_t lljoin_msgtab;
extern message_t llnick_msgtab;
extern message_t locops_msgtab;
extern message_t lusers_msgtab;
extern message_t privmsg_msgtab;
extern message_t notice_msgtab;
extern message_t map_msgtab;
extern message_t mode_msgtab;
extern message_t motd_msgtab;
extern message_t names_msgtab;
extern message_t nburst_msgtab;
extern message_t nick_msgtab;
extern message_t omotd_msgtab;
extern message_t oper_msgtab;
extern message_t operwall_msgtab;
extern message_t part_msgtab;
extern message_t pass_msgtab;
extern message_t ping_msgtab;
extern message_t pong_msgtab;
extern message_t post_msgtab;
extern message_t quit_msgtab;
extern message_t rehash_msgtab;
extern message_t restart_msgtab;
extern message_t resv_msgtab;
extern message_t rkline_msgtab;
extern message_t rxline_msgtab;
extern message_t server_msgtab;
extern message_t set_msgtab;
extern message_t sid_msgtab;
extern message_t sjoin_msgtab;
extern message_t squit_msgtab;
extern message_t stats_msgtab;
extern message_t svinfo_msgtab;
extern message_t tb_msgtab;
extern message_t tburst_msgtab;
extern message_t testline_msgtab;
extern message_t testgecos_msgtab;
extern message_t testmask_msgtab;
extern message_t time_msgtab;
extern message_t tmode_msgtab;
extern message_t topic_msgtab;
extern message_t trace_msgtab;
extern message_t uid_msgtab;
extern message_t unresv_msgtab;
extern message_t unxline_msgtab;
extern message_t user_msgtab;
extern message_t userhost_msgtab;
extern message_t users_msgtab;
extern message_t version_msgtab;
extern message_t wallops_msgtab;
extern message_t who_msgtab;
extern message_t whois_msgtab;
extern message_t whowas_msgtab;
extern message_t xline_msgtab;
extern message_t get_msgtab;
extern message_t put_msgtab;
extern message_t rxline_msgtab;
extern message_t help_msgtab;
extern message_t uhelp_msgtab;

#ifdef BUILD_CONTRIB
extern message_t bs_msgtab;
extern message_t botserv_msgtab;
extern message_t capture_msgtab;
extern message_t chanserv_msgtab;
extern message_t chghost_msgtab;
extern message_t chgident_msgtab;
extern message_t chgname_msgtab;
extern message_t classlist_msgtab;
extern message_t clearchan_msgtab;
extern message_t cs_msgtab;
extern message_t ctrace_msgtab;
extern message_t delspoof_msgtab;
extern message_t flags_msgtab;
extern message_t forcejoin_msgtab;
extern message_t forcepart_msgtab;
extern message_t global_msgtab;
extern message_t helpserv_msgtab;
extern message_t hostserv_msgtab;
extern message_t identify_msgtab;
extern message_t jupe_msgtab;
extern message_t killhost_msgtab;
extern message_t ltrace_msgtab;
extern message_t memoserv_msgtab;
extern message_t mkpasswd_msgtab;
extern message_t ms_msgtab;
extern message_t nickserv_msgtab;
extern message_t ns_msgtab;
extern message_t ojoin_msgtab;
extern message_t operserv_msgtab;
extern message_t operspy_msgtab;
extern message_t opme_msgtab;
extern message_t os_msgtab;
extern message_t seenserv_msgtab;
extern message_t spoof_msgtab;
extern message_t statserv_msgtab;
extern message_t svsnick_msgtab;
extern message_t uncapture_msgtab;
#endif

extern void load_all_modules(int);

#endif /* STATIC_MODULES */
#endif /* INCLUDED_modules_h */
