bin_PROGRAMS = amalgambuild

INCLUDES = $(ARCHIVE_CFLAGS) $(GLIB_CFLAGS) -DSVNVER=\"`$(top_srcdir)/svnver.sh $(top_srcdir)`\" -DSVNBRANCH=\"`$(top_srcdir)/svnbranch.sh $(top_srcdir)`\" -I$(top_srcdir)/src/libamalgam

amalgambuild_SOURCES = amalgambuild.c
amalgambuild_LDFLAGS = $(ARCHIVE_LIBS) $(GLIB_LIBS)
amalgambuild_LDADD = ../libamalgam/libamalgam.la
