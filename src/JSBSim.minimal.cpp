#include <FGFDMExec.h>

int main(int argc, char **argv)
{
  JSBSim::FGFDMExec FDMExec;

  FDMExec.LoadScript(SGPath(argv[1]));

  while (FDMExec.Run());
}

