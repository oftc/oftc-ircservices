from django.db import models

class Nickname(models.Model):
  nick = models.CharField(max_length=255)
  account = models.ForeignKey('account.Account')
  reg_time = models.IntegerField()
  last_seen = models.IntegerField()
  class Meta:
    db_table = u'nickname'

class ForbiddenNickname(models.Model):
  nick = models.CharField(max_length=255, primary_key=True)
  class Meta:
    db_table = u'forbidden_nickname'
