dnl =========================================================================================
dnl process this file with autoconf to produce a configure script
dnl =========================================================================================

AC_PREREQ(2.60)
AC_INIT(Jedox Palo,2.5.0,info@triagens.de)

dnl =========================================================================================
dnl auxillary directory for install-sh and missing
dnl =========================================================================================

AC_CONFIG_AUX_DIR(Config)
AC_CONFIG_MACRO_DIR(Config)

dnl =========================================================================================
dnl checks build system
dnl =========================================================================================

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

dnl =========================================================================================
dnl use automake to generate Makfile.in
dnl =========================================================================================

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(Config/config.h)

dnl =========================================================================================
dnl checks for programs
dnl =========================================================================================

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_PROG_LEX
AM_CONDITIONAL(ENABLE_FLEX, test x$LEX = xflex)

AC_CHECK_PROG(BISON, bison, bison)
AM_CONDITIONAL(ENABLE_BISON, test x$BISON = xbison)

dnl =========================================================================================
dnl option for 32bit/64bit compile
dnl =========================================================================================

AC_ARG_ENABLE(32bit,
  AS_HELP_STRING([--enable-32bit], [force 32bit compilation]),
  if test "$enableval" == "yes";  then
    CXXFLAGS="$CXXFLAGS -m32"
    CFLAGS="$CFLAGS -m32"
    LDFLAGS="$LDFLAGS -m32"
  fi
)

AC_ARG_ENABLE(profile,
  AS_HELP_STRING([--enable-profile], [add -pg for profiling]),
  if test "$enableval" == "yes";  then
    CXXFLAGS="$CXXFLAGS -pg"
    CFLAGS="$CFLAGS -pg"
    LDFLAGS="$LDFLAGS -pg"
  fi
)

dnl =========================================================================================
dnl option for enabling the legacy interface as builtin or shared library
dnl =========================================================================================

AC_ARG_ENABLE(legacy,
  AC_HELP_STRING([--enable-legacy], [enable building of legacy interface]),
  if test "$enableval" = "yes";  then
    tr_legacy="yes";
  elif test "$enableval" == "module";  then
    if test "$enable_shared" = "yes";  then
      tr_legacy="module";
    else
      [AC_MSG_ERROR([module option requires --enable-shared])]
    fi
  elif test "$enableval" == "no";  then
    tr_legacy="no";
  else
    [AC_MSG_ERROR([unknown option $enableval, please use 'yes', 'no', or 'module'])]
  fi,
  tr_legacy="yes"
)

AM_CONDITIONAL(ENABLE_LEGACY, test "$tr_legacy" = "yes" -o "$tr_legacy" = "module")
AM_CONDITIONAL(ENABLE_LEGACY_MODULE, test "$tr_legacy" = "module")

if test "$tr_legacy" = "yes";  then
  AC_DEFINE_UNQUOTED(ENABLE_LEGACY, 1, [enables legacy interface])
elif test "$tr_legacy" = "module";  then
  AC_DEFINE_UNQUOTED(ENABLE_LEGACY, 1, [enables legacy interface])
  AC_DEFINE_UNQUOTED(ENABLE_LEGACY_MODULE, 1, [enables legacy interface as shared library])
fi

dnl =========================================================================================
dnl option for enabling the http interface as builtin or shared library
dnl =========================================================================================

AC_ARG_ENABLE(http,
  AC_HELP_STRING([--enable-http], [enable building of http interface]),
  if test "$enableval" = "yes";  then
    tr_http="yes";
  elif test "$enableval" == "module";  then
    if test "$enable_shared" = "yes";  then
      tr_http="module";
    else
      [AC_MSG_ERROR([module option requires --enable-shared])]
    fi
  elif test "$enableval" == "no";  then
    tr_http="no";
  else
    [AC_MSG_ERROR([unknown option $enableval, please use 'yes', 'no', or 'module'])]
  fi,
  tr_http="yes"
)

