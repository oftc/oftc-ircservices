from django.conf.urls.defaults import *

urlpatterns = patterns('web.OperServ.views',
  (r'^$', 'index'),
  (r'^jupe/list', 'jupe_list'),
  (r'^akill/list', 'akill_list'),
)
