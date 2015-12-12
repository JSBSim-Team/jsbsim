EXTRA_DIST = \
	autogen.sh JSBSim.xsd JSBSim.xsl JSBSim.vcproj JSBSim.vcxproj JSBSim.dox JSBSimMainLoopFlow.dot JSBSim.sln JSBSimSystem.xsd JSBSimPlot.xsd

SUBDIRS	= src systems aircraft engine scripts data_output data_plot check_cases examples

dist-hook:
	(cd $(top_srcdir))

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