AM_CONDITIONAL(ENABLE_HTTP, test "$tr_http" = "yes" -o "$tr_http" = "module")
AM_CONDITIONAL(ENABLE_HTTP_MODULE, test "$tr_http" = "module")

if test "$tr_http" = "yes";  then
  AC_DEFINE_UNQUOTED(ENABLE_HTTP, 1, [enables http interface])
elif test "$tr_http" = "module";  then
  AC_DEFINE_UNQUOTED(ENABLE_HTTP, 1, [enables http interface])
  AC_DEFINE_UNQUOTED(ENABLE_HTTP_MODULE, 1, [enables http interface as shared library])
fi

dnl =========================================================================================
dnl option for enabling the https interface as builtin or shared library
dnl =========================================================================================

AC_ARG_ENABLE(https,
  AC_HELP_STRING([--enable-https], [enable building of https interface]),
  if test "$enableval" = "yes";  then
    tr_https="yes";
  elif test "$enableval" == "module";  then
    if test "$enable_shared" = "yes";  then
      tr_https="module";
    else
      [AC_MSG_ERROR([module option requires --enable-shared])]
    fi
  elif test "$enableval" == "no";  then
    tr_https="no";
  else
    [AC_MSG_ERROR([unknown option $enableval, please use 'yes', 'no', or 'module'])]
  fi,
  if test "$enable_shared" = "yes";  then
    tr_https="module"
  else
    tr_https="no"
  fi
)

AM_CONDITIONAL(ENABLE_HTTPS, test "$tr_https" = "yes" -o "$tr_https" = "module")
AM_CONDITIONAL(ENABLE_HTTPS_MODULE, test "$tr_https" = "module")

if test "$tr_https" = "yes" -o "$tr_https" = "module"; then
  if test "$tr_http" = "no"; then
    AC_MSG_ERROR([https requires http]);
  fi
fi

if test "$tr_https" = "yes";  then
  AC_DEFINE_UNQUOTED(ENABLE_HTTPS, 1, [enables https interface])
elif test "$tr_https" = "module";  then
  AC_DEFINE_UNQUOTED(ENABLE_HTTPS, 1, [enables https interface])
  AC_DEFINE_UNQUOTED(ENABLE_HTTPS_MODULE, 1, [enables https interface as shared library])
fi

dnl =========================================================================================
dnl enable trace
dnl =========================================================================================

AC_ARG_ENABLE(trace-option,
  AS_HELP_STRING([--enable-trace-option], [enables support for option --trace]),
  tr_trace_option="$enableval",
  tr_trace_option="no"
)

AC_MSG_CHECKING(trace option enabled)

AM_CONDITIONAL(ENABLE_TRACE_OPTION, test "$tr_trace_option" = "yes")

if test $tr_trace_option = "yes";  then
  AC_DEFINE_UNQUOTED(ENABLE_TRACE_OPTION, 1, [enables support for option --trace])
fi

AC_MSG_RESULT($tr_trace_option)

dnl =========================================================================================
dnl checks for system characteristics
dnl =========================================================================================

AC_C_BIGENDIAN

dnl FIXIT test for DIR_SEPARATOR_CHAR missing

dir_separator_char="/"

AC_DEFINE_UNQUOTED(DIR_SEPARATOR_CHAR, '$dir_separator_char', Path separator character)
AC_DEFINE_UNQUOTED(DIR_SEPARATOR_STR, "$dir_separator_char", Path separator string)

AC_SUBST(DIR_SEPARATOR_CHAR)
AC_SUBST(DIR_SEPARATOR_STR)

dnl =========================================================================================
dnl checks for header files
dnl =========================================================================================

AC_HEADER_STDC
AC_CHECK_HEADERS(signal.h string.h memory.h stdlib.h poll.h sys/time.h regex.h)

dnl =========================================================================================
dnl checks for typedefs, structures, and compiler characteristics
dnl =========================================================================================

AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES(suseconds_t)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(void*)
AC_CHECK_ALIGNOF(double)
AC_CHECK_ALIGNOF(void*)

AC_LANG(C++)
AC_MSG_CHECKING(overloading functions for size_t and uint32_t/uint64_t)
AC_TRY_COMPILE([#include <stdint.h>
                #include <stddef.h>
                void emil (size_t a) {}
                void emil (uint32_t a) {}
                void emil (uint64_t a) {}],
               [],
               tri_overload_funcs=yes,
               tri_overload_funcs=no)
AC_MSG_RESULT($tri_overload_funcs)

if test $tri_overload_funcs = "yes";  then
  AC_DEFINE_UNQUOTED(OVERLOAD_FUNCS_SIZE_T, 1, overloading functions for size_t and uint32_t/uint64_t is possible)
fi

AC_SUBST(OVERLOAD_FUNCS_SIZE_T)

AC_MSG_CHECKING(overloading functions for size_t and long int)
AC_TRY_COMPILE([#include <stdint.h>
                #include <stddef.h>
                void emil (size_t a) {}
                void emil (long int a) {}],
               [emil(1)],
               tri_overload_funcs_long=yes,
               tri_overload_funcs_long=no)
AC_MSG_RESULT($tri_overload_funcs_long)

if test $tri_overload_funcs_long = "yes";  then
  AC_DEFINE_UNQUOTED(OVERLOAD_FUNCS_SIZE_T_LONG, 1, overloading functions for size_t and long int is possible)
fi

AC_SUBST(OVERLOAD_FUNCS_SIZE_T_LONG)

dnl =========================================================================================
dnl check whether CC expects -export-dynamic
dnl =========================================================================================

AC_CACHE_CHECK(whether ${CC-cc} accepts -export-dynamic, gp_cv_prog_cc_export_dynamic,
  [echo 'int main(){}' > conftest.c
   if test -z "`${CC-cc} -export-dynamic -o conftest conftest.c 2>&1`"; then
     tr_cv_prog_cc_export_dynamic=yes
   else
     tr_cv_prog_cc_export_dynamic=no
   fi
   rm -f conftest*
])

AC_SEARCH_LIBS(dlopen, dl, AC_DEFINE(HAVE_DLOPEN,1,[have dlopen library function]))

if test "$ac_cv_search_dlopen" != "no";  then
  if test "$tr_cv_prog_cc_export_dynamic" = yes;  then
    LDFLAGS="$LDFLAGS -export-dynamic"
  fi
fi

dnl =========================================================================================
dnl checks for libraries
dnl =========================================================================================

AC_SEARCH_LIBS(socket, [socket])
AC_SEARCH_LIBS(gethostbyname, [nsl])

dnl =========================================================================================
dnl enable profiler
dnl =========================================================================================

AC_CHECK_FUNCS(getrusage)

AC_ARG_ENABLE(time-profiler,
  AS_HELP_STRING([--enable-time-profiler], [enables time profiling support]),
  tr_time_profiler="$enableval",
  tr_time_profiler="no"
)

AC_MSG_CHECKING(time profiling enabled)

AM_CONDITIONAL(ENABLE_TIME_PROFILER, test "$tr_time_profiler" = "yes")

if test "$tr_time_profiler" = "yes";  then
  AC_DEFINE_UNQUOTED(ENABLE_TIME_PROFILER, 1, [enables time profiling support])
fi

AC_MSG_RESULT($tr_time_profiler)

dnl =========================================================================================
dnl checks for dot
dnl =========================================================================================

AC_PATH_PROG(DOT, dot, [], [$PATH])

if test -z "$DOT";  then
  DOT_PATH=/cygdrive/c/Programme/ATT/Graphviz/bin
  AC_PATH_PROG(DOT, dot, [], [$DOT_PATH])
fi

if test -n "$DOT";  then
  HAVE_DOT="YES"

  if test -n "$DOT_PATH";  then
    DOT_PATH=$PATH_SEPARATOR$DOT_PATH
  fi
