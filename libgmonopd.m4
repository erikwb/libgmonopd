# Configure paths for libgmonopd

dnl Test for LIBGMONOPDLIB, and define LIBGMONOPDLIB_CFLAGS and LIBGMONOPDLIB_LIBS
dnl   to be used as follows:
dnl AM_PATH_LIBGMONOPDLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
AC_DEFUN(AM_PATH_LIBGMONOPDLIB,
[dnl 
dnl Get the cflags and libraries from the libgmonopdlib-config script
dnl
AC_ARG_WITH(LIBGMONOPDLIB-prefix,[  --with-libgmonopdlib-prefix=PREFIX
                          Prefix where libgmonopdlib is installed (optional)],
            libgmonopdlib_config_prefix="$withval", libgmonopdlib_config_prefix="")
AC_ARG_WITH(libgmonopdlib-exec-prefix,[  --with-libgmonopdlib-exec-prefix=PREFIX
                          Exec prefix where libgmonopdlib is installed (optional)],
            libgmonopdlib_config_exec_prefix="$withval", libgmonopdlib_config_exec_prefix="")
AC_ARG_ENABLE(libgmonopdlibtest, [  --disable-libgmonopdlibtest     Do not try to compile and run a test libgmonopdlib program],
		    , enable_libgmonopdlibtest=yes)

  if test x$libgmonopdlib_config_exec_prefix != x ; then
     libgmonopdlib_config_args="$libgmonopdlib_config_args --exec-prefix=$libgmonopdlib_config_exec_prefix"
     if test x${LIBGMONOPDLIB_CONFIG+set} != xset ; then
        LIBGMONOPDLIB_CONFIG=$libgmonopdlib_config_exec_prefix/bin/libgmonopdlib-config
     fi
  fi
  if test x$libgmonopdlib_config_prefix != x ; then
     libgmonopdlib_config_args="$libgmonopdlib_config_args --prefix=$libgmonopdlib_config_prefix"
     if test x${LIBGMONOPDLIB_CONFIG+set} != xset ; then
        LIBGMONOPDLIB_CONFIG=$libgmonopdlib_config_prefix/bin/libgmonopdlib-config
     fi
  fi

  AC_PATH_PROG(LIBGMONOPDLIB_CONFIG, libgmonopdlib-config, no)
  min_libgmonopdlib_version=ifelse([$1], ,0.1.1,$1)

  AC_MSG_CHECKING(for libgmonopdlib - version >= $min_libgmonopdlib_version)
  no_libgmonopdlib=""
  if test "$LIBGMONOPDLIB_CONFIG" = "no" ; then
    no_libgmonopdlib=yes
  else
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS

    LIBGMONOPDLIB_CFLAGS=`$LIBGMONOPDLIB_CONFIG $libgmonopdlib_config_args --cflags`
    LIBGMONOPDLIB_LIBS=`$LIBGMONOPDLIB_CONFIG $libgmonopdlib_config_args --libs`
    libgmonopdlib_config_major_version=`$LIBGMONOPDLIB_CONFIG $libgmonopdlib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    libgmonopdlib_config_minor_version=`$LIBGMONOPDLIB_CONFIG $libgmonopdlib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    libgmonopdlib_config_micro_version=`$LIBGMONOPDLIB_CONFIG $libgmonopdlib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  fi
  AC_SUBST(LIBGMONOPDLIB_CFLAGS)
  AC_SUBST(LIBGMONOPDLIB_LIBS)
])