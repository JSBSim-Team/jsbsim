AM_CPPFLAGS = -DOLD_LIBC -DAGO_DIRECTSEARCH

noinst_LIBRARIES = libdirect_search.a

libdirect_search_a_SOURCES = CompassSearch.cc \
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

noinst_HEADERS = CompassSearch.h \
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
