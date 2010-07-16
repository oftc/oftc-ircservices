from django.db import models
from django.contrib.auth.models import User

class Account(models.Model):
    primary_nick = models.ForeignKey("NickServ.Nickname", db_column='primary_nick', related_name='primary_nick_set')
    password = models.CharField(max_length=40)
    salt = models.CharField(max_length=16)
    url = models.CharField(max_length=255)
    email = models.CharField(max_length=255)
    cloak = models.CharField(max_length=255)
    flag_enforce = models.BooleanField()
    flag_secure = models.BooleanField()
    flag_verified = models.BooleanField()
    flag_cloak_enabled = models.BooleanField()
    flag_admin = models.BooleanField()
    flag_email_verified = models.BooleanField()
    flag_private = models.BooleanField()
    language = models.IntegerField()
    last_host = models.CharField(max_length=255)
    last_realname = models.CharField(max_length=255)
    last_quit_msg = models.CharField(max_length=512)
    last_quit_time = models.IntegerField()
    reg_time = models.IntegerField()
    user = models.ForeignKey(User, unique=True, null=True)

    class Meta:
        db_table = u'account'

class AccountFingerprint(models.Model):
    account = models.ForeignKey(Account)
    fingerprint = models.CharField(unique=True, max_length=40)
    nickname = models.ForeignKey("NickServ.Nickname")
    class Meta:
        db_table = u'account_fingerprint'

class AccountAccess(models.Model):
    account = models.ForeignKey(Account)
    entry = models.CharField(max_length=255)
    class Meta:
        db_table = u'account_access'

class AccountAutojoin(models.Model):
    account = models.ForeignKey(Account)
    channel_id = models.IntegerField()
    class Meta:
        db_table = u'account_autojoin'

