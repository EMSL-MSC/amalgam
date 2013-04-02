bin_PROGRAMS = amalgamfs

INCLUDES = $(ARCHIVE_CFLAGS) $(BUHFS_CFLAGS) $(FUSE_CFLAGS) $(GLIB_CFLAGS) -DSVNVER=\"`$(top_srcdir)/svnver.sh $(top_srcdir)`\" -DSVNBRANCH=\"`$(top_srcdir)/svnbranch.sh $(top_srcdir)`\" -I$(top_srcdir)/src/libamalgam

amalgamfs_SOURCES = main.c amalgamfs.c amalgamfs.h
amalgamfs_LDFLAGS = $(ARCHIVE_LIBS) $(FUSE_LIBS) $(GLIB_LIBS) $(BUHFS_LIBS)
amalgamfs_LDADD = ../libamalgam/libamalgam.la
