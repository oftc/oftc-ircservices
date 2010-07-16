from django.conf.urls.defaults import *

urlpatterns = patterns('web.ChanServ.views',
  (r'^$', 'index'),
  (r'^view/(?P<channel>\w+)/$', 'view_chan'),
)
