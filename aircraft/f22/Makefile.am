EXTRA_DIST = \
	f22.xml \
	README

aircraft = f22

check-local:
	-xmllint --noout --schema $(top_srcdir)/JSBSim.xsd \
	--path $(top_srcdir)/aircraft/$(aircraft)  $(aircraft).xml
	$(top_builddir)/src/JSBSim \
	  --root=$(top_srcdir) \
	  --catalog=$(aircraft) \
	  1>/dev/null

