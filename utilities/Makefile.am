INCLUDES = \
	-I/usr/local/dislin_2

bin_PROGRAMS = simplot

simplot_SOURCES = \
	datafile.cpp \
	datafile.h \
	plotXMLVisitor.cpp \
	plotXMLVisitor.h \
	main.cpp

simplot_CXXFLAGS = \
	-Wall

simplot_LDADD = \
	-L/usr/local/dislin_2 -ldisgcc_d -lgdi32 -lcomdlg32 \
	-L/usr/local/lib -lsgxml -lsgstructure

EXTRA_DIST = \
	README
