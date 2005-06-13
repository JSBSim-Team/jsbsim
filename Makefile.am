EXTRA_DIST = \
	autogen.sh

SUBDIRS	= src

dist-hook:
	(cd $(top_srcdir); $(HOME)/Projects/FlightGear-0.9/admin/am2dsp.pl)

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

