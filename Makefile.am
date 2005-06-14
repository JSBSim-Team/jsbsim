EXTRA_DIST = \
	autogen.sh

SUBDIRS	= src aircraft engine scripts

dist-hook:
	(cd $(top_srcdir))

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

