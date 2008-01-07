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

TESTS = test1

FORCE = force

DIFF = diff -w

# Ensure the created dummy test scripts are executable (chmod +x)

test1: $(FORCE)
	$(testprog) userid=$(USERID) sqlstmt='select 1 as C from dual union select 2 from dual' $(ORADUMPER_OPTIONS) 1>$@.log
	$(DIFF) $@.log $(srcdir)/$@.ref
	@echo : > $@ && chmod +x $@

force: ;
