from django import template
from datetime import datetime
from django.template.defaultfilters import stringfilter

@stringfilter
def fromtimestamp(value, arg='%Y-%m-%d %H:%M:%S'):
  if value == None:
    return 'Not set'
  date = datetime.fromtimestamp(int(value))

  return date.strftime(str(arg))

register = template.Library()

register.filter('fromts', fromtimestamp)
