from django.contrib.auth.models import User, check_password
from django.contrib.auth.backends import ModelBackend
from web.account.models import Account
from web.NickServ.models import Nickname
import hashlib

class OFTCBackend(ModelBackend):
  def authenticate(self, username=None, password=None):
    nickname = Nickname.objects.get(nick=username)
    account = nickname.account

    m = hashlib.sha1()

    m.update(password)
    m.update(account.salt)

    if not account.password == m.hexdigest().upper():
      return None

    if account.user == None:
      user = User()
      user.username = username
      user.save()
      account.user = user
      account.save()

    return account.user
  def get_user(self, user_id):
    try:
      return User.objects.get(pk=user_id)
    except User.DoesNotExist:
      return None
