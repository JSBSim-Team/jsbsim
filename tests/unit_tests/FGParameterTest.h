#include <cxxtest/TestSuite.h>
#include <math/FGParameter.h>

using namespace JSBSim;


// Dummy class that inherits the abstract class `FGParameter`.
class FGDummy : public FGParameter
{
public:
  FGDummy(void) : count(0) {}
  double GetValue(void) const { return count++; }
  std::string GetName(void) const { return "Counting..."; }
private:
  mutable unsigned int count;
};

class FGParameterTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    FGDummy x;

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT_EQUALS(x.GetValue(), 0.0);
    TS_ASSERT_EQUALS(x.getDoubleValue(), 1.0);
    TS_ASSERT_EQUALS(x.GetName(), "Counting...");
  }

  void testCopyConstructor() {
    FGDummy x;
    TS_ASSERT_EQUALS(x.GetValue(), 0.0);

    FGDummy y(x);
    TS_ASSERT_EQUALS(x.GetValue(), 1.0);
    TS_ASSERT_EQUALS(x.getDoubleValue(), 2.0);
    TS_ASSERT(!x.IsConstant());

    TS_ASSERT(!y.IsConstant());
    TS_ASSERT_EQUALS(y.GetValue(), 1.0);
    TS_ASSERT_EQUALS(y.GetName(), "Counting...");
    TS_ASSERT_EQUALS(x.GetValue(), 3.0);
  }

  void testOperators() {
    SGSharedPtr<FGDummy> px(new FGDummy);

    TS_ASSERT_EQUALS(px*2.0, 0.0);
    TS_ASSERT_EQUALS(-3.0*px, -3.0);
    TS_ASSERT_EQUALS(px*2.0, 4.0);
  }
};
