f#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Copyright (C) 2008 G.J. Paulissen
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AC_PREREQ(2.59)
AC_REVISION([$Revision: 1292 $])
AC_INIT([oradumper],[1.0.0],[g.paulissen@chello.nl])

# version check
if test -f "$srcdir/VERSION"
then
  read version < $srcdir/VERSION
else
  version=
fi

if test "$version" != "$PACKAGE_VERSION"
then
  echo "Updating file $srcdir/VERSION with package version $PACKAGE_VERSION." 1>&2
  echo "$PACKAGE_VERSION" > $srcdir/VERSION
fi

# Use same configuration file as EPC
AC_CONFIG_AUX_DIR([config])
AH_BOTTOM(
[#ifndef HAVE_STRDUP
extern /*@null@*/ /*@only@*/ char *strdup (char *s) /*@*/;
#endif]
)
AC_CONFIG_SRCDIR([src/prog/main.c])
AM_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST

case ${host} in
*-cygwin)
	test "$CC" = "" && CC="gcc -mno-cygwin" && export CC
	;;
?*)	;;
*)	AC_MSG_ERROR([Host not set])
	;;
esac

AM_INIT_AUTOMAKE
AM_C_PROTOTYPES
AM_WITH_DMALLOC
m4_pattern_allow([^AM_PATH_CHECK$])
AM_PATH_CHECK
# PKG_CHECK_MODULES([CHECK], [check >= 0.9.4)])

# Checks for programs.
# AM_PROG_CC_C_O
AC_PROG_CC
AM_PROG_CC_STDC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Enable dbug
ACX_ENABLE_DBUG

# Enable PRO*C
ACX_PROG_PROC

# Enable building an extproc routine

AC_ARG_ENABLE([extproc],
[  --enable-extproc        turn on creating an Oracle extproc routine for the oradumper],
[case "${enableval}" in
  yes) EXTPROC=true; ACX_PROG_OCI ;;
   no) EXTPROC=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-extproc) ;;
esac],
[EXTPROC=false])

AM_CONDITIONAL([EXTPROC], [test x$EXTPROC = xtrue])

# Enable epc
ACX_EPC

# Checks for programs
AC_ARG_ENABLE([gcov],
[  --enable-gcov           turn on profiling with gcov],
[case "${enableval}" in
  yes) AC_CHECK_PROG([GCOV], [gcov], [true], [false]) ;;
   no) GCOV=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-gcov) ;;
esac],
[GCOV=false])

AM_CONDITIONAL([GCOV], [test x$GCOV = xtrue])

if test "x$GCOV" = 'xtrue'
then
  AH_BOTTOM([#define HAVE_GCOV 1])
  AC_CHECK_LIB([gcov],
  [__gcov_init],
  [CFLAGS="-ftest-coverage -fprofile-arcs $CFLAGS -O0 -pg"; LIBS="-lgcov $LIBS"])
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([assert.h check.h ctype.h dbug.h locale.h malloc.h search.h sqlcpr.h stdbool.h stdlib.h string.h strings.h sys/errno.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Check for functions
AC_CHECK_FUNCS([strdup])

# Checks for library functions.
AC_CONFIG_LIBOBJ_DIR([replace])
AC_FUNC_MALLOC
AC_LIBOBJ([malloc])
AC_LIBOBJ([strdup])

# This is necessary so that .o files in LIBOBJS are also built via
# the ANSI2KNR-filtering rules.
LIB@&t@OBJS=`echo "$LIB@&t@OBJS" |
             sed 's,\.[[^.]]* ,$U&,g;s,\.[[^.]]*$,$U&,'`
LTLIBOBJS=`echo "$LIB@&t@OBJS" |
           sed 's,\.[[^.]]* ,.lo ,g;s,\.[[^.]]*$,.lo,'`
AC_SUBST(LTLIBOBJS)dnl
AC_SUBST(ac_aux_dir)dnl

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([replace/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/lib/Makefile])
AC_CONFIG_FILES([src/prog/Makefile])
AC_CONFIG_FILES([test/Makefile])

AC_OUTPUT

echo \
"------------------------------------------------------------------------
Configuration:

  Source code location:       ${srcdir}
  Compiler:                   ${CC}
  Compiler flags:             ${CFLAGS}
  Host System Type:           ${host}
  Install path:               ${prefix}

  See config.h for further configuration information.
------------------------------------------------------------------------"