## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2002-2007 G.J. Paulissen
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation; either
## version 2.1 of the License, or (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

MAINTAINERCLEANFILES  = Makefile.in

AUTOMAKE_OPTIONS      = $(top_builddir)/src/lib/ansi2knr

MOSTLYCLEANFILES      =

AM_CPPFLAGS           =

EXTRA_DIST            = malloc.c strdup.c

noinst_LTLIBRARIES    = libreplace.la

libreplace_la_SOURCES = 
libreplace_la_LIBADD  = @LTLIBOBJS@
libreplace_la_LDFLAGS =
