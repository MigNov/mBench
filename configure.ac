AC_INIT([mbench], [0.0.1], [minovotn@redhat.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AM_PROG_CC_C_O
AC_CHECK_LIB([m], [sin], [], AC_MSG_ERROR([You need libm to compile this utility]))

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
