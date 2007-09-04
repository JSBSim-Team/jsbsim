AM_CPPFLAGS = -DOLD_LIBC -DAGO_DIRECTSEARCH -Wno-non-template-friend

noinst_LIBRARIES = libInit.a

libInit_a_SOURCES = FGInitialCondition.cpp FGTrim.cpp FGTrimAxis.cpp \
                       FGTrimAnalysis.cpp FGTrimAnalysisControl.cpp

noinst_HEADERS = FGInitialCondition.h FGTrim.h FGTrimAxis.h \
                       FGTrimAnalysis.h FGTrimAnalysisControl.h

INCLUDES = -I$(top_srcdir)/src -I../
