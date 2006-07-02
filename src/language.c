#include "stdinc.h"

void
load_language(struct Service *service, const char *langfile)
{
  FBFILE *file;
  char buffer[256];
  char *s;
  int lang, i = 0;

  snprintf(buffer, sizeof(buffer), "%s/%s.lang", LANGPATH, langfile);

  if((file = fbopen(buffer, "r")) == NULL)
  {
    printf("Failed to open language file %s for service %s\n", langfile,
        service->name);
    return;
  }
  
  /* Read the first line which tells us which language this is */
  fbgets(buffer, sizeof(buffer), file);
  if((s = strchr(buffer, ' ')) == NULL)
  {
    printf("Language file %s for service %s is invalid\n", langfile,
        service->name);
    return;
  }

  *s = '\0';
  lang = atoi(buffer);

  printf("Loading language %d(%s) for service %s\n", lang, langfile,
      service->name);

  while(fbgets(buffer, sizeof(buffer), file) != NULL)
  {
    if((s = strchr(buffer, ' ')) == NULL)
    {
      printf("Language file %s for service %s is invalid\n", langfile,
          service->name);
      return;
    }
    *s = '\0';
    s++;

    /* Skip spaces */
    while(*s == ' ')
      s++;

    DupString(service->language_table[lang][i++], s);
  }
}
