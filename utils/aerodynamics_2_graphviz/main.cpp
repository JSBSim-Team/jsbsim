using namespace std;

#include "aerodynamics_2_graphviz.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

SGPath RootDir;
SGPath ScriptName;
string AircraftName;
string outputfile;
string imagepath;
bool show_table_png;
std::shared_ptr<JSBSim::FGFDMExec> FDMExec;
aerodynamics_2_graphviz* viz;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


void PrintHelp(void)
{
  cout << endl << "  aerodynamics_2_graphviz version " << viz->GetVersion() << endl << endl;
  cout << endl << "  JSBSim version " << FDMExec->GetVersion() << endl << endl;
  cout << "  Usage: aerodynamics_2_graphviz [script file name] [output file names] <options>" << endl << endl;
  cout << "  options:" << endl;
  cout << "    --help  returns this message" << endl;
  cout << "    --outputfile=<filename>  sets (overrides) the name of the output file" << endl;
  cout << "    --root=<path>  specifies the JSBSim root directory (where aircraft/, engine/, etc. reside)" << endl;
  cout << "    --aircraft=<filename>  specifies the name of the aircraft to be modeled" << endl;
  cout << "    --script=<filename>  specifies a script to run" << endl;
  cout << "    --show_table_png=on/off  specifies whether show table png" << endl;
  cout << "    --imagepath=<path>  specifies table png root directory" << endl;

  cout << "  NOTE: There can be no spaces around the = sign when" << endl;
  cout << "        an option is followed by a filename" << endl << endl;
}

#define gripe cerr << "Option '" << keyword     \
    << "' requires a value, as in '"    \
    << keyword << "=something'" << endl << endl;/**/

bool options(int count, char** arg)
{
  int i;
  bool result = true;

  if (count == 1) {
    PrintHelp();
    exit(0);
  }

  RootDir = SGPath::fromEnv("JSBSim_ROOT");

  cout.setf(ios_base::fixed);

  for (i = 1; i < count; i++) {
    string argument = string(arg[i]);
    string keyword(argument);
    string value("");
    string::size_type n = argument.find("=");

    if (n != string::npos && n > 0) {
      keyword = argument.substr(0, n);
      value = argument.substr(n + 1);
    }

    if (keyword == "--help") {
      PrintHelp();
      exit(0);
    }
    else if (keyword == "--outputfile") {
      if (n != string::npos) {
        outputfile = value.c_str();
      }
      else {
        gripe;
        exit(1);
      }
    }
    else if (keyword == "--root") {
      if (n != string::npos) {
        RootDir = SGPath::fromLocal8Bit(value.c_str());
      }
      else {
        gripe;
        exit(1);
      }
    }
    else if (keyword == "--aircraft") {
      if (n != string::npos) {
        AircraftName = value;
      }
      else {
        gripe;
        exit(1);
      }
    }
    else if (keyword == "--script") {
      if (n != string::npos) {
        ScriptName = SGPath::fromLocal8Bit(value.c_str());
      }
      else {
        gripe;
        exit(1);
      }
    }
    else if (keyword == "--show_table_png") {
      if (n != string::npos) {
        if (value == "on") {
          show_table_png = true;
        }
        else {
          show_table_png = false;
        }
      }
      else {
        gripe;
        exit(1);
      }
    }
    else if (keyword == "--imagepath") {
      if (n != string::npos) {
        imagepath = value;
      }
      else {
        gripe;
        exit(1);
      }
    }
    else //Unknown keyword so print the help file, the bad keyword and abort
    {
      PrintHelp();
      cerr << "The argument \"" << keyword << "\" cannot be interpreted as a file name or option." << endl;
      exit(1);
    }

  }

  // Post-processing for script options. check for incompatible options.

  if (!ScriptName.isNull() && !AircraftName.empty()) {
    cerr << "You cannot specify an aircraft file with a script." << endl;
    result = false;
  }

  return result;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int main(int argc, char* argv[])
{
  bool result = false, success;
  success = options(argc, argv);
  if (!success) {
    PrintHelp();
    exit(-1);
  }

  // *** SET UP JSBSIM *** //
  FDMExec = make_shared<JSBSim::FGFDMExec>();
  FDMExec->SetRootDir(RootDir);
  FDMExec->SetAircraftPath(SGPath("aircraft"));
  FDMExec->SetEnginePath(SGPath("engine"));
  FDMExec->SetSystemsPath(SGPath("systems"));

  // *** OPTION A: LOAD A SCRIPT, WHICH LOADS EVERYTHING ELSE *** //
  if (!ScriptName.isNull()) {

    result = FDMExec->LoadScript(ScriptName);

    if (!result) {
      cerr << "Script file " << ScriptName << " was not successfully loaded" << endl;
      exit(-1);
    }

    // *** OPTION B: LOAD AN AIRCRAFT *** //
  }
  else if (!AircraftName.empty()) {

    if (!FDMExec->LoadModel(SGPath("aircraft"),
      SGPath("engine"),
      SGPath("systems"),
      AircraftName)) {
      cerr << "  JSBSim could not be started" << endl << endl;
      exit(-1);
    }
  }
  else {
    cout << "  No Aircraft or Script information given" << endl << endl;
    exit(-1);
  }
  
  viz = new aerodynamics_2_graphviz();
  viz->set_imagepath(imagepath);
  viz->graph_jsbsim_fdm_aerodynamics(FDMExec, outputfile,show_table_png);
}