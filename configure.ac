# -*-autoconf-*-
# Process this file with autoconf to produce a configure script.

# Copyright (C) 2008-2022 G.J. Paulissen
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

AC_REVISION([2022-11-07])
AC_INIT([oradumper],[1.4.0],[gert.jan.paulissen@gmail.com])

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
	test -d $srcdir/.mvn || mkdir $srcdir/.mvn
	echo "-Drevision=$PACKAGE_VERSION" > $srcdir/.mvn/maven.config
fi

# Use same configuration file as EPC
AC_CONFIG_MACRO_DIR([config])
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
  if gcc --version | grep -E "gcc \(GCC\) [1-3]\." 1>/dev/null 2>&1
  then
    # gcc (GCC) 1.x or 2.x or 3.x: use -mno-cygwin
    test "$CC" = "" && CC="gcc -mno-cygwin" && export CC
  fi
  ;;
*solaris*|*sun*)
  test "$LIBS" = "" && LIBS="-lm" && export LIBS
  ;;
?*)
  ;;
*)
  AC_MSG_ERROR([Host not set])
  ;;
esac

AM_INIT_AUTOMAKE
# AM_C_PROTOTYPES
AM_WITH_DMALLOC

## AM_PATH_CHECK deprecated
# m4_pattern_allow([^AM_PATH_CHECK$])
# AM_PATH_CHECK([0.10.0])
PKG_CHECK_MODULES([CHECK], [check >= 0.10.0])

# Checks for programs.
# AM_PROG_CC_C_O
AC_PROG_CC
AM_PROG_CC_STDC

# dbug package ?
ACX_WITH_DBUG

# Enable PRO*C
ACX_PROG_PROC

#
# The checks for LIBTOOL must be placed after ACX_PROG_PROC, since
# that macro changes LDFLAGS and LIBS which affects the libtool settings
# on AIX.
#
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

case ${host_os} in
*darwin*)
  case "${enable_shared}" in
	yes)
    AC_MSG_NOTICE([On Mac OS X you should disable shared libraries (configure --help)])
    enable_shared=no
    enable_static=yes
    ;;
	no)
    ;;
  esac
  darwin=true
  ;;
*)
  darwin=false
  ;;
esac

AM_CONDITIONAL([DARWIN], [test x$darwin = xtrue])

# Enable building an extproc routine

AC_ARG_ENABLE([extproc],
              AS_HELP_STRING([--enable-extproc],[turn on creating an Oracle extproc routine for the oradumper]),
							[case "${enableval}" in
							   yes) EXTPROC=true; ACX_PROG_OCI ;;
								 no)  EXTPROC=false ;;
								 *)   AC_MSG_ERROR(bad value ${enableval} for --enable-extproc) ;;
							 esac],
							[EXTPROC=false])

AM_CONDITIONAL([EXTPROC], [test x$EXTPROC = xtrue])

# Enable epc
ACX_EPC

# Checks for programs
AC_ARG_ENABLE([gcov],
							AS_HELP_STRING([--enable-gcov],[turn on profiling with gcov]),
							[case "${enableval}" in
							   yes) AC_CHECK_PROG([GCOV], [gcov], [true], [false]) ;;
								 no)  GCOV=false ;;
    						 *)   AC_MSG_ERROR(bad value ${enableval} for --enable-gcov) ;;
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

# Check library unistring
AC_ARG_ENABLE([unistring],
              AS_HELP_STRING([--enable-unistring],[turn on the GNU library unistring]),
							[case "${enableval}" in
							   yes) UNISTRING=true;;
								 no)  UNISTRING=false ;;
								 *)   AC_MSG_ERROR(bad value ${enableval} for --enable-unistring) ;;
							 esac],
							[UNISTRING=false])

AM_CONDITIONAL([UNISTRING], [test x$UNISTRING = xtrue])

if test "x$UNISTRING" = 'xtrue'
then
  AC_CHECK_LIB([unistring],[u8_check],[LIBS="-lunistring $LIBS"])
	AC_CHECK_HEADERS([unitypes.h unistr.h uniconv.h unistdio.h uniname.h unictype.h uniwidth.h uniwbrk.h unilbrk.h uninorm.h unicase.h])
  AH_BOTTOM([#define HAVE_UNISTRING 1])
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([assert.h check.h ctype.h dbug.h iconv.h langinfo.h locale.h malloc.h search.h sqlcpr.h stdbool.h stdlib.h string.h strings.h sys/errno.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Check for functions
AC_CHECK_FUNCS([strdup setenv])

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
