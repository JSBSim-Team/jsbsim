#include <sstream>
#include <limits>

#include <cxxtest/TestSuite.h>
#include <math/FGTable.h>
#include "TestUtilities.h"

const double epsilon = 100. * std::numeric_limits<double>::epsilon();

using namespace JSBSim;


class FGTable1DTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    FGTable t1(1);
    TS_ASSERT_EQUALS(t1.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t1.GetName(), std::string(""));
    TS_ASSERT(!t1.IsConstant());

    FGTable t2(2);
    TS_ASSERT_EQUALS(t2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t2.GetName(), std::string(""));
    TS_ASSERT(!t2.IsConstant());
  }

  void testPopulateAndGetElement() {
    FGTable t1(1);
    t1 << 0.0 << 1.0;
    TS_ASSERT_EQUALS(t1(1,0), 0.0);
    TS_ASSERT_EQUALS(t1(1,1), 1.0);
    TS_ASSERT_EQUALS(t1.GetElement(1,0), 0.0);
    TS_ASSERT_EQUALS(t1.GetElement(1,1), 1.0);

    FGTable t2(2);
    t2 << 1.0 << -1.0
       << 2.0 << 1.5;
    TS_ASSERT_EQUALS(t2(1,0), 1.0);
    TS_ASSERT_EQUALS(t2(1,1), -1.0);
    TS_ASSERT_EQUALS(t2(2,0), 2.0);
    TS_ASSERT_EQUALS(t2(2,1), 1.5);
    TS_ASSERT_EQUALS(t2.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t2.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t2.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t2.GetElement(2,1), 1.5);
  }

  void testCopyConstructor() {
    FGTable t(2);
    t << 1.0 << -1.0
      << 2.0 << 1.5;
    TS_ASSERT(!t.IsConstant());

    FGTable t2(t);
    TS_ASSERT_EQUALS(t2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t2.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t2.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t2.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t2.GetElement(2,1), 1.5);
    TS_ASSERT(!t2.IsConstant());

    // Check that the data of the 2 tables is independent.
    FGTable temp(2);
    temp << 1.0 << -1.0;
    TS_ASSERT(!temp.IsConstant());

    FGTable temp2(temp);
    // Alter the data of the 2 tables *after* the copy.
    temp << 2.0 << 1.5;
    temp2 << 2.5 << -3.2;
    TS_ASSERT(!temp2.IsConstant());

    TS_ASSERT_EQUALS(temp.GetNumRows(), 2);
    TS_ASSERT_EQUALS(temp.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(temp.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(temp.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(temp.GetElement(2,1), 1.5);

    TS_ASSERT_EQUALS(temp2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(temp2.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(temp2.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(temp2.GetElement(2,0), 2.5);
    TS_ASSERT_EQUALS(temp2.GetElement(2,1), -3.2);
  }

  void testGetValue() {
    FGTable t1(1);
    t1 << 0.0 << 1.0;
    TS_ASSERT_EQUALS(t1.GetValue(-1.3), 1.0);
    TS_ASSERT_EQUALS(t1.GetValue(0.0), 1.0);
    TS_ASSERT_EQUALS(t1.GetValue(2.5), 1.0);

    FGTable t2(2);
    t2 << 1.0 << -1.0
       << 2.0 << 1.5;
    TS_ASSERT_EQUALS(t2.GetValue(0.3), -1.0);  // Saturated value
    TS_ASSERT_EQUALS(t2.GetValue(1.0), -1.0);  // Table data
    TS_ASSERT_EQUALS(t2.GetValue(1.5),  0.25); // Interpolation
    TS_ASSERT_EQUALS(t2.GetValue(2.0),  1.5);  // Table data
    TS_ASSERT_EQUALS(t2.GetValue(2.47), 1.5);  // Saturated value
  }

  void testLookupProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto node = pm->GetNode("x", true);
    FGTable t(2);
    t << 1.0 << -1.0
      << 2.0 << 1.5;
    t.SetRowIndexProperty(node);

    node->setDoubleValue(0.3);
    TS_ASSERT_EQUALS(t.GetValue(), -1.0);
    node->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t.GetValue(), -1.0);
    node->setDoubleValue(1.5);
    TS_ASSERT_EQUALS(t.GetValue(), 0.25);
    node->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t.GetValue(),  1.5);
    node->setDoubleValue(2.47);
    TS_ASSERT_EQUALS(t.GetValue(),  1.5);
  }

  void testMinValue() {
    FGTable t1(1);
    t1 << 0.0 << 1.0;

    TS_ASSERT_EQUALS(t1.GetMinValue(), 1.0);

    FGTable t21(2);
    t21 << 0.0 << -1.0
        << 1.0 << 5.0;

    TS_ASSERT_EQUALS(t21.GetMinValue(), -1.0);

    FGTable t22(2);
    t22 << 0.0 << 1.0
        << 1.0 << -5.0;

    TS_ASSERT_EQUALS(t22.GetMinValue(), -5.0);

    FGTable t31(3);
    t31 << 0.0 << -1.0
        << 1.0 << 5.0
        << 3.0 << 3.0;

    TS_ASSERT_EQUALS(t31.GetMinValue(), -1.0);

    FGTable t32(3);
    t32 << 0.0 << 1.0
        << 1.0 << -5.0
        << 3.0 << 3.0;

    TS_ASSERT_EQUALS(t32.GetMinValue(), -5.0);

    FGTable t33(3);
    t33 << 0.0 << 1.0
        << 1.0 << 5.0
        << 3.0 << -3.0;

    TS_ASSERT_EQUALS(t33.GetMinValue(), -3.0);
  }

  void testLoadInternalFromXML() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0 = readFromXML("<dummy>"
                                  "  <table name=\"test0\" type=\"internal\">"
                                  "    <tableData>"
                                  "      0.0 1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm0->FindElement("table");

    FGTable t_1x1(pm, el_table);
    TS_ASSERT_EQUALS(t_1x1.GetName(), std::string("test0"));
    TS_ASSERT_EQUALS(t_1x1.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x1.GetElement(1,0), 0.0);
    TS_ASSERT_EQUALS(t_1x1.GetElement(1,1), 1.0);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm->FindElement("table");

    FGTable t_2x1(pm, el_table);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test"));
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), 1.5);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm2 = readFromXML("<dummy>"
                                  "  <table name=\"test2\" type=\"internal\">"
                                  "    <tableData>"
                                  "      1.0  1.0\n"
                                  "      2.0  0.5\n"
                                  "      4.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm2->FindElement("table");

    FGTable t_3x1(pm, el_table);
    TS_ASSERT_EQUALS(t_3x1.GetNumRows(), 3);
    TS_ASSERT_EQUALS(t_3x1.GetName(), std::string("test2"));
    TS_ASSERT_EQUALS(t_3x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_3x1.GetElement(1,1), 1.0);
    TS_ASSERT_EQUALS(t_3x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_3x1.GetElement(2,1), 0.5);
    TS_ASSERT_EQUALS(t_3x1.GetElement(3,0), 4.0);
    TS_ASSERT_EQUALS(t_3x1.GetElement(3,1), 0.5);
  }

  void testLoadIndepVarFromXML() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0 = readFromXML("<dummy>"
                                  "  <table name=\"test0\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "      0.0 1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm0->FindElement("table");

    FGTable t_1x1(pm, el_table);
    TS_ASSERT_EQUALS(t_1x1.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x1.GetName(), std::string("test0"));
    TS_ASSERT_EQUALS(t_1x1.GetElement(1,0), 0.0);
    TS_ASSERT_EQUALS(t_1x1.GetElement(1,1), 1.0);
    // Check that the property "test" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test0"));

    auto output0 = pm->GetNode("test0");
    // Check that modifying the "x" property results in the table issuing
    // consistent results; including setting its bound property "test".
    x->setDoubleValue(-0.5);
    TS_ASSERT_EQUALS(t_1x1.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output0->getDoubleValue(), 1.0);
    x->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output0->getDoubleValue(), 1.0);
    x->setDoubleValue(0.3);
    TS_ASSERT_EQUALS(t_1x1.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output0->getDoubleValue(), 1.0);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm->FindElement("table");

    FGTable t_2x1(pm, el_table);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test"));
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), 1.5);
    // Check that the property "test" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test"));

    auto output = pm->GetNode("test");
    // Check that modifying the "x" property results in the table issuing
    // consistent results; including setting its bound property "test".
    x->setDoubleValue(0.3);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.0);
    x->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.0);
    x->setDoubleValue(1.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.25);
    x->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(),  1.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 1.5);
    x->setDoubleValue(2.47);
    TS_ASSERT_EQUALS(t_2x1.GetValue(),  1.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 1.5);
  }

  void testLoadWithNumericPrefix() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x2", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test#\">"
                                  "    <independentVar>x#</independentVar>"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    FGTable t_2x1(pm, el_table, "2");
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test2"));
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), 1.5);
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test2"));

    auto output = pm->GetNode("test2");
    x->setDoubleValue(1.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.25);
  }

  void testLoadWithStringPrefix() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    FGTable t_2x1(pm, el_table, "tables");
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("tables/test"));
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), 1.5);
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("tables/test"));

    auto output = pm->GetNode("tables/test");
    x->setDoubleValue(1.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.25);
  }

  void testMonoticallyIncreasingRows() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\" type=\"internal\">"
                                  "    <tableData>"
                                  "      1.0  1.0\n"
                                  "      1.0  0.5\n"
                                  "      2.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element *el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_3x1(pm, el_table), BaseException&);
  }
};


