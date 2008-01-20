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

MAINTAINERCLEANFILES = Makefile.in

prog                 = $(top_builddir)/src/prog/oradumper$(EXEEXT)
testlib              = $(top_builddir)/src/lib/liboradumper.la
testprog             = ./oradumper$(EXEEXT)
sqlstmt1             = "select 1234567890 as NR, 'mystring' as STR, to_date('1900-12-31 23:23:59') as DAY from dual\
                        union\
                        select 2345678901, 'ƒ Õ’ÿABCDE', to_date('20001231232359', 'yyyymmddhh24miss') from dual"
sqlstmt1_old         = "select 1234567890 as NR, 'mystring' as STR, to_date('1900-12-31 23:23:59') as DAY from dual\
                        union\
                        select 2345678901, 'YOURSTRING', to_date('20001231232359', 'yyyymmddhh24miss') from dual"
sqlstmt2             = "select to_clob(rpad('0123456789', 8000, '0123456789')) as myclob from dual"
sqlstmt3             = "select object_name, object_type from all_objects where owner = 'SYS' and object_type <> 'JAVA CLASS' and rownum <= 1000 order by object_name"
check_DATA           = $(testprog)

$(testprog): $(prog)
	@ln -s $< $@

$(prog): build.lib build.prog

build.lib:
	cd $(top_builddir)/src/lib && $(MAKE) all

build.prog:
	cd $(top_builddir)/src/prog && $(MAKE) all

##
## Test software
##

TESTS = test1 test2 test3

FORCE = force

DIFF = diff -w

# Ensure the created dummy test scripts are executable (chmod +x)

$(TESTS):: $(FORCE)

$(TESTS)::
	@-rm $@.log $@.err

test1::
	for nr in 1 2 3; do $(testprog) userid=$(USERID) sqlstmt=$(sqlstmt1) arraysize=$$nr $(ORADUMPER_OPTIONS) 1>>$@.log 2>>$@.err; done

test2::
	$(testprog) userid=$(USERID) sqlstmt=$(sqlstmt2) arraysize=2 $(ORADUMPER_OPTIONS) 1>$@.log 2>$@.err

test3::
	$(testprog) userid=$(USERID) sqlstmt=$(sqlstmt3) arraysize=1000 $(ORADUMPER_OPTIONS) 1>$@.log 2>$@.err

$(TESTS)::
	@$(DIFF) $@.log $(srcdir)/$@.log.ref
	@$(DIFF) $@.err $(srcdir)/$@.err.ref
	@echo : > $@ && chmod +x $@

force: ;
