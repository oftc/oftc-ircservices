from django.db import models

CHACCESS_LEVEL = (
  (1, 'Identified User'),
  (2, 'Channel Member'),
  (3, 'Channel Operator'),
  (4, 'Channel Master'),
)

CHMODE_FLAG = (
  (0, 'AKick'),
  (1, 'AKill'),
  (2, 'Invex'),
  (3, 'Except'),
  (4, 'Quiet'),
)

class Channel(models.Model):
  channel = models.CharField(max_length=255)
  flag_private = models.BooleanField()
  flag_restricted = models.BooleanField()
  flag_topic_lock = models.BooleanField()
  flag_verbose = models.BooleanField()
  flag_autolimit = models.BooleanField()
  flag_expirebans = models.BooleanField()
  flag_floodserv = models.BooleanField()
  flag_autoop = models.BooleanField()
  flag_autovoice = models.BooleanField()
  flag_leaveops = models.BooleanField()
  flag_autosave = models.BooleanField()
  description = models.CharField(max_length=512)
  url = models.CharField(max_length=255)
  email = models.CharField(max_length=255)
  entrymsg = models.CharField(max_length=512)
  topic = models.CharField(max_length=512)
  mlock = models.CharField(max_length=255)
  expirebans_lifetime = models.IntegerField()
  reg_time = models.IntegerField()
  last_used = models.IntegerField()
  class Meta:
    db_table = u'channel'

class ChannelAKick(models.Model):
  channel = models.ForeignKey(Channel)
  setter = models.ForeignKey('account.Account', db_column='setter', related_name='setter_set')
  target = models.ForeignKey('account.Account', db_column='target', related_name='target_set')
  mask = models.CharField(max_length=255)
  reason = models.CharField(max_length=512)
  time = models.IntegerField()
  duration = models.IntegerField()
  chmode = models.IntegerField(choices=CHMODE_FLAG)
  class Meta:
    db_table = u'channel_akick'

class ForbiddenChannel(models.Model):
  channel = models.CharField(max_length=255, primary_key=True)
  class Meta:
    db_table = u'forbidden_channel'

class ChannelAccess(models.Model):
  channel = models.ForeignKey(Channel)
  account = models.ForeignKey('account.Account')
  group = models.ForeignKey('GroupServ.Group')
  level = models.IntegerField(choices=CHACCESS_LEVEL)
  class Meta:
    db_table = u'channel_access'
