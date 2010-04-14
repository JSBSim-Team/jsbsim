EXTRA_DIST = \
	autogen.sh JSBSim.xsd JSBSim.xsl JSBSim.vcproj JSBSim.dox

SUBDIRS	= src systems aircraft engine scripts data_output data_plot check_cases

dist-hook:
	(cd $(top_srcdir))

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

