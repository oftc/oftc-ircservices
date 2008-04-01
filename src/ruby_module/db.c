#include <ruby.h>
#include "libruby_module.h"

VALUE cDB = Qnil;

static VALUE prepare(VALUE, VALUE);
static VALUE execute(int, VALUE *, VALUE);
static VALUE execute_scalar(int, VALUE *, VALUE);
static VALUE execute_nonquery(int, VALUE *, VALUE);

void
Init_DB(void)
{
  cDB = rb_define_class("DB", rb_cObject);

  rb_define_singleton_method(cDB, "prepare", prepare, 1);
  rb_define_singleton_method(cDB, "execute", execute, -1);
  rb_define_singleton_method(cDB, "execute_scalar", execute_scalar, -1);
  rb_define_singleton_method(cDB, "execute_nonquery", execute_nonquery, -1);
}

static VALUE
prepare(VALUE self, VALUE query)
{
  return INT2NUM(db_prepare(-1, StringValueCStr(query)));
}

static int
create_db_list(int argc, VALUE *argv, VALUE self,
  char **format, int *query_id, dlink_list *list)
{
  VALUE query, fmt, args, arg;
  size_t len = 0, ary_len = 0, i = 0;
  void *ptr = NULL;
  int *iptr = NULL;
  char *cptr = NULL;

  rb_scan_args(argc, argv, "2*", &query, &fmt, &args);

  *query_id = NUM2INT(query);
  *format = StringValueCStr(fmt);
  len = strlen(*format);
  ary_len = RARRAY(args)->len;

  if(len != ary_len)
  {
    ilog(L_CRIT, "Wrong argument count, got %zu expected %zu", ary_len, len);
    return FALSE;
  }

  for(i = 0; i < len; ++i)
  {
    arg = rb_ary_entry(args, i);

    if(arg == Qnil)
    {
      dlinkAddTail(NULL, make_dlink_node(), list);
      continue;
    }

    switch(*(*format+i))
    {
      case 'i':
        Check_Type(arg, T_FIXNUM);
        iptr = MyMalloc(sizeof(int));
        *iptr = NUM2INT(arg);
        ptr = iptr;
        break;
      case 'b':
        if(arg == Qtrue || arg == Qfalse)
        {
          cptr = MyMalloc(sizeof(char));
          (*cptr) = arg == Qtrue ? 1 : 0;
          ptr = cptr;
        }
        else
        {
          ilog(L_CRIT, "Not a Boolean Argument: TYPE(%d)", TYPE(arg));
          /* TODO FIXME XXX throw exception */
          return FALSE;
        }
        break;
      case 's':
        Check_Type(arg, T_STRING);
        cptr = MyMalloc(sizeof(char) * strlen(StringValueCStr(arg)));
        DupString(cptr, StringValueCStr(arg));
        ptr = cptr;
        break;
      default:
        ilog(L_CRIT, "Unknown Format Type: %c", *(*format+i));
        /* TODO FIXME XXX throw exception */
        return FALSE;
        break;
    }
    dlinkAddTail(ptr, make_dlink_node(), list);
  }

  return TRUE;
}

static void
cleanup_db_list(dlink_list *list)
{
  dlink_node *ptr = NULL;
  DLINK_FOREACH(ptr, list->head)
  {
    MyFree(ptr->data);
  }
}

static VALUE
execute(int argc, VALUE *argv, VALUE self)
{
  dlink_list list = { 0 };
  char *format;
  int query_id, error;
  result_set_t *results;
  VALUE dbresult;

  /* TODO XXX FIXME catch cleanup list and rethrow exceptions */
  if(create_db_list(argc, argv, self, &format, &query_id, &list))
  {
    results = db_vexecute(query_id, &error, format, &list);
    if(error)
    {
      db_log("Ruby Database Error: %d", error);
      cleanup_db_list(&list);
      return Qnil;
    }

    if(results == NULL)
    {
      cleanup_db_list(&list);
      return Qnil;
    }

    dbresult = dbresult_to_value(results);
    cleanup_db_list(&list);
    return dbresult;
  }

  return Qnil;
}

static VALUE
execute_scalar(int argc, VALUE *argv, VALUE self)
{
  dlink_list list = { 0 };
  char *format, *result;
  int query_id, error;

  /* TODO XXX FIXME catch cleanup list and rethrow exceptions */
  if(create_db_list(argc, argv, self, &format, &query_id, &list))
  {
    result = db_vexecute_scalar(query_id, &error, format, &list);
    if(error)
    {
      db_log("Ruby Database Error: %d", error);
      cleanup_db_list(&list);
      return Qnil;
    }

    if(result == NULL)
    {
      cleanup_db_list(&list);
      return Qnil;
    }

    cleanup_db_list(&list);
    return rb_str_new2(result);
  }

  return Qnil;
}

static VALUE
execute_nonquery(int argc, VALUE *argv, VALUE self)
{
  dlink_list list = { 0 };
  char *format;
  int query_id, rows;

  /* TODO XXX FIXME catch cleanup list and rethrow exceptions */
  if(create_db_list(argc, argv, self, &format, &query_id, &list))
  {
    rows = INT2NUM(db_vexecute_nonquery(query_id, format, &list));
    cleanup_db_list(&list);
    return rows;
  }

  return Qnil;
}
