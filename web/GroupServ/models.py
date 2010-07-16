from django.db import models

class Group(models.Model):
  name = models.CharField(max_length=32)
  description = models.CharField(max_length=255)
  url = models.CharField(max_length=255)
  email = models.CharField(max_length=255)
  flag_private = models.BooleanField()
  reg_time = models.IntegerField()
  class Meta:
    db_table = u'group'

class GroupAccess(models.Model):
  group = models.ForeignKey(Group)
  account = models.ForeignKey('account.Account')
  level = models.IntegerField()
  class Meta:
    db_table = u'group_access'
