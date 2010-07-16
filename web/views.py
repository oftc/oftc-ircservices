from web.util import render_to

@render_to('index.html')
def index(request):
  return { }
