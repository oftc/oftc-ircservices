#include <ruby.h>
#include "libruby_module.h"

VALUE cDBRow = Qnil;

static VALUE initialize(VALUE, VALUE);
static VALUE indexer(VALUE, VALUE);

void
Init_DBRow(void)
{
  cDBRow = rb_define_class("DBRow", rb_cObject);

  rb_define_method(cDBRow, "initialize", initialize, 1);
  rb_define_method(cDBRow, "[]", indexer, 1);
}

static VALUE
initialize(VALUE self, VALUE row)
{
  rb_iv_set(self, "@realptr", row);
  return self;
}

static VALUE
indexer(VALUE self, VALUE index)
{
  row_t *row = value_to_dbrow(self);

  Check_Type(index, T_FIXNUM);

  if(row->cols[NUM2INT(index)] == NULL)
    return Qnil;
  else
    return rb_str_new2(row->cols[NUM2INT(index)]);
}

row_t*
value_to_dbrow(VALUE self)
{
  row_t *out;
  VALUE row = rb_iv_get(self, "@realptr");
  Data_Get_Struct(row, row_t, out);
  return out;
}

VALUE
dbrow_to_value(row_t *row)
{
  VALUE tmp, real;

  tmp = Data_Wrap_Struct(rb_cObject, 0, 0, row);
  real = do_ruby_ret(cDBRow, rb_intern("new"), 1, tmp);

  if(real == Qnil)
  {
    ilog(L_CRIT, "RUBY ERROR: Ruby Failed To Create DBRow");
    return Qnil;
  }

  return real;
}
