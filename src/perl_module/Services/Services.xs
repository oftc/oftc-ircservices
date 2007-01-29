#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

MODULE = Services		PACKAGE = Services  PREFIX = perl_

int
log(const char *format)
  CODE:
    RETVAL = ilog("%s", format);
    if(RETVAL < 0)
      XSRETURN_UNDEF;
  OUTPUT:
    RETVAL