class FGTable2DTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    FGTable t_1x1(1,1);
    TS_ASSERT_EQUALS(t_1x1.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x1.GetName(), std::string(""));
    TS_ASSERT(!t_1x1.IsConstant());

    FGTable t_2x1(2,1);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string(""));
    TS_ASSERT(!t_2x1.IsConstant());

    FGTable t_1x2(1,2);
    TS_ASSERT_EQUALS(t_1x2.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x2.GetName(), std::string(""));
    TS_ASSERT(!t_1x2.IsConstant());

    FGTable t_2x2(2,2);
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string(""));
    TS_ASSERT(!t_2x2.IsConstant());
  }

  void testPopulateAndGetElement() {
    FGTable t_1x1(1,1);
    t_1x1 << 0.0
          << 1.0 << 2.0;
    TS_ASSERT_EQUALS(t_1x1(0,1), 0.0);
    TS_ASSERT_EQUALS(t_1x1(1,0), 1.0);
    TS_ASSERT_EQUALS(t_1x1(1,1), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetElement(0,1), 0.0);
    TS_ASSERT_EQUALS(t_1x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_1x1.GetElement(1,1), 2.0);

    FGTable t_2x1(2,1);
    t_2x1 << 0.0
          << 1.0 << 2.0
          << 3.0 << -1.0;
    TS_ASSERT_EQUALS(t_2x1(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x1(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1(1,1), 2.0);
    TS_ASSERT_EQUALS(t_2x1(2,0), 3.0);
    TS_ASSERT_EQUALS(t_2x1(2,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 3.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), -1.0);

    FGTable t_1x2(1,2);
    t_1x2 << 0.0 << 1.0
          << 2.0 << 3.0 << -1.0;
    TS_ASSERT_EQUALS(t_1x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_1x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_1x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_1x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_1x2(1,2), -1.0);
    TS_ASSERT_EQUALS(t_1x2.GetElement(0,1), 0.0);
    TS_ASSERT_EQUALS(t_1x2.GetElement(0,2), 1.0);
    TS_ASSERT_EQUALS(t_1x2.GetElement(1,0), 2.0);
    TS_ASSERT_EQUALS(t_1x2.GetElement(1,1), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetElement(1,2), -1.0);

    FGTable t_2x2(2,2);
    t_2x2 << 0.0 << 1.0
          << 2.0 << 3.0 << -1.0
          << 4.0 << -0.5 << 0.3;
    TS_ASSERT_EQUALS(t_2x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2(1,2), -1.0);
    TS_ASSERT_EQUALS(t_2x2(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2(2,1), -0.5);
    TS_ASSERT_EQUALS(t_2x2(2,2), 0.3);
    TS_ASSERT_EQUALS(t_2x2.GetElement(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2.GetElement(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2.GetElement(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2.GetElement(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2.GetElement(1,2), -1.0);
    TS_ASSERT_EQUALS(t_2x2.GetElement(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2.GetElement(2,1), -0.5);
    TS_ASSERT_EQUALS(t_2x2.GetElement(2,2), 0.3);
  }

  void testCopyConstructor() {
    FGTable temp0(2,2);
    temp0 << 0.0 << 1.0
          << 2.0 << 3.0 << -1.0
          << 4.0 << -0.5 << 0.3;
    TS_ASSERT(!temp0.IsConstant());

    FGTable t_2x2(temp0);
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string(""));
    TS_ASSERT_EQUALS(t_2x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2(1,2), -1.0);
    TS_ASSERT_EQUALS(t_2x2(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2(2,1), -0.5);
    TS_ASSERT_EQUALS(t_2x2(2,2), 0.3);
    TS_ASSERT(!t_2x2.IsConstant());

    FGTable temp1(2,2);
    temp1 << 0.0 << 1.0
          << 2.0 << 3.0 << -1.0;
    // Copy temp1 before the table is completely populated
    FGTable t2(temp1);
    t2 << 4.0 << -0.5 << 0.3;

    // Alter temp1 to make sure this is not modifying t2.
    temp1 << 10.0 << 11.0 << -12.0;
    TS_ASSERT_EQUALS(temp1(2,0), 10.0);
    TS_ASSERT_EQUALS(temp1(2,1), 11.0);
    TS_ASSERT_EQUALS(temp1(2,2), -12.0);
    TS_ASSERT(!temp1.IsConstant());

    TS_ASSERT_EQUALS(t2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t2.GetName(), std::string(""));
    TS_ASSERT_EQUALS(t2(0,1), 0.0);
    TS_ASSERT_EQUALS(t2(0,2), 1.0);
    TS_ASSERT_EQUALS(t2(1,0), 2.0);
    TS_ASSERT_EQUALS(t2(1,1), 3.0);
    TS_ASSERT_EQUALS(t2(1,2), -1.0);
    TS_ASSERT_EQUALS(t2(2,0), 4.0);
    TS_ASSERT_EQUALS(t2(2,1), -0.5);
    TS_ASSERT_EQUALS(t2(2,2), 0.3);
    TS_ASSERT(!t2.IsConstant());
  }

  void testGetValue() {
    FGTable t_1x1(1,1);
    t_1x1 << 0.0
          << 1.0 << 2.0;
    TS_ASSERT_EQUALS(t_1x1.GetValue(0.0, -1.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(1.0, -1.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(2.0, -1.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(0.0, 0.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(1.0, 0.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(2.0, 0.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(0.0, 1.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(1.0, 1.0), 2.0);
    TS_ASSERT_EQUALS(t_1x1.GetValue(2.0, 1.0), 2.0);

    FGTable t_2x1(2,1);
    t_2x1 << 0.0
          << 1.0 << 2.0
          << 3.0 << -1.0;
    TS_ASSERT_EQUALS(t_2x1.GetValue(0.0, -1.0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(1.0, -1.0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(2.0, -1.0), 0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(3.0, -1.0), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(4.0, -1.0), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(0.0, 0.0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(1.0, 0.0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(2.0, 0.0), 0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(3.0, 0.0), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(4.0, 0.0), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(0.0, 1.0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(1.0, 1.0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(2.0, 1.0), 0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(3.0, 1.0), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(4.0, 1.0), -1.0);

    FGTable t_1x2(1,2);
    t_1x2 << 0.0 << 1.0
          << 2.0 << 3.0 << -1.0;
    TS_ASSERT_EQUALS(t_1x2.GetValue(1.0, -1.0), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(1.0, 0.0), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(1.0, 0.5), 1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(1.0, 1.0), -1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(1.0, 2.0), -1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(2.0, -1.0), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(2.0, 0.0), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(2.0, 0.5), 1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(2.0, 1.0), -1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(2.0, 2.0), -1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(3.0, -1.0), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(3.0, 0.0), 3.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(3.0, 0.5), 1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(3.0, 1.0), -1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(3.0, 2.0), -1.0);

    FGTable t_2x2(2,2);
    t_2x2 << 0.0 << 1.0
          << 2.0 << 3.0 << -2.0
          << 4.0 << -1.0 << 0.5;
    // Saturated by column value
    TS_ASSERT_EQUALS(t_2x2.GetValue(1.0, -1.0), 3.0);  // Saturated by row value
    TS_ASSERT_EQUALS(t_2x2.GetValue(2.0, -1.0), 3.0);  // Test at table row data
    TS_ASSERT_EQUALS(t_2x2.GetValue(3.0, -1.0), 1.0);  // Interpolate row data
    TS_ASSERT_EQUALS(t_2x2.GetValue(4.0, -1.0), -1.0); // Test at atble row data
    TS_ASSERT_EQUALS(t_2x2.GetValue(5.0, -1.0), -1.0); // Saturated by row value

    // Test at the table column data : 0.0
    TS_ASSERT_EQUALS(t_2x2.GetValue(1.0, 0.0), 3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(2.0, 0.0), 3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(3.0, 0.0), 1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(4.0, 0.0), -1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(5.0, 0.0), -1.0);

    // Interpolate column data
    TS_ASSERT_EQUALS(t_2x2.GetValue(1.0, 0.5), 0.5);
    TS_ASSERT_EQUALS(t_2x2.GetValue(2.0, 0.5), 0.5);
    TS_ASSERT_EQUALS(t_2x2.GetValue(3.0, 0.5), 0.125);
    TS_ASSERT_EQUALS(t_2x2.GetValue(4.0, 0.5), -0.25);
    TS_ASSERT_EQUALS(t_2x2.GetValue(5.0, 0.5), -0.25);

    // Test at the table column data : 1.0
    TS_ASSERT_EQUALS(t_2x2.GetValue(1.0, 1.0), -2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(2.0, 1.0), -2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(3.0, 1.0), -0.75);
    TS_ASSERT_EQUALS(t_2x2.GetValue(4.0, 1.0), 0.5);
    TS_ASSERT_EQUALS(t_2x2.GetValue(5.0, 1.0), 0.5);

    // Saturated by column value
    TS_ASSERT_EQUALS(t_2x2.GetValue(1.0, 2.0), -2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(2.0, 2.0), -2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(3.0, 2.0), -0.75);
    TS_ASSERT_EQUALS(t_2x2.GetValue(4.0, 2.0), 0.5);
    TS_ASSERT_EQUALS(t_2x2.GetValue(5.0, 2.0), 0.5);
  }

  void testLookupProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y", true);
    FGTable t_2x2(2,2);

    t_2x2 << 0.0 << 1.0
          << 2.0 << 3.0 << -2.0
          << 4.0 << -1.0 << 0.5;
    t_2x2.SetColumnIndexProperty(column);
    t_2x2.SetRowIndexProperty(row);

    column->setDoubleValue(-1.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 1.0);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);

    column->setDoubleValue(0.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 1.0);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);

    column->setDoubleValue(0.5);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.125);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.25);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.25);

    column->setDoubleValue(1.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.75);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);

    column->setDoubleValue(2.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.75);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
  }

  void testLoadInternalFromXML() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0h = readFromXML("<dummy>"
                                  "  <table name=\"test0h\" type=\"internal\">"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm0h->FindElement("table");

    FGTable t_1x2(pm, el_table);
    TS_ASSERT_EQUALS(t_1x2.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x2.GetName(), std::string("test0h"));
    TS_ASSERT_EQUALS(t_1x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_1x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_1x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_1x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_1x2(1,2), -2.0);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0v = readFromXML("<dummy>"
                                  "  <table name=\"test0v\" type=\"internal\">"
                                  "    <tableData>"
                                  "            0.0\n"
                                  "      2.0   3.0\n"
                                  "      4.0  -1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm0v->FindElement("table");

    FGTable t_2x1(pm, el_table);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test0v"));
    TS_ASSERT_EQUALS(t_2x1(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x1(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x1(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x1(2,1), -1.0);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm->FindElement("table");

    FGTable t_2x2(pm, el_table);
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string("test"));
    TS_ASSERT_EQUALS(t_2x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2(1,2), -2.0);
    TS_ASSERT_EQUALS(t_2x2(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2(2,1), -1.0);
    TS_ASSERT_EQUALS(t_2x2(2,2), 0.5);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm2 = readFromXML("<dummy>"
                                  "  <table name=\"test2\" type=\"internal\">"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      1.0   1.0 -2.0\n"
                                  "      2.0   1.0  0.5\n"
                                  "      4.0   0.5  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm2->FindElement("table");

    FGTable t_3x2(pm, el_table);
    TS_ASSERT_EQUALS(t_3x2.GetNumRows(), 3);
    TS_ASSERT_EQUALS(t_3x2.GetName(), std::string("test2"));
    TS_ASSERT_EQUALS(t_3x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_3x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_3x2(1,0), 1.0);
    TS_ASSERT_EQUALS(t_3x2(1,1), 1.0);
    TS_ASSERT_EQUALS(t_3x2(1,2), -2.0);
    TS_ASSERT_EQUALS(t_3x2(2,0), 2.0);
    TS_ASSERT_EQUALS(t_3x2(2,1), 1.0);
    TS_ASSERT_EQUALS(t_3x2(2,2), 0.5);
    TS_ASSERT_EQUALS(t_3x2(3,0), 4.0);
    TS_ASSERT_EQUALS(t_3x2(3,1), 0.5);
    TS_ASSERT_EQUALS(t_3x2(3,2), 0.5);
  }

  void testLoadIndepVarFromXML() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0h = readFromXML("<dummy>"
                                  "  <table name=\"test0h\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm0h->FindElement("table");

    FGTable t_1x2(pm, el_table);
    TS_ASSERT_EQUALS(t_1x2.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x2.GetName(), std::string("test0h"));
    TS_ASSERT_EQUALS(t_1x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_1x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_1x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_1x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_1x2(1,2), -2.0);
    // Check that the property "test0h" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test0h"));

    auto output0h = pm->GetNode("test0h");
    // Check that modifying the "x" and "y" properties results in the table
    // issuing consistent results; including setting its bound property "test".

    row->setDoubleValue(0.0);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 3.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 3.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 0.5);
    column->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), -2.0);
    column->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), -2.0);

    row->setDoubleValue(2.0);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 3.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 3.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 0.5);
    column->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), -2.0);
    column->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), -2.0);

    row->setDoubleValue(2.5);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 3.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 3.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), 0.5);
    column->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), -2.0);
    column->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_1x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output0h->getDoubleValue(), -2.0);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0v = readFromXML("<dummy>"
                                  "  <table name=\"test0v\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0\n"
                                  "      2.0   3.0\n"
                                  "      4.0  -1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm0v->FindElement("table");

    FGTable t_2x1(pm, el_table);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test0v"));
    TS_ASSERT_EQUALS(t_2x1(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x1(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x1(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x1(2,1), -1.0);
    // Check that the property "test0v" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test0v"));

    auto output0v = pm->GetNode("test0v");
    // Check that modifying the "x" and "y" properties results in the table
    // issuing consistent results; including setting its bound property "test".

    row->setDoubleValue(1.0);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 3.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 3.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 3.0);

    row->setDoubleValue(2.0);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 3.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 3.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 3.0);

    row->setDoubleValue(3.0);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 1.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 1.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), 1.0);

    row->setDoubleValue(4.0);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), -1.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), -1.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), -1.0);

    row->setDoubleValue(4.5);
    column->setDoubleValue(-1.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), -1.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), -1.0);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x1.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output0v->getDoubleValue(), -1.0);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm->FindElement("table");

    FGTable t_2x2(pm, el_table);
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string("test"));
    TS_ASSERT_EQUALS(t_2x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2(1,2), -2.0);
    TS_ASSERT_EQUALS(t_2x2(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2(2,1), -1.0);
    TS_ASSERT_EQUALS(t_2x2(2,2), 0.5);
    // Check that the property "test" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test"));

    auto output = pm->GetNode("test");
    // Check that modifying the "x" and "y" properties results in the table
    // issuing consistent results; including setting its bound property "test".

    column->setDoubleValue(-1.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 3.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 3.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 1.0);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.0);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.0);

    column->setDoubleValue(0.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 3.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 3.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 3.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 1.0);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.0);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -1.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.0);

    column->setDoubleValue(0.5);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.125);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.125);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.25);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.25);

    column->setDoubleValue(1.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -2.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -2.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.75);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.75);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);

    column->setDoubleValue(2.0);
    row->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -2.0);
    row->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -2.0);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), -0.75);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.75);
    row->setDoubleValue(4.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);
    row->setDoubleValue(5.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);
  }

  void testLoadWithNumericPrefix() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y2", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test#\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y#</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    FGTable t_2x2(pm, el_table, "2");
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string("test2"));
    TS_ASSERT_EQUALS(t_2x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2(1,2), -2.0);
    TS_ASSERT_EQUALS(t_2x2(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2(2,1), -1.0);
    TS_ASSERT_EQUALS(t_2x2(2,2), 0.5);
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test2"));

    auto output = pm->GetNode("test2");
    column->setDoubleValue(0.5);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.125);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.125);
  }

  void testLoadWithStringPrefix() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    FGTable t_2x2(pm, el_table, "tables");
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string("tables/test"));
    TS_ASSERT_EQUALS(t_2x2(0,1), 0.0);
    TS_ASSERT_EQUALS(t_2x2(0,2), 1.0);
    TS_ASSERT_EQUALS(t_2x2(1,0), 2.0);
    TS_ASSERT_EQUALS(t_2x2(1,1), 3.0);
    TS_ASSERT_EQUALS(t_2x2(1,2), -2.0);
    TS_ASSERT_EQUALS(t_2x2(2,0), 4.0);
    TS_ASSERT_EQUALS(t_2x2(2,1), -1.0);
    TS_ASSERT_EQUALS(t_2x2(2,2), 0.5);
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("tables/test"));

    auto output = pm->GetNode("tables/test");
    column->setDoubleValue(0.5);
    row->setDoubleValue(3.0);
    TS_ASSERT_EQUALS(t_2x2.GetValue(), 0.125);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.125);
  }

  void testMonoticallyIncreasingRows() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      2.0   2.5 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_3x2(pm, el_table), BaseException&);
  }

  void testMonoticallyIncreasingColumns() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <tableData>"
                                  "            0.0  1.0 1.0\n"
                                  "      2.0   3.0 -2.0 1.0\n"
                                  "      4.0  -1.0  0.5 0.75\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x3(pm, el_table), BaseException&);
  }
};

