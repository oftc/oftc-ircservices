from django.conf.urls.defaults import *

urlpatterns = patterns('web.NickServ.views',
  (r'^$', 'index'),
  (r'^access/list/$', 'access_list'),
  (r'^cert/list/$', 'cert_list'),
)
