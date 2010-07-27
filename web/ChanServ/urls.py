from django.conf.urls.defaults import *

CHAN_REGEX = r'(?P<channel>[\w\-\[\]{}_]+)'

urlpatterns = patterns('web.ChanServ.views',
  (r'^$', 'index'),
  (r'^view/'+CHAN_REGEX+'/?$', 'view_chan'),
  (r'^access/list/'+CHAN_REGEX+'/?$', 'access_list'),
  (r'^akick/list/'+CHAN_REGEX+'/?$', 'akick_list'),
)