class FGTable3DTest : public CxxTest::TestSuite
{
public:
  void testLoadIndepVarFromXML() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y", true);
    auto table = pm->GetNode("z", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData breakPoint=\"-1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData breakPoint=\"0.5\">"
                                  "            0.5  1.5\n"
                                  "      2.5   3.5 -2.5\n"
                                  "      4.5  -1.5  1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    FGTable t_2x2x2(pm, el_table);
    TS_ASSERT_EQUALS(t_2x2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2x2.GetName(), std::string("test2"));
    // Check breakpoints value
    TS_ASSERT_EQUALS(t_2x2x2(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x2x2(2,1), 0.5);
    TS_ASSERT(!t_2x2x2.IsConstant());

    // Check the table values.
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.0, 0.0, -1.0), 3.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.0, 0.0, -1.0), -1.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.0, 1.0, -1.0), -2.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.0, 1.0, -1.0), 0.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.5, 0.5, 0.5), 3.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.5, 0.5, 0.5), -1.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.5, 1.5, 0.5), -2.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.5, 1.5, 0.5), 1.0);
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test2"));
    auto output = pm->GetNode("test2");

    table->setDoubleValue(0.5);
    row->setDoubleValue(2.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), 3.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 3.5);

    table->setDoubleValue(-0.7);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), 3.1);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 3.1);

    table->setDoubleValue(0.5);
    row->setDoubleValue(4.0);
    column->setDoubleValue(0.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.25);

    table->setDoubleValue(-0.7);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -0.85);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.85);

    table->setDoubleValue(0.5);
    row->setDoubleValue(2.0);
    column->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);

    table->setDoubleValue(-0.7);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -1.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.5);

    table->setDoubleValue(0.5);
    row->setDoubleValue(4.0);
    column->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -0.0625);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.0625);

    table->setDoubleValue(-0.7);
    TS_ASSERT_DELTA(t_2x2x2.GetValue(), 0.3875, epsilon);
    TS_ASSERT_DELTA(output->getDoubleValue(), 0.3875, epsilon);

    table->setDoubleValue(-1.0);
    row->setDoubleValue(2.5);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), 0.3125);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.3125);

    table->setDoubleValue(-0.7);
    TS_ASSERT_DELTA(t_2x2x2.GetValue(), 0.95, epsilon);
    TS_ASSERT_DELTA(output->getDoubleValue(), 0.95, epsilon);

    table->setDoubleValue(-1.0);
    row->setDoubleValue(4.5);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -0.25);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.25);

    table->setDoubleValue(-0.7);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -0.5);

    table->setDoubleValue(-1.0);
    row->setDoubleValue(2.5);
    column->setDoubleValue(1.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -1.375);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.375);

    table->setDoubleValue(-0.7);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -1.6);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.6);

    table->setDoubleValue(-1.0);
    row->setDoubleValue(4.5);
    column->setDoubleValue(1.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), 0.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.5);

    table->setDoubleValue(-0.7);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), 0.6);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.6);

    table->setDoubleValue(-1.5);
    row->setDoubleValue(1.0);
    column->setDoubleValue(2.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -2.0);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -2.0);

    table->setDoubleValue(1.0);
    row->setDoubleValue(5.0);
    column->setDoubleValue(-0.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -1.5);
    TS_ASSERT_EQUALS(output->getDoubleValue(), -1.5);
  }

  void testCopyConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y", true);
    auto table = pm->GetNode("z", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData breakPoint=\"-1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData breakPoint=\"0.5\">"
                                  "            0.5  1.5\n"
                                  "      2.5   3.5 -2.5\n"
                                  "      4.5  -1.5  1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");
    auto ref = std::make_unique<FGTable>(pm, el_table);
    TS_ASSERT(!ref->IsConstant());
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test2"));
    auto output = pm->GetNode("test2");

    table->setDoubleValue(-1.0);
    row->setDoubleValue(2.5);
    column->setDoubleValue(0.5);
    TS_ASSERT_EQUALS(ref->GetValue(), 0.3125);
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.3125);

    FGTable t_2x2x2(*ref.get());
    // Delete the original table to make sure that `t_2x2x2` does not make use
    // of any of `ref` data.
    ref.reset();

    TS_ASSERT_EQUALS(t_2x2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2x2.GetName(), std::string("test2"));
    TS_ASSERT(!t_2x2x2.IsConstant());
    // Check breakpoints value
    TS_ASSERT_EQUALS(t_2x2x2(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x2x2(2,1), 0.5);

    // Check the table values.
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.0, 0.0, -1.0), 3.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.0, 0.0, -1.0), -1.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.0, 1.0, -1.0), -2.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.0, 1.0, -1.0), 0.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.5, 0.5, 0.5), 3.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.5, 0.5, 0.5), -1.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(2.5, 1.5, 0.5), -2.5);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(4.5, 1.5, 0.5), 1.0);

    table->setDoubleValue(0.5);
    row->setDoubleValue(4.0);
    column->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(t_2x2x2.GetValue(), -0.0625);

    // Check that the property `test2` has remained unchanged since the table
    // `ref` was destroyed.
    TS_ASSERT_EQUALS(output->getDoubleValue(), 0.3125);
  }
};


