INCLUDES = \
	-I/usr/local/dislin_2

bin_PROGRAMS = simplot

simplot_SOURCES = \
	datafile.cpp \
	datafile.h \
	main.cpp

simplot_CXXFLAGS = \
	-Wall

simplot_LDADD = \
	-L/usr/local/dislin_2 -ldisgcc_d -lgdi32 -lcomdlg32 \
	$(top_srcdir)/libJSBSim.a \
	$(top_srcdir)/filtersjb/libFCSComponents.a

EXTRA_DIST = \
	README
