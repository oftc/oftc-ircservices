#ifndef RUBY_MODULE_H
#define RUBY_MODULE_H

void init_ruby();
int load_ruby_module(const char *name, const char *dir, const char *fname);

#endif