class FGTableErrorsTest : public CxxTest::TestSuite
{
public:
  void testTypeError() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"wrong\">"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void testLookupNameError() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <independentVar lookup=\"wrong\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void testMalformedData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <tableData>"
                                  "      1.0% -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void testPropertyAlreadyTied() {
    auto pm = std::make_shared<FGPropertyManager>();
    static double value = 0;
    pm->Tie("test", &value);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void testUnexpectedPrefix() {
    auto pm = std::make_shared<FGPropertyManager>();

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table, "0"), BaseException&);
  }

  void test1DInternalMissingTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void test1DInternalEmptyTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <tableData/>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void test1DMissingTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void test1DEmptyTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData/>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void test1DRowsNotIncreasing() {
    FGTable t(2);
    t << 1.0 << -1.0;
    TS_ASSERT_THROWS(t << 1.0, BaseException&);
  }

  void test1DMissingLookupAxis() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void test1DBadNumberOfColumns() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm0 = readFromXML("<dummy>"
                                  "  <table name=\"test0\">"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "      3.0  0.0 4.0\n"
                                  "      -0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm0->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_4x1(pm, el_table), BaseException&);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm1 = readFromXML("<dummy>"
                                  "  <table name=\"test1\">"
                                  "    <tableData>"
                                  "      1.0 -1.0 2.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm1->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_1x1(pm, el_table), BaseException&);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm2 = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5 3.0 0.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm2->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_1x2(pm, el_table), BaseException&);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm3 = readFromXML("<dummy>"
                                  "  <table name=\"test2\" type=\"internal\">"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "      3.0  0.0 4.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm3->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_3x1(pm, el_table), BaseException&);

    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm4 = readFromXML("<dummy>"
                                  "  <table name=\"test2\" type=\"internal\">"
                                  "    <tableData>"
                                  "      1.0 -1.0  2.3\n"
                                  "      2.0  1.5 -7.1\n"
                                  "      3.0  0.0  4.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    el_table = elm4->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_3x2(pm, el_table), BaseException&);
  }

  void test2DMissingColumnLookupAxis1() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void test2DMissingColumnLookupAxis2() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void test2DMissingRowLookupAxis() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"column\">x</independentVar>"
                                  "    <tableData>"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void test2DMissingTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\" type=\"internal\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void test2DEmptyTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData/>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void test2DColumnsNotIncreasing() {
    FGTable t(2,2);
    t << 1.0;
    TS_ASSERT_THROWS(t << 0.9, BaseException&);
  }

  void test2DRowsNotIncreasing() {
    FGTable t(2,2);
    t << 1.0 << 2.0
      << 1.0 << -1.0 << -2.5;
    TS_ASSERT_THROWS(t << 0.9, BaseException&);
  }

  void testXMLRowsNotIncreasing() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <tableData>"
                                  "      2.0 -1.0\n"
                                  "      1.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), BaseException&);
  }

  void testXMLColumnsNotIncreasing() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData>"
                                  "            1.0  0.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), BaseException&);
  }

  void testBreakpointsNotIncreasing() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData breakPoint=\"1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData breakPoint=\"0.5\">"
                                  "            0.5  1.5\n"
                                  "      2.5   3.5 -2.5\n"
                                  "      4.5  -1.5  1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }

  void test3DMissingRowLookupAxis() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData breakPoint=\"1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData breakPoint=\"2.0\">"
                                  "            0.5  1.5\n"
                                  "      2.5   3.5 -2.5\n"
                                  "      4.5  -1.5  1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }

  void test3DMissingColumnLookupAxis() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData breakPoint=\"1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData breakPoint=\"2.0\">"
                                  "            0.5  1.5\n"
                                  "      2.5   3.5 -2.5\n"
                                  "      4.5  -1.5  1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }

  void test3DMissingTableLookupAxis() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <tableData breakPoint=\"1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData breakPoint=\"2.0\">"
                                  "            0.5  1.5\n"
                                  "      2.5   3.5 -2.5\n"
                                  "      4.5  -1.5  1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }

  void test3DMissingTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }

  void test3DEmptyTableData() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData/>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }

  void test3DEmptyTableData2() {
    auto pm = std::make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test\">"
                                  "    <independentVar lookup=\"row\">x</independentVar>"
                                  "    <independentVar lookup=\"column\">y</independentVar>"
                                  "    <independentVar lookup=\"table\">z</independentVar>"
                                  "    <tableData breakPoint=\"1.0\">"
                                  "            0.0  1.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "    <tableData/>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), BaseException&);
  }
};
