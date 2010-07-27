from django.contrib.auth.models import User, check_password
from django.contrib.auth.backends import ModelBackend
from web.account.models import Account
from web.NickServ.models import Nickname
import hashlib

class OFTCBackend(ModelBackend):
  def authenticate(self, username=None, password=None):
    try:
      nickname = Nickname.objects.get(nick=username)
    except Nickname.DoesNotExist:
      return None

    account = nickname.account

    m = hashlib.sha1()

    m.update(password)
    m.update(account.salt)

    if not account.password.upper() == m.hexdigest().upper():
      return None

    if account.user == None:
      user = User()
      user.username = username
      if account.flag_admin:
        user.is_staff = True
      user.save()
      account.user = user
      account.save()
    else:
      account.user.is_staff = account.flag_admin
      account.user.save()

    return account.user
  def get_user(self, user_id):
    try:
      return User.objects.get(pk=user_id)
    except User.DoesNotExist:
      return None
