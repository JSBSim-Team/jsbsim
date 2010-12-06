EXTRA_DIST = \
	BLC.xml \
	FCS-pitch.xml \
	FCS-roll.xml \
	FCS-yaw.xml \
	NWS.xml \
	armament.xml \
	beacon.xml \
	canopy.xml \
	chute.xml \
	crash-detect.xml \
	flaps.xml \
	gear.xml \
	radar.xml \
	speedbrakes.xml \
	tailhook.xml

aircraft = f104

check-local:
	-xmllint --noout --schema $(top_srcdir)/JSBSimSystem.xsd \
	  --path $(top_srcdir)/aircraft/$(aircraft)/Systems $(EXTRA_DIST)
