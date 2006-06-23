#ifndef PARSE_H
#define PARSE_H

message_t *find_command(const char *);
extern void m_ignore(client_t *, client_t *, int, char *[]);

#endif
