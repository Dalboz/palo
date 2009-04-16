dnl =========================================================================================
dnl process this file with autoconf to produce a configure script
dnl =========================================================================================

AC_PREREQ(2.59)
AC_INIT(Jedox Palo Legacy Interface Tests,0.9.0,info@triagens.de)

dnl =========================================================================================
dnl auxillary directory for install-sh and missing
dnl =========================================================================================

AC_CONFIG_AUX_DIR(Config)
AC_CONFIG_MACRO_DIR(Config)

dnl =========================================================================================
dnl use automake to generate Makfile.in
dnl =========================================================================================

AM_INIT_AUTOMAKE([foreign subdir-objects])

dnl =========================================================================================
dnl checks for programs
dnl =========================================================================================

AC_PROG_CXX
AC_PROG_CC

AC_ARG_ENABLE(32bit,
  AS_HELP_STRING([--enable-32bit], [force 32bit compilation]),
  CXXFLAGS="$CXXFLAGS -m32"
  CFLAGS="$CFLAGS -m32"
  LDFLAGS="$LDFLAGS -m32"
)

dnl =========================================================================================
dnl checks for libpalo 1.0
dnl =========================================================================================

AC_LANG(C)

AC_ARG_WITH(palo1,
  AS_HELP_STRING([--with-palo1=DIR], [where palo 1.0 is located]),
  CXXFLAGS="$CXXFLAGS -I$withval/server -I$withval/libpalo"
  CFLAGS="$CFLAGS -I$withval/server -I$withval/libpalo"
  LDFLAGS="$LDFLAGS -L$withval/libpalo -lpalo"
)

tr_PALO1=no

AC_CHECK_LIB([palo], [libpalo_init], [tr_PALO1=yes], [AC_MSG_WARN(libpalo version 1.0 not found (try using --with-palo1))])

AM_CONDITIONAL(ENABLE_PALO1, test x$tr_PALO1 = xyes)

if test x$tr_PALO1 = xyes;  then

dnl =========================================================================================
dnl checks for header and compiler characteristics
dnl =========================================================================================

AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h)
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

AC_FUNC_STRTOD
AC_CHECK_FUNCS([setlocale strtol])

dnl =========================================================================================
dnl output makefile
dnl =========================================================================================

fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
