#include "servicemask.h"
#include "dbm.h"

static struct ServiceMask *
row_to_servicemask(row_t *row)
{
  struct ServiceMask *sban;

  sban = MyMalloc(sizeof(struct ServiceMask));
  sban->id = atoi(row->cols[0]);
  sban->channel = atoi(row->cols[1]);
  if(row->cols[2] != NULL)
    sban->target = atoi(row->cols[2]);
  else
    sban->target = 0;
  sban->setter = atoi(row->cols[3]);
  DupString(sban->mask, row->cols[4]);
  DupString(sban->reason, row->cols[5]);
  sban->time_set = atoi(row->cols[6]);
  sban->duration = atoi(row->cols[7]);
  sban->type = atoi(row->cols[8]);

  return sban;
}

void
free_servicemask(struct ServiceMask *ban)
{
  ilog(L_DEBUG, "Freeing servicemask %p for %s", ban, ban->mask);
  MyFree(ban->mask);
  MyFree(ban->reason);
  MyFree(ban);
}


static int
servicemask_add(const char *mask, unsigned int setter, unsigned int channel,
  unsigned int time_set, unsigned int duration, const char *reason, unsigned int mode)
{
  int ret;

  ret = db_execute_nonquery(INSERT_AKICK_MASK, "iissiii", &channel,
      &setter, reason, mask, &time_set, &duration, &mode);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
servicemask_add_akick_target(unsigned int target, unsigned int setter, unsigned int channel,
  unsigned int time_set, unsigned int duration, const char *reason)
{
  int ret;
  unsigned int type = AKICK_MASK;

  ret = db_execute_nonquery(INSERT_AKICK_ACCOUNT, "iiisiii", &channel, &target,
    &setter, reason, &time_set, &duration, &type);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
servicemask_add_akick(const char *mask, unsigned int setter, unsigned int channel,
  unsigned int time_set, unsigned int duration, const char *reason)
{
  return servicemask_add(mask, setter, channel, time_set, duration, reason, AKICK_MASK);
}

int
servicemask_add_invex(const char *mask, unsigned int setter, unsigned int channel,
  unsigned int time_set, unsigned int duration, const char *reason)
{
  return servicemask_add(mask, setter, channel, time_set, duration, reason, INVEX_MASK);
}

int
servicemask_add_excpt(const char *mask, unsigned int setter, unsigned int channel,
  unsigned int time_set, unsigned int duration, const char *reason)
{
  return servicemask_add(mask, setter, channel, time_set, duration, reason, EXCPT_MASK);
}

int
servicemask_add_quiet(const char *mask, unsigned int setter, unsigned int channel,
  unsigned int time_set, unsigned int duration, const char *reason)
{
  return servicemask_add(mask, setter, channel, time_set, duration, reason, QUIET_MASK);
}

static int
servicemask_remove(unsigned int channel, const char *mask, unsigned int mode)
{
  int ret = db_execute_nonquery(DELETE_AKICK_MASK, "isi", &channel, mask, &mode);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
servicemask_remove_akick_target(unsigned int channel, const char *account)
{
  unsigned int mode = AKICK_MASK;
  int ret = db_execute_nonquery(DELETE_AKICK_ACCOUNT, "isi", &channel, account, &mode);
  return ret == -1 ? FALSE : TRUE;
}

int
servicemask_remove_akick(unsigned int channel, const char *mask)
{
  return servicemask_remove(channel, mask, AKICK_MASK);
}

int
servicemask_remove_invex(unsigned int channel, const char *mask)
{
  return servicemask_remove(channel, mask, INVEX_MASK);
}

int
servicemask_remove_excpt(unsigned int channel, const char *mask)
{
  return servicemask_remove(channel, mask, EXCPT_MASK);
}

int
servicemask_remove_quiet(unsigned int channel, const char *mask)
{
  return servicemask_remove(channel, mask, QUIET_MASK);
}

static int
servicemask_list(unsigned int channel, unsigned int mode, dlink_list *list)
{
  result_set_t *results;
  int error;
  int i;

  results = db_execute(GET_AKICKS, &error, "ii", &channel, &mode);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "servicemask_list: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    struct ServiceMask *sban = row_to_servicemask(row);

    dlinkAdd(sban, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

void
servicemask_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct ServiceMask *sban;

  ilog(L_DEBUG, "Freeing servicemask list %p of length %lu", list, 
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    sban = (struct ServiceMask *)ptr->data;
    free_servicemask(sban);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
servicemask_list_akick(unsigned int channel, dlink_list *list)
{
  return servicemask_list(channel, AKICK_MASK, list);
}

int
servicemask_list_invex(unsigned int channel, dlink_list *list)
{
  return servicemask_list(channel, INVEX_MASK, list);
}

int
servicemask_list_excpt(unsigned int channel, dlink_list *list)
{
  return servicemask_list(channel, EXCPT_MASK, list);
}

int
servicemask_list_quiet(unsigned int channel, dlink_list *list)
{
  return servicemask_list(channel, QUIET_MASK, list);
}

static int
servicemask_list_masks(unsigned int channel, unsigned int mode, dlink_list *list)
{
  int error, i;
  result_set_t *results;

  results = db_execute(GET_SERVICEMASK_MASKS, &error, "ii", &channel, &mode);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "servicemask_list_masks: CHMODE %d CHANNEL %d ERROR %d",
      channel, mode, error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; ++i)
  {
    char *tmp;
    row_t *row = &results->rows[i];
    DupString(tmp, row->cols[0]);
    dlinkAdd(tmp, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

void
servicemask_list_masks_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
servicemask_list_akick_masks(unsigned int channel, dlink_list *list)
{
  return servicemask_list_masks(channel, AKICK_MASK, list);
}

int
servicemask_list_invex_masks(unsigned int channel, dlink_list *list)
{
  return servicemask_list_masks(channel, INVEX_MASK, list);
}

int
servicemask_list_quiet_masks(unsigned int channel, dlink_list *list)
{
  return servicemask_list_masks(channel, QUIET_MASK, list);
}

int
servicemask_list_excpt_masks(unsigned int channel, dlink_list *list)
{
  return servicemask_list_masks(channel, EXCPT_MASK, list);
}
