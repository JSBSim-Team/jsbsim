includedir = @includedir@/JSBSim/initialization

###AM_CPPFLAGS = -DOLD_LIBC -DAGO_DIRECTSEARCH -Wno-non-template-friend

noinst_LTLIBRARIES = libInit.la

libInit_la_SOURCES = FGInitialCondition.cpp FGTrim.cpp FGTrimAxis.cpp
###                       FGTrimAnalysis.cpp FGTrimAnalysisControl.cpp

include_HEADERS = FGInitialCondition.h FGTrim.h FGTrimAxis.h
###                       FGTrimAnalysis.h FGTrimAnalysisControl.h

INCLUDES = -I$(top_srcdir)/src -I../
