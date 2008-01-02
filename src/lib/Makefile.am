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

MAINTAINERCLEANFILES    = Makefile.in

AUTOMAKE_OPTIONS        = $(top_builddir)/src/prog/ansi2knr

MOSTLYCLEANFILES        = oradumper.c

#AM_CPPFLAGS             = @DBUG_CPPFLAGS@ @EPC_CPPFLAGS@
AM_CPPFLAGS             = @DBUG_CPPFLAGS@

noinst_HEADERS          = oradumper.h oradumper_int.h

lib_LTLIBRARIES	        = liboradumper.la
liboradumper_la_SOURCES = oradumper.c oradumper_int.c


##
## History of interface versions:
## - 0:0:0 start

liboradumper_la_LDFLAGS = -no-undefined -version-info 0:0:0
#liboradumper_la_LIBADD  = $(top_builddir)/replace/libreplace.la @DBUG_LIBADD@ @EPC_LIBADD@
liboradumper_la_LIBADD  = $(top_builddir)/replace/libreplace.la @DBUG_LIBADD@

EXTRA_DIST              = 

.pc.c:
	$(PROC) $(PROCINCLUDES) $(PROCFLAGS) DYNAMIC=ANSI MODE=ANSI INAME=$< ONAME=$@
