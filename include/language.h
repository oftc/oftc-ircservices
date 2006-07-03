#ifndef LANGUAGE_H
#define LANGUAGE_H

#define LANG_EN       0
#define LANG_NAUGHTY  1
#define LANG_LAST     2

#define LANG_TABLE_SIZE 128

#define _L(s, c, m) ((c)->nickname != NULL) ? \
          (s)->language_table[(c)->nickname->language][(m)] : \
          (s)->language_table[0][(m)]

#endif
