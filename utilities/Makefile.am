INCLUDES = \
	-Wall

bin_PROGRAMS = simplot

simplot_SOURCES = \
	datafile.cpp \
	datafile.h \
	main.cpp

simplot_CFLAGS = \
	-I/usr/local/dislin

simplot_LDADD = \
	-L/usr/local/dislin -ldisgcc -lgdi32 -lcomdlg32 \
	$(top_srcdir)/libJSBSim.a \
	$(top_srcdir)/filtersjb/libFCSComponents.a

EXTRA_DIST = \
	README
