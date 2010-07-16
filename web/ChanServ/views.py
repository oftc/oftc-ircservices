from web.util import render_to
from web.account.models import Account
from web.ChanServ.models import *
from datetime import datetime

@render_to('ChanServ/index.html')
def index(request):
  account = Account.objects.get(user=request.user)
  channels = ChannelAccess.objects.filter(account=account)

  return { 'channels': channels, }

@render_to('ChanServ/view_chan.html')
def view_chan(request, channel):
  chan = '#' + channel
  dbchan = Channel.objects.get(channel=chan)
  
  return {
    'channel': dbchan,
    'reg_time': datetime.fromtimestamp(dbchan.reg_time),
  }
  
@render_to('ChanServ/access_list.html')
def access_list(request, channel):
  chan = '#' + channel
  list = ChannelAccess.objects.filter(channel__channel=chan)

  return {
    'list': list,
  }
