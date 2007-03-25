/* TODO: add copyright block */

#ifndef INCLUDED_client_h
#define INCLUDED_client_h

extern dlink_list global_client_list;
extern dlink_list global_server_list;
EXTERN unsigned int user_modes[];

#define FLAGS_PINGSENT      0x00000001UL /* Unreplied ping sent*/
#define FLAGS_DEADSOCKET    0x00000002UL /* Local socket is dead--Exiting soon*/
#define FLAGS_KILLED        0x00000004UL /* Prevents "QUIT" from being sent to this */
#define FLAGS_CLOSING       0x00000008UL /* set when closing */
#define FLAGS_CONNECTING    0x00000010UL /* Currently connecting not bursted */
#define FLAGS_ONACCESS      0x00000020UL /* Client isnt authed with nickserv but does match the access list*/
#define FLAGS_ENFORCE       0x00000040UL /* User is to be enforced */

#define STAT_SERVER         0x01
#define STAT_CLIENT         0x02
#define STAT_ME             0x04

/* umodes, settable flags */
#define UMODE_SERVNOTICE   0x00001 /* server notices such as kill */
#define UMODE_CCONN        0x00002 /* Client Connections */
#define UMODE_REJ          0x00004 /* Bot Rejections */
#define UMODE_SKILL        0x00008 /* Server Killed */
#define UMODE_FULL         0x00010 /* Full messages */
#define UMODE_SPY          0x00020 /* see STATS / LINKS */
#define UMODE_DEBUG        0x00040 /* 'debugging' info */
#define UMODE_NCHANGE      0x00080 /* Nick change notice */
#define UMODE_WALLOP       0x00100 /* send wallops to them */
#define UMODE_OPERWALL     0x00200 /* Operwalls */
#define UMODE_INVISIBLE    0x00400 /* makes user invisible */
#define UMODE_BOTS         0x00800 /* shows bots */
#define UMODE_EXTERNAL     0x01000 /* show servers introduced and splitting */
#define UMODE_CALLERID     0x02000 /* block unless caller id's */
#define UMODE_SOFTCALLERID 0x04000 /* block unless on common channel */
#define UMODE_UNAUTH       0x08000 /* show unauth connects here */
#define UMODE_LOCOPS       0x10000 /* show locops */
#define UMODE_DEAF         0x20000 /* don't receive channel messages */

/* user information flags, only settable by remote mode or local oper */
#define UMODE_OPER         0x40000 /* Operator */
#define UMODE_ADMIN        0x80000 /* Admin on server */
#define UMODE_IDENTIFIED  0x100000 /* Registered with nickserv */
#define UMODE_ALL    UMODE_SERVNOTICE

#define IsConnecting(x)         ((x)->flags & FLAGS_CONNECTING)
#define IsDefunct(x)            ((x)->flags & (FLAGS_DEADSOCKET|FLAGS_CLOSING))
#define IsDead(x)               ((x)->flags & FLAGS_DEADSOCKET)
#define IsClosing(x)            ((x)->flags & FLAGS_CLOSING)
#define IsOnAccess(x)           ((x)->flags & FLAGS_ONACCESS)
#define IsEnforce(x)            ((x)->flags & FLAGS_ENFORCE)

#define SetConnecting(x)        ((x)->flags |= FLAGS_CONNECTING)
#define SetClosing(x)           ((x)->flags |= FLAGS_CLOSING)
#define SetOnAccess(x)          ((x)->flags |= FLAGS_ONACCESS)
#define SetEnforce(x)           ((x)->flags |= FLAGS_ENFORCE)

#define ClearConnecting(x)      ((x)->flags &= ~FLAGS_CONNECTING)
#define ClearOnAccess(x)        ((x)->flags &= ~FLAGS_ONACCESS)
#define ClearEnforce(x)         ((x)->flags &= ~FLAGS_ENFORCE)

#define IsServer(x)             ((x)->status & STAT_SERVER)
#define IsClient(x)             ((x)->status & STAT_CLIENT)
#define IsMe(x)                 ((x)->status & STAT_ME)

#define SetServer(x)            ((x)->status |= STAT_SERVER)
#define SetClient(x)            ((x)->status |= STAT_CLIENT)
#define SetMe(x)                ((x)->status |= STAT_ME)

#define IsOper(x)               ((x)->umodes & UMODE_OPER)
#define IsIdentified(x)         ((x)->umodes & UMODE_IDENTIFIED)
#define IsAdmin(x)              ((x)->umodes & UMODE_ADMIN)

#define SetOper(x)              ((x)->umodes |= UMODE_OPER)
#define SetIdentified(x)        ((x)->umodes |= UMODE_IDENTIFIED)
#define SetAdmin(x)             ((x)->umodes |= UMODE_ADMIN)

#define ClearOper(x)            ((x)->umodes &= ~UMODE_OPER)
#define ClearIdentified(x)      ((x)->umodes &= ~UMODE_IDENTIFIED)

#define MyConnect(x)            ((x)->from == &me)
#define MyClient(x)             (MyConnect(x) && IsClient(x))

#define IDLEN           12 /* this is the maximum length, not the actual
                              generated length; DO NOT CHANGE! */

#define MODE_QUERY  0
#define MODE_ADD    1
#define MODE_DEL   -1

struct Server
{
  dlink_node node;
  fde_t fd;
  int flags;
  struct dbuf_queue buf_recvq;
  struct dbuf_queue buf_sendq;
  char pass[PASSLEN+1];
};

struct Client
{
  dlink_node node;
  dlink_node lnode;
  dlink_list channel;

  dlink_list server_list;   /**< Servers on this server      */
  dlink_list client_list;   /**< Clients on this server      */


  struct Client *hnext;         /* For client hash table lookups by name */
  struct Client *idhnext;       /* For SID hash table lookups by sid */
  struct Client *from;
  struct Client *servptr;
  struct Client *uplink;        /* services uplink server */
  struct Client *release_to;    /* The client this one will give its nick to */

  struct Nick   *nickname;

  char          name[HOSTLEN+1];
  char          host[HOSTLEN+1];
  char          sockhost[HOSTLEN+1];
  char          id[IDLEN + 1];      /* client ID, unique ID per client */
  char          info[REALLEN + 1];  /* Free form additional client info */
  char          username[USERLEN + 1];
    
  struct Server      *server;

  time_t        tsinfo;
  time_t        enforce_time;
  time_t        release_time;
  unsigned int  status;
  unsigned int  umodes;
  unsigned int  access;
  unsigned int  num_badpass;    /* Number of incorrect passwords */
  unsigned int  hopcount;
  unsigned char handler;        /* Handler index */
  int flags;
} Client;

void init_client();
struct Client *make_client(struct Client*);
struct Server *make_server(struct Client*);
struct Client *find_person(const struct Client *, const char *);
struct Client *find_chasing(struct Client *, const char *, int *);
void dead_link_on_write(struct Client *, int);
void set_user_mode(struct Client *, struct Client *, int, char *[]);
void exit_client(struct Client *, struct Client *, const char *);
int check_clean_nick(struct Client *, struct Client *, char *, char *, 
    struct Client *);
int check_clean_user(struct Client *, char *, char *, struct Client *);
int check_clean_host(struct Client *, char *, char *, struct Client *);
void nick_from_server(struct Client *, struct Client *, int,
                     char *[], time_t, char *, char *);
void register_remote_user(struct Client *, struct Client *,
                         const char *, const char *, const char *, const char *);

#endif /* INCLUDED_client_h */
