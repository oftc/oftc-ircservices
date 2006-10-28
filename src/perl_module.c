typedef        struct crypt_data {     /* straight from /usr/include/crypt.h */
  /* From OSF, Not needed in AIX
   *        char C[28], D[28];
   *            */
  char E[48];
  char KS[16][48];
  char block[66];
  char iobuf[16];
} CRYPTD;

#include <EXTERN.h>
#include <perl.h>
#undef load_module
#undef my_perl
#undef opendir
#undef readdir
#undef strerror

PerlInterpreter *P;

void
init_perl()
{
  P = perl_alloc();
  perl_construct(P);
}

int
load_perl_module(const char *name, const char *dir, const char *fname)
{
  int status;
  char path[PATH_MAX];
  char *embedding[] = { "",  path };

  snprintf(path, sizeof(path), "%s/%s", dir, fname);

  printf("Loading perl module: %s\n", path);
  status = perl_parse(P, NULL, 2, embedding, NULL);
  if(status != 0)
    return 0;
  status = perl_run(P);
  if(status != 0)
    return 0;

  return 1;
}
