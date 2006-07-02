#include "stdinc.h"

static const char saltChars[] = 
  "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  /* 0 .. 63, ascii - 64 */

char *
generate_md5_salt(char *salt, int length)
{
  int i;
  srandom(time(NULL));
  for(i = 0; i < length; i++)
  {
    salt[i] = saltChars[random() % 64];
  }
  return(salt);
}

char *
crypt_pass(char *password)
{
  char salt[16];

  return servcrypt(password, generate_md5_salt(salt, 16));
}
