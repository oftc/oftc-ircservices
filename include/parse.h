#ifndef PARSE_H
#define PARSE_H

struct MessageTree;

struct Message *find_command(const char *, struct MessageTree*);
void parse(struct Client *, char *, char *);
extern void m_ignore(struct Client *, struct Client *, int, char *[]);
void mod_add_cmd(struct Message *);
void mod_del_cmd(struct Message *);
void mod_add_servcmd(struct MessageTree *, struct Message *);
void mod_del_servcmd(struct MessageTree *, struct Message *);
void process_privmsg(struct Client *, struct Client *, int, char *[]);
void clear_tree_parse(struct MessageTree *);

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

#endif
