#include <cxxtest/TestSuite.h>
#include <math/FGParameterValue.h>
#include "TestUtilities.h"

using namespace JSBSim;


class FGParameterValueTest : public CxxTest::TestSuite
{
public:
  void testRealConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    FGParameterValue x("1.2", pm, nullptr);

    TS_ASSERT(x.IsConstant());
    TS_ASSERT(!x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetValue(), 1.2);
    TS_ASSERT_EQUALS(x.GetName(), "constant value 1.200000");
  }

  void testPropertyConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto node = pm->GetNode("x", true);
    FGParameterValue x("x", pm, nullptr);

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT(!x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetName(), "x");

    node->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(x.GetValue(), 0.0);
    node->setDoubleValue(1.2);
    TS_ASSERT_EQUALS(x.GetValue(), 1.2);
  }

  void testLateBoundPropertyConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    FGParameterValue x("x", pm, nullptr);

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT(x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetName(), "x");

    auto node = pm->GetNode("x", true);
    node->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(x.GetValue(), 0.0);
    TS_ASSERT(!x.IsLateBound());
    node->setDoubleValue(1.2);
    TS_ASSERT_EQUALS(x.GetValue(), 1.2);
  }

  void testLateBoundPropertyIllegalAccess() {
    auto pm = std::make_shared<FGPropertyManager>();
    FGParameterValue x("x", pm, nullptr);

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT(x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetName(), "x");
    TS_ASSERT_THROWS(x.GetValue(), BaseException&);
  }

  void testXMLRealConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy> 1.2 </dummy>");
    FGParameterValue x(elm, pm);

    TS_ASSERT(x.IsConstant());
    TS_ASSERT(!x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetValue(), 1.2);
    TS_ASSERT_EQUALS(x.GetName(), "constant value 1.200000");
  }

  void testXMLPropertyConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto node = pm->GetNode("x", true);
    Element_ptr elm = readFromXML("<dummy> x </dummy>");
    FGParameterValue x(elm, pm);

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT(!x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetName(), "x");

    node->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(x.GetValue(), 0.0);
    node->setDoubleValue(1.2);
    TS_ASSERT_EQUALS(x.GetValue(), 1.2);
  }

  void testXMLLateBoundPropertyConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy> x </dummy>");
    FGParameterValue x(elm, pm);

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT(x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetName(), "x");

    auto node = pm->GetNode("x", true);
    node->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(x.GetValue(), 0.0);
    TS_ASSERT(!x.IsLateBound());
    node->setDoubleValue(1.2);
    TS_ASSERT_EQUALS(x.GetValue(), 1.2);
  }

  void testXMLLateBoundPropertyIllegalAccess() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy> x </dummy>");
    FGParameterValue x(elm, pm);

    TS_ASSERT(!x.IsConstant());
    TS_ASSERT(x.IsLateBound());
    TS_ASSERT_EQUALS(x.GetName(), "x");
    TS_ASSERT_THROWS(x.GetValue(), BaseException&);
  }

  void testXMLEmptyNameConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy/>");
    TS_ASSERT_THROWS(FGParameterValue x(elm, pm), BaseException&);
  }

  void testXMLMultiLinesConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy>x\ny</dummy>");
    TS_ASSERT_THROWS(FGParameterValue x(elm, pm), BaseException&);
  }
};
