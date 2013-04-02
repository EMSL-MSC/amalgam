AC_INIT([amalgam],[0.1.0],[Kevin.Fox@pnl.gov])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_SRCDIR([configure.ac])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
#AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_ARG_ENABLE([dist-only],
        AS_HELP_STRING([--enable-dist-only],[Don't check requirements. Only for building releases.]),
        [
                do_dist_only=yes;
		export BUHFS_CFLAGS="-I.";
		export BUHFS_LIBS="-L.";
		export FUSE_CFLAGS="-I.";
		export FUSE_LIBS="-L.";
		export GLIB_CFLAGS="-I.";
		export GLIB_LIBS="-L.";
		export ARCHIVE_CFLAGS="-I.";
		export ARCHIVE_LIBS="-L.";
	],do_dist_only=no)
AM_CONDITIONAL(DO_DIST_ONLY, test x"$do_dist_only" = "xyes")

PKG_CHECK_MODULES([FUSE],[fuse])
PKG_CHECK_MODULES([GLIB],[glib-2.0, gthread-2.0])
PKG_CHECK_MODULES([ARCHIVE],[libarchive])
PKG_CHECK_MODULES([BUHFS],[buhfsutil])

AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug],[Add Debug Code]),
        [
CFLAGS=`echo ${CFLAGS} | sed 's/-O[[0-9]]//'`;
CFLAGS+=" -O0"],
)

AC_ARG_ENABLE([coverage],
        AS_HELP_STRING([--enable-coverage],[Add coverage code]),
        [
CFLAGS+=" --coverage"],
)

CFLAGS+=" -DEBUG"

docdir='${prefix}/share/doc/amalgam'
AC_SUBST(docdir)

AC_CHECK_HEADERS(attr/xattr.h, [], [echo; echo "You need the libattr development package."; test x"$do_dist_only" = "xyes" || exit -1;])

PACKAGE_NAME=AC_PACKAGE_NAME
AC_SUBST(PACKAGE_NAME)

AC_OUTPUT([
		Makefile
		doc/Makefile
		src/Makefile
		src/libamalgam/Makefile
		src/libamalgam/amalgam.pc
		src/amalgamfs/Makefile
		src/amalgambuild/Makefile
		src/amalgaminfo/Makefile
		amalgam.spec
])
