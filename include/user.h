#ifndef USER_H
#define USER_H

enum identify_errors
{
  ERR_ID_NOERROR = 0,
  ERR_ID_NONICK,
  ERR_ID_WRONGPASS
};

int identify_user(struct Client *, const char *);

#endif
