CC = g++
CCOPTS = -pg -O2
INCLUDES = -I../ -I../filtersjb
LINKDIR= -L../ -L../filtersjb/
LIBS= -lJSBSim -lFCSComponents -lm

c172_vti: c172_vti.cpp ../libJSBSim.a ../filtersjb/libFCSComponents.a
	$(CC) $(INCLUDES) $(LINKDIR) -o c172_vti c172_vti.cpp $(LIBS)

