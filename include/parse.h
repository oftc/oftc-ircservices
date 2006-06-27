#ifndef PARSE_H
#define PARSE_H

struct Message *find_command(const char *);
void parse(struct Client *, char *, char *);
extern void m_ignore(struct Client *, struct Client *, int, char *[]);
void mod_add_cmd(struct Message *);
void mod_del_cmd(struct Message *);

#endif
