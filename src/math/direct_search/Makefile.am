includedir = @includedir@/JSBSim/math/direct_search

AM_CPPFLAGS = -DOLD_LIBC -DAGO_DIRECTSEARCH

noinst_LTLIBRARIES = libdirect_search.la

libdirect_search_la_SOURCES = CompassSearch.cc \
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

include_HEADERS = CompassSearch.h \
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

INCLUDES = -I$(top_srcdir)/src -I../../
