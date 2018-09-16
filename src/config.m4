dnl $Id$
dnl config.m4 for extension yy_prof

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(yy_prof, for yy_prof support,
dnl Make sure that the comment is aligned:
dnl [  --with-yy_prof             Include yy_prof support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(yy_prof, whether to enable yy_prof support,
dnl Make sure that the comment is aligned:
[  --enable-yy_prof           Enable yy_prof support])

if test "$PHP_YY_PROF" != "no"; then
  dnl Write more examples of tests here...

  dnl # get library FOO build options from pkg-config output
  dnl AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  dnl AC_MSG_CHECKING(for libfoo)
  dnl if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists foo; then
  dnl   if $PKG_CONFIG foo --atleast-version 1.2.3; then
  dnl     LIBFOO_CFLAGS=`$PKG_CONFIG foo --cflags`
  dnl     LIBFOO_LIBDIR=`$PKG_CONFIG foo --libs`
  dnl     LIBFOO_VERSON=`$PKG_CONFIG foo --modversion`
  dnl     AC_MSG_RESULT(from pkgconfig: version $LIBFOO_VERSON)
  dnl   else
  dnl     AC_MSG_ERROR(system libfoo is too old: version 1.2.3 required)
  dnl   fi
  dnl else
  dnl   AC_MSG_ERROR(pkg-config not found)
  dnl fi
  dnl PHP_EVAL_LIBLINE($LIBFOO_LIBDIR, YY_PROF_SHARED_LIBADD)
  dnl PHP_EVAL_INCLINE($LIBFOO_CFLAGS)

  dnl # --with-yy_prof -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/yy_prof.h"  # you most likely want to change this
  dnl if test -r $PHP_YY_PROF/$SEARCH_FOR; then # path given as parameter
  dnl   YY_PROF_DIR=$PHP_YY_PROF
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for yy_prof files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       YY_PROF_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$YY_PROF_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the yy_prof distribution])
  dnl fi

  dnl # --with-yy_prof -> add include path
  dnl PHP_ADD_INCLUDE($YY_PROF_DIR/include)

  dnl # --with-yy_prof -> check for lib and symbol presence
  dnl LIBNAME=yy_prof # you may want to change this
  dnl LIBSYMBOL=yy_prof # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $YY_PROF_DIR/$PHP_LIBDIR, YY_PROF_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_YY_PROFLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong yy_prof lib version or lib not found])
  dnl ],[
  dnl   -L$YY_PROF_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(YY_PROF_SHARED_LIBADD)

  eval "LOCALSTATEDIR=$localstatedir"
  AC_DEFINE_UNQUOTED([LOCALSTATEDIR], ["${LOCALSTATEDIR}"], [define local state dir])

  CFLAGS="$CFLAGS -lcurl -lleveldb -lmdbm -Wall -D_GNU_SOURCE"

  YY_PROF_SOURCE_FILES="yy_time.c yy_log.c yy_api.c yy_stat.c yy_storage.c"
  YY_PROF_SOURCE_FILES="${YY_PROF_SOURCE_FILES} storage/mdbm.c storage/leveldb.c"
  YY_PROF_SOURCE_FILES="${YY_PROF_SOURCE_FILES} yy_adapter.c yy_prof.c"
  PHP_NEW_EXTENSION(yy_prof, $YY_PROF_SOURCE_FILES, $ext_shared)

  PHP_ADD_BUILD_DIR([$ext_builddir/storage])
fi
