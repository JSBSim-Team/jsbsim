#include <cxxtest/TestSuite.h>
#include <math/FGRealValue.h>

using namespace JSBSim;


class FGRealValueTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    FGRealValue x(1.0);

    TS_ASSERT_EQUALS(x.GetValue(), 1.0);
    TS_ASSERT_EQUALS(x.GetName(), "constant value 1.000000");
    TS_ASSERT(x.IsConstant());
  }
};
