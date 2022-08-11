## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2002 G.J. Paulissen
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

include $(top_srcdir)/lint.am

MAINTAINERCLEANFILES = Makefile.in

# AUTOMAKE_OPTIONS     = ansi2knr

AM_CPPFLAGS          = -I$(top_srcdir)/src/lib

AM_CFLAGS            = @DBUG_CPPFLAGS@

bin_PROGRAMS         = oradumper
oradumper_SOURCES    = main.c
oradumper_LDADD      = $(top_builddir)/src/lib/liboradumper.la @DBUG_LDADD@

check_DATA           =

if DARWIN

check_DATA          += oradumper.no_dynamic

endif

if DARWIN

oradumper.no_dynamic:
	ORACLE_LIBS=$(ORACLE_LIBS) ORACLE_LDFLAGS=$(ORACLE_LDFLAGS) $(top_srcdir)/darwin-remove-rpath.sh oradumper

endif

