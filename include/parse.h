#ifndef PARSE_H
#define PARSE_H

struct Message *find_command(const char *);
extern void m_ignore(struct Client *, struct Client *, int, char *[]);

#endif
