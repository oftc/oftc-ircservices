/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  user.c: User functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
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
 *  $Id: $
 */

#include "stdinc.h"

/* memory is cheap. map 0-255 to equivalent mode */
unsigned int user_modes[256] =
{
  /* 0x00 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x0F */
  /* 0x10 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x1F */
  /* 0x20 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x2F */
  /* 0x30 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x3F */
  0,                  /* @ */
  0,                  /* A */
  0,                  /* B */
  0,                  /* C */
  UMODE_DEAF,         /* D */
  0,                  /* E */
  0,                  /* F */
  UMODE_SOFTCALLERID, /* G */
  0,                  /* H */
  0,                  /* I */
  0,                  /* J */
  0,                  /* K */
  0,                  /* L */
  0,                  /* M */
  0,                  /* N */
  0,                  /* O */
  0,                  /* P */
  0,                  /* Q */
  0,                  /* R */
  0,                  /* S */
  0,                  /* T */
  0,                  /* U */
  0,                  /* V */
  0,                  /* W */
  0,                  /* X */
  0,                  /* Y */
  0,                  /* Z 0x5A */
  0, 0, 0, 0, 0,      /* 0x5F   */
  0,                  /* 0x60   */
  UMODE_ADMIN,        /* a */
  UMODE_BOTS,         /* b */
  UMODE_CCONN,        /* c */
  UMODE_DEBUG,        /* d */
  0,                  /* e */
  UMODE_FULL,         /* f */
  UMODE_CALLERID,     /* g */
  0,                  /* h */
  UMODE_INVISIBLE,    /* i */
  0,                  /* j */
  UMODE_SKILL,        /* k */
  UMODE_LOCOPS,       /* l */
  0,                  /* m */
  UMODE_NCHANGE,      /* n */
  UMODE_OPER,         /* o */
  0,                  /* p */
  0,                  /* q */
  UMODE_REJ,          /* r */
  UMODE_SERVNOTICE,   /* s */
  0,                  /* t */
  UMODE_UNAUTH,       /* u */
  0,                  /* v */
  UMODE_WALLOP,       /* w */
  UMODE_EXTERNAL,     /* x */
  UMODE_SPY,          /* y */
  UMODE_OPERWALL,     /* z      0x7A */
  0,0,0,0,0,          /* 0x7B - 0x7F */

  /* 0x80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x8F */
  /* 0x90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x9F */
  /* 0xA0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xAF */
  /* 0xB0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xBF */
  /* 0xC0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xCF */
  /* 0xD0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xDF */
  /* 0xE0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xEF */
  /* 0xF0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* 0xFF */
};


/* set_user_mode()
 *
 * added 15/10/91 By Darren Reed.
 * parv[0] - sender
 * parv[1] - username to change mode for
 * parv[2] - modes to change
 */
void
set_user_mode(struct Client *client_p, struct Client *source_p,
              int parc, char *parv[])
{
  unsigned int flag, setflags;
  char **p, *m; 
  struct Client *target_p;
  int what = MODE_ADD;

  assert(!(parc < 2));

  if ((target_p = find_person(client_p, parv[1])) == NULL)
  {
    return;
  }

  if (IsServer(source_p))
  {
     return;
  }

  if (source_p != target_p || target_p->from != source_p->from)
  {
     return;
  }

  //execute_callback(entering_umode_cb, client_p, source_p);

  /* find flags already set for user */
  setflags = source_p->umodes;

  /* parse mode change string(s) */
  for (p = &parv[2]; p && *p; p++)
  {
    for (m = *p; *m; m++)
    {
      switch (*m)
      {
        case '+':
          what = MODE_ADD;
          break;
        case '-':
          what = MODE_DEL;
          break;
        case 'o':
          if (what == MODE_ADD)
          {
            if (IsServer(client_p) && !IsOper(source_p))
            {
              printf("Setting %s!%s@%s as oper\n", source_p->name,
                  source_p->username, source_p->host);
              SetOper(source_p);
            }
          }
          else
          {
            /* Only decrement the oper counts if an oper to begin with
             * found by Pat Szuta, Perly , perly@xnet.com
             */
            if (!IsOper(source_p))
              break;

            ClearOper(source_p);

          }

          break;

        /* we may not get these,
         * but they shouldnt be in default
         */
        case ' ' :
        case '\n':
        case '\r':
        case '\t':
          break;


        default:
          if ((flag = user_modes[(unsigned char)*m]))
          {
/*            else
              execute_callback(umode_cb, client_p, source_p, what, flag);*/
          }
          break;
      }
    }
  }

  //send_umode_out(client_p, source_p, setflags);
}

