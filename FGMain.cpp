#include "FGFDMExec.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

#include <iostream>
#include <ctime>

void main(int argc, char** argv)
{
	FGFDMExec* FDMExec;
	
//  struct timespec short_wait = {0,100000000};
//  struct timespec no_wait    = {0,100000000};

  if (argc != 3) {
    cout << endl
         << "  You must enter the name of a registered aircraft and reset point:"
         << endl << endl << "  FDM <aircraft name> <reset file>" << endl;
    exit(0);
  }

  FDMExec = new FGFDMExec();

  FDMExec->GetAircraft()->LoadAircraft("aircraft", "engine", string(argv[1]));
  FDMExec->GetState()->Reset("aircraft", string(argv[2]));

  while (FDMExec->GetState()->Getsim_time() <= 25.0)
  {
//
// fake an aileron, rudder and elevator kick here after 20 seconds
//

		if (FDMExec->GetState()->Getsim_time() > 5.0) {
			FDMExec->GetFCS()->SetDe(0.05);
//			FDMExec->GetFCS()->SetDr(0.05);
//			FDMExec->GetFCS()->SetDa(0.05);
		}
		
    FDMExec->Run();
//    nanosleep(&short_wait,&no_wait);
  }

  delete FDMExec;
}
