bin_PROGRAMS = amalgaminfo

INCLUDES = $(ARCHIVE_CFLAGS) $(GLIB_CFLAGS) -DSVNVER=\"`$(top_srcdir)/svnver.sh $(top_srcdir)`\" -DSVNBRANCH=\"`$(top_srcdir)/svnbranch.sh $(top_srcdir)`\" -I$(top_srcdir)/src/libamalgam

amalgaminfo_SOURCES = amalgaminfo.c
amalgaminfo_LDFLAGS = $(ARCHIVE_LIBS) $(GLIB_LIBS)
amalgaminfo_LDADD = ../libamalgam/libamalgam.la
