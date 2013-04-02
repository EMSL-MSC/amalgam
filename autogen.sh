#!/bin/sh
aclocal &&
autoheader &&
libtoolize -c &&
automake --add-missing --copy &&
autoconf --force &&
./configure $*
