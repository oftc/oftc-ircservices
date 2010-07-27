from django.conf.urls.defaults import *

# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()

urlpatterns = patterns('',
    (r'^auth/', include('registration.urls')),
    (r'^NickServ/', include('web.NickServ.urls')),
    (r'^ChanServ/', include('web.ChanServ.urls')),
    (r'^OperServ/', include('web.OperServ.urls')),
    (r'^$', 'web.views.index'),
)
