from django.http import HttpResponseRedirect
from web.util import render_to
from web.account.models import Account
from web.OperServ.models import *
from datetime import datetime

@render_to('OperServ/index.html')
def index(request):
  account = Account.objects.get(user=request.user)
  
  if not account.flag_admin:
    return HttpResponseRedirect('/')

  return { }

@render_to('OperServ/jupe_list.html')
def jupe_list(request):
  account = Account.objects.get(user=request.user)
  
  if not account.flag_admin:
    return HttpResponseRedirect('/')
 
  return { 
    'list': Jupe.objects.all(),
  }
  
@render_to('OperServ/akill_list.html')
def akill_list(request):
  account = Account.objects.get(user=request.user)
  
  if not account.flag_admin:
    return HttpResponseRedirect('/')
 
  list = Akill.objects.all()

  return {
    'list': list,
  }
