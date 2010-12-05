EXTRA_DIST = \
        f104.xml \
	INSTALL

SUBDIRS = \
	Systems

aircraft = f104

check-local:
	-xmllint --noout --schema $(top_builddir)/JSBSim.xsd $(aircraft).xml
	$(top_builddir)/src/JSBsim \
	  --root=$(top_srcdir) \
	  --catalog=$(aircraft) \
	  1>/dev/null

