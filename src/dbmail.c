#include "stdinc.h"
#include "conf/conf.h"
#include "dbm.h"
#include "dbmail.h"

int
dbmail_add_sent(unsigned int account, const char *email)
{
  if(db_execute_nonquery(INSERT_SENT_MAIL, "isi", &account, email, &CurrentTime))
    return TRUE;
  else
    return FALSE;
}

int
dbmail_is_sent(unsigned int account, const char *email)
{
  int error, ret;
  char *result;

  result = db_execute_scalar(GET_SENT_MAIL, &error, "is", &account, email);
  if(result != NULL)
    ret = atoi(result);
  else
    ret = FALSE;

  if(error)
  {
    ilog(L_CRIT, "dbmail_is_sent: database error %d", error);
    return FALSE;
  }

  return ret;
}

void
dbmail_expire_sentmail(void *param)
{
  db_execute_nonquery(DELETE_EXPIRED_SENT_MAIL, "ii", &Mail.expire_time, &CurrentTime);
}
