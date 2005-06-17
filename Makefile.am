EXTRA_DIST = \
	autogen.sh aircraft engine scripts

SUBDIRS	= src

dist-hook:
	(cd $(top_srcdir))

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