else
  HAVE_DOT="NO"
  PATH_PATH=""
fi

AC_SUBST(HAVE_DOT)
AC_SUBST(DOT_PATH)

dnl =========================================================================================
dnl check for libpalo 1.0
dnl =========================================================================================

AC_LANG(C)

save_CFLAGS=$CFLAGS
save_LDFLAGS=$LDFLAGS

AC_ARG_WITH(palo1,
  AS_HELP_STRING([--with-palo1=DIR], [where palo 1.0 is located]),
  CFLAGS="$CFLAGS -I$withval/server -I$withval/libpalo"
  LDFLAGS="$LDFLAGS -L$withval/libpalo -lpalo"
)

AC_CHECK_LIB([palo], [libpalo_init], [tr_PALO1="yes"], [tr_PALO1="no"])
AM_CONDITIONAL(ENABLE_PALO1, test "$tr_PALO1" = "yes")

CFLAGS=$save_CFLAGS
LDFLAGS=$save_LDFLAGS

dnl =========================================================================================
dnl check for OpenSSL
dnl =========================================================================================

OPENSSL_LIBS=""

if test "$tr_https" = "yes" -o "$tr_https" = "module";  then
  AC_ARG_WITH(openssl-inc,
    AS_HELP_STRING([--with-openssl-inc=DIR], [OpenSSL include files]),
    CFLAGS="$CFLAGS -I$withval"
  )

  AC_CHECK_HEADERS([openssl/ssl.h])

  AC_ARG_WITH(openssl-lib,
    AS_HELP_STRING([--with-openssl-lib=DIR], [OpenSSL library files]),
    LDFLAGS="$LDFLAGS -L$withval"
  )

  AC_CHECK_LIB([ssl], [SSL_get_error],

  if test "$tr_https" = "module"; then
    OPENSSL_LIBS="-lssl"
  else
    LIBS="-lssl $LIBS"
  fi)
fi

AC_SUBST(OPENSSL_LIBS)

dnl =========================================================================================
dnl output makefile
dnl =========================================================================================

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([Programs/Makefile])
AC_CONFIG_FILES([UnitTests/Makefile])
AC_CONFIG_FILES([UnitTests/Collections/Makefile])
AC_CONFIG_FILES([UnitTests/InputOutput/Makefile])
AC_CONFIG_FILES([UnitTests/HttpClient/Makefile])
AC_CONFIG_FILES([UnitTests/Palo/Makefile])
AC_CONFIG_FILES([Doxygen/palo.doxy])

dnl AC_CONFIG_SUBDIRS(UnitTests/LegacyInterface)

AC_OUTPUT

if test "$LEX" != flex; then
  AC_MSG_WARN(need a version of flex - changes to the .ll files will have no effect)
fi

if test "$BISON" != bison; then
  AC_MSG_WARN(need a version of bison - changes to the .yy files will have no effect)
fi

if test "$tr_legacy" = "yes";  then
  AC_MSG_NOTICE(legacy interface is compiled as built-in)
elif test "$tr_legacy" = "no"; then
  AC_MSG_NOTICE(legacy interface is disabled)
elif test "$tr_legacy" = "module"; then
  AC_MSG_NOTICE(legacy interface is compiled as shared library)
fi

if test "$tr_http" = "yes";  then
  AC_MSG_NOTICE(http interface is compiled as built-in)
elif test "$tr_http" = "no"; then
  AC_MSG_NOTICE(http interface is disabled)
elif test "$tr_http" = "module"; then
  AC_MSG_NOTICE(http interface is compiled as shared library)
fi

if test "$tr_https" = "yes";  then
  AC_MSG_NOTICE(https interface is compiled as built-in)
elif test "$tr_https" = "no"; then
  AC_MSG_NOTICE(https interface is disabled)
elif test "$tr_https" = "module"; then
  AC_MSG_NOTICE(https interface is compiled as shared library)
fi
