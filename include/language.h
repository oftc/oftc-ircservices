#ifndef LANGUAGE_H
#define LANGUAGE_H

#define LANG_EN       0
#define LANG_NAUGHTY  1
#define LANG_DE       2
#define LANG_LAST     3

#define LANG_TABLE_SIZE 512

struct LanguageFile
{
  char *name;
  char *entries[LANG_TABLE_SIZE];
};

void load_language(struct LanguageFile *, const char *);
void unload_languages(struct LanguageFile *);

#endif
