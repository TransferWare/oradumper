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

MAINTAINERCLEANFILES    = Makefile.in
DISTCLEANFILES          = test_enclosure_string

check_PROGRAMS          = oradumper_check
check_SCRIPTS           = check_userid oradumper_test
oradumper_check_SOURCES = oradumper_check.c $(top_srcdir)/src/lib/oradumper.h $(top_srcdir)/src/lib/oradumper_int.h
AM_CPPFLAGS             = @CHECK_CFLAGS@
oradumper_check_LDADD   = $(top_builddir)/src/lib/liboradumper.la @CHECK_LIBS@ @DBUG_LDADD@

CLEANFILES              = *.lis *~ *.log usage.txt $(check_PROGRAMS) `find ${top_builddir}/src \( -name '*.gcda' -o -name '*.gcov' \) -delete`

if GCOV

GCOV_TESTS              = check_coverage

else

GCOV_TESTS              =

endif

check_SCRIPTS          +=  $(GCOV_TESTS)

DMALLOC_OPTIONS         = debug=0x4000503,log=dmalloc%p.log
AM_TESTS_ENVIRONMENT    = (cd ${srcdir}; for f in ${check_SCRIPTS}; do chmod +x $$f; done);
TESTS_ENVIRONMENT       = top_builddir=${top_builddir} top_srcdir=${top_srcdir} DMALLOC_OPTIONS=$(DMALLOC_OPTIONS)

TESTS                   = $(check_PROGRAMS) $(check_SCRIPTS)

EXTRA_DIST              = check_coverage check_userid oradumper_test oradumper_test.sql \
                          query0.lis.ref query1.lis.ref query2.lis.ref query3.lis.ref query4.lis.ref query5.lis.ref query6.lis.ref query7.lis.ref \
                          query_data_types.lis.ref usage.txt.ref
