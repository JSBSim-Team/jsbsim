#include <sstream>
#include <limits>

#include <cxxtest/TestSuite.h>
#include <math/FGTable.h>
#include <input_output/FGXMLParse.h>

const double epsilon = 100. * std::numeric_limits<double>::epsilon();

using namespace JSBSim;


Element_ptr readFromXML(const std::string& XML) {
  std::istringstream data(XML);
  FGXMLParse parser;
  readXML(data, parser);
  return parser.GetDocument();
}


class FGTable1DTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    FGTable t1(1);
    TS_ASSERT_EQUALS(t1.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t1.GetName(), std::string(""));

    FGTable t2(2);
    TS_ASSERT_EQUALS(t2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t2.GetName(), std::string(""));
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

    FGTable t2(t);
    TS_ASSERT_EQUALS(t2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t2.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t2.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t2.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t2.GetElement(2,1), 1.5);

    // Check that the data of the 2 tables is independent.
    FGTable temp(2);
    temp << 1.0 << -1.0;

    FGTable temp2(temp);
    // Alter the data of the 2 tables *after* the copy.
    temp << 2.0 << 1.5;
    temp2 << 2.5 << -3.2;

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
    auto pm = make_shared<FGPropertyManager>();
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

  void testLoadInternalFromXML() {
    auto pm = make_shared<FGPropertyManager>();
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
    Element* el_table = elm->FindElement("table");

    FGTable t_2x1(pm, el_table);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test"));
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), 1.5);
  }

  void testLoadIndepVarFromXML() {
    auto pm = make_shared<FGPropertyManager>();
    auto x = pm->GetNode("x", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <tableData>"
                                  "      1.0 -1.0\n"
                                  "      2.0  1.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    FGTable t_2x1(pm, el_table);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string("test2"));
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,0), 1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(1,1), -1.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,0), 2.0);
    TS_ASSERT_EQUALS(t_2x1.GetElement(2,1), 1.5);
    // Check that the property "test2" is now bound to the property manager
    TS_ASSERT(pm->HasNode("test2"));

    auto output = pm->GetNode("test2");
    // Check that modifying the "x" property results in the table issuing
    // consistent results; including setting its bound property "test2".
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
    auto pm = make_shared<FGPropertyManager>();
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
    auto pm = make_shared<FGPropertyManager>();
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
};


class FGTable2DTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    FGTable t_1x1(1,1);
    TS_ASSERT_EQUALS(t_1x1.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x1.GetName(), std::string(""));

    FGTable t_2x1(2,1);
    TS_ASSERT_EQUALS(t_2x1.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x1.GetName(), std::string(""));

    FGTable t_1x2(1,2);
    TS_ASSERT_EQUALS(t_1x2.GetNumRows(), 1);
    TS_ASSERT_EQUALS(t_1x2.GetName(), std::string(""));

    FGTable t_2x2(2,2);
    TS_ASSERT_EQUALS(t_2x2.GetNumRows(), 2);
    TS_ASSERT_EQUALS(t_2x2.GetName(), std::string(""));
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
  }

  void testGetValue() {
    FGTable t_1x1(1,1);
    t_1x1 << 0.0
          << 1.0 << 2.0;
    // SEGFAULT: Need to fix FGTable
    // TS_ASSERT_EQUALS(t_1x1.GetValue(0.0, -1.0), 2.0);

    FGTable t_2x1(2,1);
    t_2x1 << 0.0
          << 1.0 << 2.0
          << 3.0 << -1.0;
    // SEGFAULT: Need to fix FGTable
    // TS_ASSERT_EQUALS(t_2x1.GetValue(0.0, -1.0), 2.0);

    FGTable t_1x2(1,2);
    t_1x2 << 0.0 << 1.0
          << 2.0 << 3.0 << -1.0;
    // SEGFAULT: Need to fix FGTable
    // TS_ASSERT_EQUALS(t_1x2.GetValue(1.0, -1.0), 3.0);
    // TS_ASSERT_EQUALS(t_1x2.GetValue(0.5, 2.0), 1.0);

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
    auto pm = make_shared<FGPropertyManager>();
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
    auto pm = make_shared<FGPropertyManager>();
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
    Element* el_table = elm->FindElement("table");

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
  }

  void testLoadIndepVarFromXML() {
    auto pm = make_shared<FGPropertyManager>();
    auto row = pm->GetNode("x", true);
    auto column = pm->GetNode("y", true);
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
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

    FGTable t_2x2(pm, el_table);
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
    // Check that modifying the "x" and "y" properties results in the table
    // issuing consistent results; including setting its bound property "test2".

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
    auto pm = make_shared<FGPropertyManager>();
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
    auto pm = make_shared<FGPropertyManager>();
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
};

class FGTable3DTest : public CxxTest::TestSuite
{
public:
  void testLoadIndepVarFromXML() {
    auto pm = make_shared<FGPropertyManager>();
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
};


class FGTableErrorsTest : public CxxTest::TestSuite
{
public:
  void testTypeError() {
    auto pm = make_shared<FGPropertyManager>();
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

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), TableException&);
  }

  void testLookupError() {
    auto pm = make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
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

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), TableException&);
  }

  void testIncompleteDefinition() {
    auto pm = make_shared<FGPropertyManager>();
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

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), TableException&);
  }

  void testNotEnoughColumns() {
    auto pm = make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <independentVar lookup=\"wrong\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0\n"
                                  "      2.0   3.0\n"
                                  "      4.0  -1.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), TableException&);
  }

  void testNotEnoughRows() {
    auto pm = make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <independentVar lookup=\"wrong\">y</independentVar>"
                                  "    <tableData>"
                                  "            0.0 1.0\n"
                                  "      2.0   3.0 4.0\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), TableException&);
  }

  void testRowsNotIncreasing() {
    auto pm = make_shared<FGPropertyManager>();
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

    TS_ASSERT_THROWS(FGTable t_2x1(pm, el_table), TableException&);
  }

  void testColumnsNotIncreasing() {
    auto pm = make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
                                  "    <independentVar>x</independentVar>"
                                  "    <independentVar lookup=\"wrong\">y</independentVar>"
                                  "    <tableData>"
                                  "            1.0  0.0\n"
                                  "      2.0   3.0 -2.0\n"
                                  "      4.0  -1.0  0.5\n"
                                  "    </tableData>"
                                  "  </table>"
                                  "</dummy>");
    Element* el_table = elm->FindElement("table");

    TS_ASSERT_THROWS(FGTable t_2x2(pm, el_table), TableException&);
  }

  void testBreakpointsNotIncreasing() {
    auto pm = make_shared<FGPropertyManager>();
    // FGTable expects <table> to be the child of another XML element, hence the
    // <dummy> element.
    Element_ptr elm = readFromXML("<dummy>"
                                  "  <table name=\"test2\">"
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

    TS_ASSERT_THROWS(FGTable t_2x2x2(pm, el_table), TableException&);
  }
};
