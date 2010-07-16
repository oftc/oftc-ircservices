from django.conf.urls.defaults import *

# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()

urlpatterns = patterns('',
    (r'^auth/', include('registration.backends.default.urls')),
    (r'^NickServ/', include('web.NickServ.urls')),
    (r'^ChanServ/', include('web.ChanServ.urls')),
    (r'^$', 'web.views.index'),
)
