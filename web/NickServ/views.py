from web.util import render_to
from web.account.models import *
from datetime import datetime

@render_to('NickServ/index.html')
def index(request):
  account = Account.objects.get(user=request.user)
  if account.last_quit_time == None:
    quittime = None
  else:
    quittime = datetime.fromtimestamp(account.last_quit_time)

  return {
    'account': account,
    'reg_time': datetime.fromtimestamp(account.reg_time),
    'quit_time': quittime,
  }

@render_to('NickServ/access_list.html')
def access_list(request):
  list = AccountAccess.objects.filter(account__user=request.user)
  return {
    'list': list,
  }

@render_to('NickServ/cert_list.html')
def cert_list(request):
  list = AccountFingerprint.objects.filter(account__user=request.user)
  return {
    'list': list,
  }
