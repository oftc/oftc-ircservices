/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  language.c: Language file functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#include "stdinc.h"

void
load_language(struct LanguageFile *language, const char *langfile)
{
  FBFILE *file;
  char buffer[256];
  char *s;
  int lang, i = 0;

  snprintf(buffer, sizeof(buffer), "%s/%s.lang", LANGPATH, langfile);

  if((file = fbopen(buffer, "r")) == NULL)
  {
    ilog(L_DEBUG, "Failed to open language file %s (%s)", langfile, buffer);
    return;
  }
  
  /* Read the first line which tells us which language this is */
  fbgets(buffer, sizeof(buffer), file);
  if((s = strchr(buffer, ' ')) == NULL)
  {
    ilog(L_DEBUG, "Language file %s is invalid", langfile);
    return;
  }

  *s++ = '\0';
  lang = atoi(buffer);
 
  if(s[strlen(s) - 1] == '\n')
    s[strlen(s) - 1] = '\0';
  
  DupString(language[lang].name, s);
  
  ilog(L_DEBUG, "Loading language %d(%s)", lang, langfile);

  while(fbgets(buffer, sizeof(buffer), file) != NULL)
  {
    char *ptr;

    if(buffer[0] != '\t')
    {
      i++;
      continue;
    }

    s = buffer;
    s++;

    if(language[lang].entries[i] != NULL)
    {
      ptr = MyMalloc(strlen(language[lang].entries[i]) + strlen(s) + 2);
      sprintf(ptr, "%s\n%s", language[lang].entries[i], s);

      if(ptr[strlen(ptr)-1] == '\n')
        ptr[strlen(ptr)-1] = '\0';
 
      MyFree(language[lang].entries[i]);
    }
    else
    {
      DupString(ptr, s);
      if(ptr[strlen(ptr)-1] == '\n')
        ptr[strlen(ptr)-1] = '\0';
    }
    language[lang].entries[i] = ptr;
  }
  fbclose(file);
}

void
unload_languages(struct LanguageFile *languages)
{
  int i, j = 1;

  for(i = 0; i < LANG_LAST; i++)
  {
    ilog(L_DEBUG, "Unloading language %s", languages[i].name);
    if(languages[i].name == NULL)
      continue;
  
    MyFree(languages[i].name);
    while(languages[i].entries[j] != NULL)
    {
      MyFree(languages[i].entries[j]);
      j++;
    }
  }
}
