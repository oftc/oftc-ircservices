/* TODO: add copyright block */

#ifndef INCLUDED_ruby_module_h
#define INCLUDED_ruby_module_h

void init_ruby();
void cleanup_ruby();
int load_ruby_module(const char *name, const char *dir, const char *fname);

#endif /* INCLUDED_ruby_module_h */
