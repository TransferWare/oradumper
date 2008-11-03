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

AUTOMAKE_OPTIONS 	= 
SUBDIRS 		= replace src . test
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure config-h.in \
			  stamp-h.in $(AUX_DIST)
EXTRA_DIST              = lint.am

CLEANFILES              = `find . \( -name '*.gcov' -o -name '*.gcda' -o -name '*.gcno' -o -name '*.lis' \)`

lintdirs		= $(top_builddir)/src

lint:
	@echo Making $@ in $(lintdirs)
	@for d in $(lintdirs); do (cd $$d && make $@); done
