from django.db import models

class Jupe(models.Model):
    id = models.IntegerField(primary_key=True)
    name = models.CharField(max_length=255)
    reason = models.CharField(max_length=255)
    setter = models.ForeignKey('account.Account', db_column='setter')
    class Meta:
        db_table = u'jupes'

class Akill(models.Model):
    id = models.IntegerField(primary_key=True)
    mask = models.CharField(unique=True, max_length=255)
    reason = models.CharField(max_length=255)
    setter = models.ForeignKey('account.Account', db_column='setter')
    time = models.IntegerField()
    duration = models.IntegerField()
    class Meta:
        db_table = u'akill'

class SentMail(models.Model):
    id = models.IntegerField(primary_key=True)
    account = models.ForeignKey('account.Account')
    email = models.CharField(max_length=255)
    sent = models.IntegerField()
    class Meta:
        db_table = u'sent_mail'

