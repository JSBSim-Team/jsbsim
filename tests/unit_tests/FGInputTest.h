#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGInput.h>
#include "TestUtilities.h"

using namespace JSBSim;

class FGInputTest : public CxxTest::TestSuite
{
public:
  // A socket input whose port and protocol duplicate one already registered is
  // rejected; a socket on a different port is accepted. Load() only parses the
  // socket parameters (the port is bound later, in InitModel), so this exercises
  // the duplicate guard without opening a real socket.
  void testRejectsDuplicateSocketPort()
  {
    FGFDMExec fdmex;
    auto input = fdmex.GetInput();

    Element_ptr first = readFromXML("<input type=\"SOCKET\" port=\"1137\"/>");
    TS_ASSERT(input->Load(first));

    // Same port and protocol -> rejected.
    Element_ptr duplicate = readFromXML("<input type=\"SOCKET\" port=\"1137\"/>");
    TS_ASSERT(!input->Load(duplicate));

    // Different port -> accepted.
    Element_ptr different = readFromXML("<input type=\"SOCKET\" port=\"1138\"/>");
    TS_ASSERT(input->Load(different));
  }
};
