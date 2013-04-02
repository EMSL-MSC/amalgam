INCLUDES = $(ARCHIVE_CFLAGS) $(GLIB_CFLAGS) -DSVNVER=\"`$(top_srcdir)/svnver.sh $(top_srcdir)`\" -DSVNBRANCH=\"`$(top_srcdir)/svnbranch.sh $(top_srcdir)`\"

lib_LTLIBRARIES = libamalgam.la

libamalgam_la_SOURCES = tar.c tar.h
libamalgam_la_LDFLAGS = $(ARCHIVE_LIBS) $(GLIB_LIBS)

pkginclude_HEADERS = tar.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = amalgam.pc
