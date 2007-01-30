#!/bin/sh
set -e
aclocal-1.9
autoheader
libtoolize --automake --ltdl
automake-1.9 --add-missing
autoconf
./configure $@
