#ifndef INCLUDED_dbmail_h
#define INCLUDED_dbmail_h

int dbmail_add_sent(unsigned int, const char*);
int dbmail_is_sent(unsigned int, const char*);
void dbmail_expire_sentmail(void*);

#endif
