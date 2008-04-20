includedir = @includedir@/JSBSim/math/direct_search

AM_CPPFLAGS = -DOLD_LIBC -DAGO_DIRECTSEARCH

LIBRARY_SOURCES = CompassSearch.cc \
                             CoordinateSearch.cc \
                             DirectSearch.cc \
                             EdHJSearch.cc \
                             HJSearch.cc \
                             Hybrid_NMSearch.cc \
                             NLessSearch.cc \
                             NMSearch.cc \
                             PatternSearch.cc \
                             SHHSearch.cc \
                             SMDSearch.cc \
                             SimplexSearch.cc \
                             objective.cc

LIBRARY_INCLUDES = CompassSearch.h \
                 CoordinateSearch.h \
                 DirectSearch.h \
                 Dyn_alloc.h \
                 EdHJSearch.h \
                 HJSearch.h \
                 Hybrid_NMSearch.h \
                 NLessSearch.h \
                 NMSearch.h \
                 PatternSearch.h \
                 SHHSearch.h \
                 SMDSearch.h \
                 SimplexSearch.h \
                 cppmat.h \
                 vec.h \
                 objective.h

if BUILD_LIBRARIES
noinst_LTLIBRARIES = libdirect_search.la
include_HEADERS = $(LIBRARY_INCLUDES)
libdirect_search_la_SOURCES = $(LIBRARY_SOURCES)
libdirect_search_la_CXXFLAGS = $(AM_CXXFLAGS)
else
noinst_LIBRARIES = libdirect_search.a
noinst_HEADERS = $(LIBRARY_INCLUDES)
libdirect_search_a_SOURCES = $(LIBRARY_SOURCES)
endif

INCLUDES = -I$(top_srcdir)/src -I../../