/* register_remote_user()
 *
 * inputs       - client_p directly connected client
 *              - source_p remote or directly connected client
 *              - username to register as
 *              - host name to register as
 *              - server name
 *              - realname (gecos)
 * output - NONE
 * side effects - This function is called when a remote client
 *      is introduced by a server.
 */
void
register_remote_user(struct Client *client_p, struct Client *source_p,
                     const char *username, const char *host, const char *server,
                     const char *realname)
{
  struct Client *target_p = NULL;

  assert(source_p != NULL);
  assert(source_p->username != username);

  strlcpy(source_p->host, host, sizeof(source_p->host));
  strlcpy(source_p->info, realname, sizeof(source_p->info));
  strlcpy(source_p->username, username, sizeof(source_p->username));

  /*
   * coming from another server, take the servers word for it
   */
  source_p->servptr = find_server(server);

  /* Super GhostDetect:
   * If we can't find the server the user is supposed to be on,
   * then simply blow the user away.        -Taner
   */
  if (source_p->servptr == NULL)
  {
    printf("No server %s for user %s[%s@%s] from %s",
        server, source_p->name, source_p->username,
        source_p->host, source_p->from->name);
    exit_client(source_p, &me, "Ghosted Client");
    return;
  }

  if ((target_p = source_p->servptr) && target_p->from != source_p->from)
  {
    printf("Bad User [%s] :%s USER %s@%s %s, != %s[%s]",
        client_p->name, source_p->name, source_p->username,
        source_p->host, source_p->servptr->name,
        target_p->name, target_p->from->name);
    exit_client(source_p, &me, "USER server wrong direction");
    return;
  }

  /* Increment our total user count here */

  SetClient(source_p);
  dlinkAdd(source_p, &source_p->lnode, &source_p->servptr->client_list);
  printf("Adding client %s!%s@%s from %s\n", source_p->name, source_p->username,
      source_p->host, server);
}

/*
 * nick_from_server()
 */
void
nick_from_server(struct Client *client_p, struct Client *source_p, int parc,
                 char *parv[], time_t newts, char *nick, char *ngecos)
{
  int samenick = 0;

  if (IsServer(source_p))
  {
    /* A server introducing a new client, change source */
    source_p = make_client(client_p);
    dlinkAdd(source_p, &source_p->node, &global_client_list);

    if (parc > 2)
      source_p->hopcount = atoi(parv[2]);
    if (newts)
      source_p->tsinfo = newts;
    else
    {
      newts = source_p->tsinfo = CurrentTime;
      printf("Remote nick %s (%s) introduced without a TS", nick, parv[0]);
    }

    /* copy the nick in place */
    strlcpy(source_p->name, nick, sizeof(source_p->name));
    hash_add_client(source_p);

    if (parc > 8)
    {
      unsigned int flag;
      char *m;

      /* parse usermodes */
      m = &parv[4][1];

      while (*m)
      {
        flag = user_modes[(unsigned char)*m];

        source_p->umodes |= flag;
        printf("Setting umode %c on %s\n", *m, source_p->name);
        m++;
      }

      register_remote_user(client_p, source_p, parv[5], parv[6],
                           parv[7], ngecos);
      return;
    }
  }
  else if (source_p->name[0])
  {
    samenick = !irccmp(parv[0], nick);


    /* client changing their nick */
    if (!samenick)
    {
      source_p->tsinfo = newts ? newts : CurrentTime;
    }
  }

  /* set the new nick name */
  assert(source_p->name[0]);

  hash_del_client(source_p);
  strcpy(source_p->name, nick);
  hash_add_client(source_p);
}
