from django import template
from datetime import datetime
from django.template.defaultfilters import stringfilter

@stringfilter
def fromtimestamp(value, arg):
  date = datetime.fromtimestamp(int(value))

  return date.strftime(str(arg))

register = template.Library()

register.filter('fromts', fromtimestamp)
