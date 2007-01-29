/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  perl_module.c: An interface to run perl scripts
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General my_perlublic License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A my_perlARTICULAR my_perlURmy_perlOSE.  See the
 *  GNU General my_perlublic License for more details.
 *
 *  You should have received a copy of the GNU General my_perlublic License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple my_perllace, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#define STUS_HACKERY 1
#ifdef STUS_HACKERY
struct crypt_data
{
  char keysched[16 * 8];
  char sb0[32768];
  char sb1[32768];
  char sb2[32768];
  char sb3[32768];
  /* end-of-aligment-critical-data */
  char crypt_3_buf[14];
  char current_salt[2];
  long int current_saltbits;
  int  direction, initialized;
};
#endif
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include "config.h"
#include <openssl/ssl.h>
#include "irc_libio.h"
#undef FALSE
#undef TRUE
#include "defines.h"
#include "client.h"
#include "channel_mode.h"
#include "channel.h"
#include "language.h"
#include "dbm.h"
#include "parse.h"
#include "interface.h"

#define new_pv(a) \
    (newSVpv((a) == NULL ? "" : (a), (a) == NULL ? 0 : strlen(a)))

PerlInterpreter *my_perl;
extern void boot_DynaLoader(pTHX_ CV* cv);

static const char *core_code =
"package Services::Core;\n"
"\n"
"use Symbol;\n"
"\n"
"sub destroy {\n"
"  eval { $_[0]->UNLOAD() if $_[0]->can('UNLOAD'); };\n"
"  Symbol::delete_package($_[0]);\n"
"}\n"
"\n"
"sub eval_data {\n"
"  my ($data, $id) = @_;\n"
"  #destroy(\"Services::Script::$id\");\n"
"\n"
"  my $package = \"Services::Script::$id\";\n"
"  my $eval = qq{package $package; sub handler { $data; }};\n"
"  {\n"
"      # hide our variables within this block\n"
"      my ($filename, $package, $data);\n"
"      eval $eval;\n"
"  }\n"
"  die $@ if $@;\n"
"\n"
"  my $ret;\n"
"  eval { $ret = $package->handler; };\n"
"  die $@ if $@;\n"
"  return $ret;\n"
"}\n"
"\n"
"sub eval_file {\n"
"  my ($filename, $id) = @_;\n"
"\n"
"  local *FH;\n"
"  open FH, $filename or die \"File not found: $filename\";\n"
"  local($/) = undef;\n"
"  my $data = <FH>;\n"
"  close FH;\n"
"  local($/) = \"\\n\";\n"
"\n"
"  eval_data($data, $id);\n"
"}\n";

XS(plog)
{
  size_t notused;
  char *msg;
  dXSARGS;

  (void) cv;

  if (items < 1)
    XSRETURN_IV(-1);

  msg = SvPV(ST(0), notused);
  ilog(L_DEBUG, "[PERL]: %s", msg);
  XSRETURN_IV(0);
}

XS(register_service)
{
  char *name, *tmp;
  struct Service *perl_service;
  HV *commands;
  int num_cmds, i;
  char *key;
  char *val;
  I32 len;
  dXSARGS;

  (void) cv;

  if (items < 2)
    XSRETURN_IV(-1);

  name = SvPV_nolen(ST(0));
  commands = (HV*)SvRV(ST(1));
  num_cmds = hv_iterinit(commands);
  
  for(i = 0; i < num_cmds; i++)
  {
    SV *sv = hv_iternextsv(commands, &key, &len);
    val = SvPV(sv, PL_na);
    int foo = 0;
  }

  perl_service = make_service(name);
  dlinkAdd(perl_service, &perl_service->node, &services_list);
  hash_add_service(perl_service);
  introduce_client(perl_service->name);

  ilog(L_DEBUG, "[PERL]: Registering Service '%s'", name);
  XSRETURN_IV(0);
}

static void
xs_init(pTHX)
{
  dXSUB_SYS;

  newXS("DynaLoader::boot_Dynaloader", boot_DynaLoader, __FILE__);
  newXS("Services::log", plog, __FILE__);
  newXS("Services::Service::register", register_service, __FILE__);
}

void
init_perl()
{
  char *args[] = {"", "-e", "0"};

  my_perl = perl_alloc();
  perl_construct(my_perl);

  perl_parse(my_perl, xs_init, 3, args, NULL);
  perl_eval_pv(core_code, TRUE);
}

void destroy_perl() {
  perl_destruct(my_perl);
  perl_free(my_perl);
}

int
load_perl_module(const char *name, const char *dir, const char *fname)
{
  dSP;
  char *error;
  int retcount;
  SV *ret;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(new_pv(fname)));
  XPUSHs(sv_2mortal(new_pv(name)));
  PUTBACK;

  ilog(L_NOTICE, "Loading PERL module: %s %s", name, fname);

  retcount = perl_call_pv("Services::Core::eval_file", G_EVAL|G_SCALAR);
  SPAGAIN;

  error = NULL;
  if(SvTRUE(ERRSV))
  {
    error = SvPV(ERRSV, PL_na);
    if(error != NULL)
    {
      printf("PERL ERROR: %s\n", error);
      free(error);
    }
    else if(retcount > 0)
    {
      ret = POPs;
      if(ret != &PL_sv_undef && SvIOK(ret) && SvIV(ret) == 0)
        error = "";
    }
  }

  PUTBACK;
  FREETMPS;
  LEAVE;

  return error == NULL;
}
