#!/bin/sh
# BUILD.sh - preconfigure libapreq

libtoolize --automake -c -f
aclocal
autoconf
automake -a -c
