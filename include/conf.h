#ifndef CONFHINC
#define CONFHINC

int conf_fbgets(char *, int, FBFILE *);
int conf_yy_fatal_error(const char *); 
void read_services_conf(int);

typedef struct 
{
  char        *name;
  char        *description;
#ifdef HAVE_LIBCRYPTO
  char        *rsa_private_key_file;
  RSA         *rsa_private_key;
  char        *ssl_certificate_file;
  SSL_CTX     *ctx;
  SSL_METHOD  *meth;
#endif
  char        *sid;
  struct      irc_ssaddr ip;
  struct      irc_ssaddr ip6;
  int         specific_ipv4_vhost;
  int         specific_ipv6_vhost;
  struct      sockaddr_in dns_host;
  int         can_use_v6;
} services_info_t;

extern FBFILE *conf_fbfile_in;
extern services_info_t services_info;

#endif
