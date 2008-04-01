#include <ruby.h>
#include "libruby_module.h"

VALUE cDBResult = Qnil;

static VALUE initialize(VALUE, VALUE);
static VALUE row_count(VALUE);
static VALUE row_each(VALUE);
static VALUE m_free(VALUE);

void
Init_DBResult(void)
{
  cDBResult = rb_define_class("DBResult", rb_cObject);

  rb_define_method(cDBResult, "initialize", initialize, 1);
  rb_define_method(cDBResult, "row_count", row_count, 0);
  rb_define_method(cDBResult, "row_each", row_each, 0);
  rb_define_method(cDBResult, "free", m_free, 0);
}

static VALUE
initialize(VALUE self, VALUE result)
{
  rb_iv_set(self, "@realptr", result);
  return self;
}

static VALUE
row_count(VALUE self)
{
  result_set_t *result = value_to_dbresult(self);
  return INT2NUM(result->row_count);
}

static VALUE
row_each(VALUE self)
{
  if(rb_block_given_p())
  {
    result_set_t *result = value_to_dbresult(self);
    int i = 0;
    for(i = 0; i < result->row_count; i++)
    {
      rb_yield(dbrow_to_value(&result->rows[i]));
    }
  }

  return self;
}

static VALUE
m_free(VALUE self)
{
  result_set_t *result = value_to_dbresult(self);
  db_free_result(result);
  return self;
}

result_set_t*
value_to_dbresult(VALUE self)
{
  result_set_t *out;
  VALUE result = rb_iv_get(self, "@realptr");
  Data_Get_Struct(result, result_set_t, out);
  return out;
}

VALUE
dbresult_to_value(result_set_t *result)
{
  VALUE tmp, real;

  tmp = Data_Wrap_Struct(rb_cObject, 0, 0, result);
  real = do_ruby_ret(cDBResult, rb_intern("new"), 1, tmp);

  if(real == Qnil)
  {
    ilog(L_CRIT, "RUBY ERROR: Ruby Failed To Create DBResult");
    return Qnil;
  }

  return real;
}
