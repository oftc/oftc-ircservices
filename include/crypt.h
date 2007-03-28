/* TODO: add copyright block */

#ifndef INCLUDED_crypt_h
#define INCLUDED_crypt_h

#define DIGEST_FUNCTION "SHA1"
#define DIGEST_LEN 20

char *generate_md5_salt(char *, int);
char *crypt_pass(char *, int);

#endif /* INCLUDED_crypt_h */
