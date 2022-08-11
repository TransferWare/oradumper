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

MAINTAINERCLEANFILES     = Makefile.in
DISTCLEANFILES           = oradumper.h oradumper_int.h

# AUTOMAKE_OPTIONS         = $(top_builddir)/src/prog/ansi2knr

AM_CPPFLAGS              = -I$(srcdir)

AM_CFLAGS                = @DBUG_CPPFLAGS@

noinst_HEADERS           = oradumper_int.h
include_HEADERS          = oradumper.h

lib_LTLIBRARIES	         = liboradumper.la

liboradumper_la_SOURCES  = oradumper.pc oradumper.h oradumper_int.c oradumper_int.h
SOURCES                  = oradumper.c oradumper_int.c
MOSTLYCLEANFILES         = oradumper.c

if EXTPROC

liboradumper_la_SOURCES += oradumper_extproc.pc
SOURCES                 += oradumper_extproc.c
MOSTLYCLEANFILES        += oradumper_extproc.c

endif

# Do not forget the PRO*C files

EXTRA_DIST               = oradumper.pc oradumper_extproc.pc

##
## History of interface versions:
## - 0:0:0 start

liboradumper_la_LDFLAGS  = -no-undefined -version-info 0:0:0 @ORACLE_LDFLAGS@
liboradumper_la_LIBADD   = $(top_builddir)/replace/libreplace.la @DBUG_LIBADD@ @ORACLE_LIBS@

.pc.c:
	$(PROC) $(PROCINCLUDES) $(PROCFLAGS) DYNAMIC=ANSI MODE=ANSI INAME=$< ONAME=$@

LINTFLAGS += -f $(srcdir)/oradumper.splintrc
