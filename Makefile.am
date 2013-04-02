SUBDIRS = src doc

dist-hook:
	$(top_srcdir)/svnver.sh $(top_srcdir) > $(distdir)/svnver.txt
	$(top_srcdir)/svnbranch.sh $(top_srcdir) > $(distdir)/svnbranch.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir);
	$(top_srcdir)/svnver.sh $(top_srcdir) > $(DESTDIR)$(docdir)/svnver.txt
	$(top_srcdir)/svnbranch.sh $(top_srcdir) > $(DESTDIR)$(docdir)/svnbranch.txt

EXTRA_DIST = amalgam.spec svnver.sh svnbranch.sh COPYING.LIB COPYING

DEV=1
REVISION=1

rpm: dist
	mkdir -p packages
	rm -rf packages
	mkdir -p packages/bin
	mkdir -p packages/src
	if [ "x${DEV}" != "x1" ]; \
	then \
		rpmbuild -ta @PACKAGE_NAME@-@VERSION@.tar.gz --define '_rpmdir '`pwd`'/packages/bin' --define '_srcrpmdir '`pwd`'/packages/src' --define "LOCALREVISION ${REVISION}"; \
	else \
		rpmbuild -ta @PACKAGE_NAME@-@VERSION@.tar.gz --define '_rpmdir '`pwd`'/packages/bin' --define '_srcrpmdir '`pwd`'/packages/src' --define "LOCALREVISION `./svnver.sh $(top_srcdir)`"; \
	fi

rpms: rpm

MOCKOPTS=
MOCKDIST=fedora-18-x86_64
MOCK=/usr/bin/mock

mock: dist
	rm -rf packages
	mkdir -p packages/"$(MOCKDIST)"/srpms
	mkdir -p packages/"$(MOCKDIST)"/bin
	$(MOCK) -r "$(MOCKDIST)" --buildsrpm --spec amalgam.spec $(MOCKOPTS) --sources "`pwd`"
	mv "/var/lib/mock/$(MOCKDIST)/result/"*.src.rpm packages/"$(MOCKDIST)"/srpms/
	$(MOCK) -r "$(MOCKDIST)" --result "$(CURDIR)"/packages/"$(MOCKDIST)"/bin $(MOCKOPTS) "$(CURDIR)"/packages/"$(MOCKDIST)"/srpms/*.src.rpm
