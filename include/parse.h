/* TODO: add copyright block */

#ifndef INCLUDED_parse_h
#define INCLUDED_parse_h

struct MessageTree;
struct ServiceMessageTree;
struct ServiceMessage;
struct Service;

struct Message *find_command(const char *, struct MessageTree*);
struct ServiceMessage *find_services_command(const char *cmd, struct ServiceMessageTree *msg_tree);
void parse(struct Client *, char *, char *);
extern void m_ignore(struct Client *, struct Client *, int, char *[]);
extern void m_servignore(struct Service *, struct Client *, int, char *[]);
extern void m_notid(struct Service *, struct Client *, int, char *[]);
extern void m_alreadyreg(struct Service *, struct Client *, int, char *[]);
extern void m_notadmin(struct Service *, struct Client *, int, char *[]);

void mod_add_cmd(struct Message *);
void mod_del_cmd(struct Message *);
void mod_add_servcmd(struct ServiceMessageTree *, struct ServiceMessage *);
void mod_del_servcmd(struct ServiceMessageTree *, struct ServiceMessage *);
void process_privmsg(int, struct Client *, struct Client *, int, char *[]);
void clear_tree_parse(struct MessageTree *);
void clear_serv_tree_parse(struct ServiceMessageTree *);
void init_parser();
void do_serv_help_messages(struct Service *service, struct Client *client);
size_t join_params(char *target, int parc, char *parv[]);
void serv_clear_messages(struct Service *service);

void parse_reopen_log();

#define MAXPTRLEN    32
        /* Must be a power of 2, and
         * larger than 26 [a-z]|[A-Z]
         * its used to allocate the set
         * of pointers at each node of the tree
         * There are MAXPTRLEN pointers at each node.
         * Obviously, there have to be more pointers
         * Than ASCII letters. 32 is a nice number
         * since there is then no need to shift
         * 'A'/'a' to base 0 index, at the expense
         * of a few never used pointers. For a small
         * parser like this, this is a good compromise
         * and does make it somewhat faster.
         *
         * - Dianora
         */

struct MessageTree
{
  int links; /* Count of all pointers (including msg) at this node
              * used as reference count for deletion of
              * _this_ node.
              *               */
  struct Message *msg;
  struct MessageTree *pointers[MAXPTRLEN];
};

struct ServiceMessageTree
{
  int links; /* Count of all pointers (including msg) at this node
              * used as reference count for deletion of
              *_this_ node.
              */
  struct ServiceMessage *msg;
  struct ServiceMessageTree *pointers[MAXPTRLEN];
};


#endif /* INCLUDED_parse_h */
