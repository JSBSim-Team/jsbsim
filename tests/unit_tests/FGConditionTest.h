#include <array>

#include <cxxtest/TestSuite.h>
#include <math/FGCondition.h>
#include "TestUtilities.h"

using namespace JSBSim;


class FGConditionTest : public CxxTest::TestSuite
{
public:
  void testXMLEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> XML{"<dummy> x == 1.0 </dummy>",
                                         "<dummy> x EQ 1.0 </dummy>",
                                         "<dummy> x eq 1.0 </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> conditions{"x == 1.0", "x EQ 1.0", "x eq 1.0"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testXMLNotEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> XML{"<dummy> x != 1.0 </dummy>",
                                         "<dummy> x NE 1.0 </dummy>",
                                         "<dummy> x ne 1.0 </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testNotEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> conditions{"x != 1.0", "x NE 1.0", "x ne 1.0"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testXMLGreaterThanConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> XML{"<dummy> x &gt; 1.0 </dummy>",
                                "<dummy> x GT 1.0 </dummy>",
                                "<dummy> x gt 1.0 </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testGreaterThanConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> conditions{"x > 1.0", "x GT 1.0", "x gt 1.0"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testXMLGreaterOrEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> XML{"<dummy> x &gt;= 1.0 </dummy>",
                                "<dummy> x GE 1.0 </dummy>",
                                "<dummy> x ge 1.0 </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testGreaterOrEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> conditions{"x >= 1.0", "x GE 1.0", "x ge 1.0"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testXMLLowerThanConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> XML{"<dummy> x &lt; 1.0 </dummy>",
                                "<dummy> x LT 1.0 </dummy>",
                                "<dummy> x lt 1.0 </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testLowerThanConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> conditions{"x < 1.0", "x LT 1.0", "x lt 1.0"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testXMLLowerOrEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> XML{"<dummy> x &lt;= 1.0 </dummy>",
                                         "<dummy> x LE 1.0 </dummy>",
                                         "<dummy> x le 1.0 </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testLowerOrEqualConstant() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    const std::array<std::string, 3> conditions{"x <= 1.0", "x LE 1.0", "x le 1.0"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testXMLEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> XML{"<dummy> x == y </dummy>",
                                         "<dummy> x EQ y </dummy>",
                                         "<dummy> x eq y </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> conditions{"x == y", "x EQ y", "x eq y"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testXMLNotEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> XML{"<dummy> x != y </dummy>",
                                         "<dummy> x NE y </dummy>",
                                         "<dummy> x ne y </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(0.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testNotEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> conditions{"x != y", "x NE y", "x ne y"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(0.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testXMLGreaterThanProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> XML{"<dummy> x &gt; y </dummy>",
                                         "<dummy> x GT y </dummy>",
                                         "<dummy> x gt y </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testGreaterThanProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> conditions{"x > y", "x GT y", "x gt y"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testXMLGreaterOrEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> XML{"<dummy> x &gt;= y </dummy>",
                                         "<dummy> x GE y </dummy>",
                                         "<dummy> x ge y </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testGreaterOrEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> conditions{"x >= y", "x GE y", "x ge y"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());
    }
  }

  void testXMLLowerThanProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> XML{"<dummy> x &lt; y </dummy>",
                                         "<dummy> x LT y </dummy>",
                                         "<dummy> x lt y </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testLowerThanProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> conditions{"x < y", "x LT y", "x lt y"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testXMLLowerOrEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> XML{"<dummy> x &lt;= y </dummy>",
                                         "<dummy> x LE y </dummy>",
                                         "<dummy> x le y </dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testLowerOrEqualProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 3> conditions{"x <= y", "x LE y", "x le y"};
    for(const std::string& line: conditions) {
      FGCondition cond(line, pm, nullptr);

      x->setDoubleValue(-1.0);
      y->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(0.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(1.0);
      TS_ASSERT(cond.Evaluate());
      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testAND() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto onoff = pm->GetNode("on-off", true);
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    const std::array<std::string, 2> XML{"<dummy> on-off == 1\nx GE y</dummy>",
                                         "<dummy logic=\"AND\"> on-off == 1\nx GE y</dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      onoff->setDoubleValue(0.0);
      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());

      onoff->setDoubleValue(1.0);
      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());

      y->setDoubleValue(3.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testANDLateBound() {
    auto pm = std::make_shared<FGPropertyManager>();
    const std::array<std::string, 2> XML{"<dummy> on-off == 1\nx GE y</dummy>",
                                         "<dummy logic=\"AND\"> on-off == 1\nx GE y</dummy>"};
    for(const std::string& line: XML) {
      Element_ptr elm = readFromXML(line);
      FGCondition cond(elm, pm);

      auto onoff = pm->GetNode("on-off", true);
      auto x = pm->GetNode("x", true);
      auto y = pm->GetNode("y", true);

      onoff->setDoubleValue(0.0);
      x->setDoubleValue(0.0);
      y->setDoubleValue(1.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(2.0);
      TS_ASSERT(!cond.Evaluate());

      onoff->setDoubleValue(1.0);
      x->setDoubleValue(0.0);
      TS_ASSERT(!cond.Evaluate());

      x->setDoubleValue(2.0);
      TS_ASSERT(cond.Evaluate());

      y->setDoubleValue(3.0);
      TS_ASSERT(!cond.Evaluate());
    }
  }

  void testOR() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto onoff = pm->GetNode("on-off", true);
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    Element_ptr elm = readFromXML("<dummy logic=\"OR\">"
                                  "on-off == 1\n"
                                  "x GE y"
                                  "</dummy>");
    FGCondition cond(elm, pm);

    onoff->setDoubleValue(0.0);
    x->setDoubleValue(0.0);
    y->setDoubleValue(1.0);
    TS_ASSERT(!cond.Evaluate());

    x->setDoubleValue(2.0);
    TS_ASSERT(cond.Evaluate());

    y->setDoubleValue(3.0);
    TS_ASSERT(!cond.Evaluate());

    onoff->setDoubleValue(1.0);
    x->setDoubleValue(4.0);
    TS_ASSERT(cond.Evaluate());

    x->setDoubleValue(2.0);
    TS_ASSERT(cond.Evaluate());
  }

  void testNested() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto onoff = pm->GetNode("on-off", true);
    auto x = pm->GetNode("x", true);
    auto y = pm->GetNode("y", true);
    Element_ptr elm = readFromXML("<dummy>"
                                  "  on-off == 1"
                                  "  <dummy logic=\"AND\">"
                                  "    x GE y\n"
                                  "    x LT 2.0"
                                  "  </dummy>"
                                  "</dummy>");
    FGCondition cond(elm, pm);

    onoff->setDoubleValue(0.0);
    x->setDoubleValue(0.0);
    y->setDoubleValue(1.0);
    TS_ASSERT(!cond.Evaluate());

    x->setDoubleValue(1.5);
    TS_ASSERT(!cond.Evaluate());

    x->setDoubleValue(3.0);
    TS_ASSERT(!cond.Evaluate());

    onoff->setDoubleValue(1.0);
    x->setDoubleValue(0.0);
    TS_ASSERT(!cond.Evaluate());

    y->setDoubleValue(-1.0);
    TS_ASSERT(cond.Evaluate());

    x->setDoubleValue(3.0);
    TS_ASSERT(!cond.Evaluate());
  }

  void testIllegalLOGIC() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy logic=\"XOR\">"
                                  "  on-off == 1\n"
                                  "  x GE y"
                                  "</dummy>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);
  }

  void testWrongNumberOfElements() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy> on-off == </dummy>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);

    elm = readFromXML("<dummy> on-off </dummy>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);

    elm = readFromXML("<dummy/>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);

    elm = readFromXML("<dummy> 0.0 LE on-off GE 1.0 </dummy>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);
  }

  void testIllegalNested() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy>"
                                  "  on-off == 1"
                                  "  <crash logic=\"AND\">"
                                  "    x GE y\n"
                                  "    x LT 2.0"
                                  "  </crash>"
                                  "</dummy>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);
  }

  void testIllegalOperation() {
    auto pm = std::make_shared<FGPropertyManager>();
    Element_ptr elm = readFromXML("<dummy> on-off # 0.0 </dummy>");
    TS_ASSERT_THROWS(FGCondition cond(elm, pm), BaseException&);
  }
};
