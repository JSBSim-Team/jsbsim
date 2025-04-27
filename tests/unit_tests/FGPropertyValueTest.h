#include <cxxtest/TestSuite.h>
#include <math/FGPropertyValue.h>

using namespace JSBSim;

class FGPropertyValueTest : public CxxTest::TestSuite
{
public:
  void testConstructorFromNode() {
    SGPropertyNode root;
    SGPropertyNode_ptr node = root.getNode("x", true);
    FGPropertyValue property(node);

    TS_ASSERT_EQUALS(property.GetValue(), 0.0);
    TS_ASSERT_EQUALS(property.IsConstant(), false);
    TS_ASSERT_EQUALS(property.IsLateBound(), false);
    TS_ASSERT_EQUALS(property.GetName(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetNameWithSign(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetFullyQualifiedName(), std::string("/x"));
    TS_ASSERT_EQUALS(property.GetPrintableName(), std::string("x"));
  }

  void testSetValue() {
    SGPropertyNode root;
    SGPropertyNode_ptr node = root.getNode("x", true);
    FGPropertyValue property(node);

    TS_ASSERT_EQUALS(node->getDoubleValue(), 0.0);
    property.SetValue(1.54);
    TS_ASSERT_EQUALS(property.GetValue(), 1.54);
    TS_ASSERT_EQUALS(node->getDoubleValue(), 1.54);
  }

  void testSetNode() {
    SGPropertyNode root;
    SGPropertyNode_ptr node_x = root.getNode("x", true);
    SGPropertyNode_ptr node_y = root.getNode("y", true);
    FGPropertyValue property(node_x);

    node_y->setDoubleValue(-1.547);
    TS_ASSERT_EQUALS(property.GetValue(), 0.0);
    TS_ASSERT_EQUALS(property.GetName(), "x");
    property.SetNode(node_y);
    TS_ASSERT_EQUALS(property.GetValue(), -1.547);
    TS_ASSERT_EQUALS(property.GetName(), "y");
  }

  void testConstant_ness() {
    auto pm = std::make_shared<FGPropertyManager>();
    SGPropertyNode_ptr node = pm->GetNode("x", true);
    FGPropertyValue property(node);

    TS_ASSERT(!property.IsConstant());
    node->setAttribute(SGPropertyNode::WRITE, false);
    TS_ASSERT(property.IsConstant());
  }

  void testTiedPropertiesAreNotConstant() {
    // Check that tied properties are not constant even if the underlying
    // property is set to READ ONLY.
    auto pm = std::make_shared<FGPropertyManager>();
    double value = 0.0;
    SGPropertyNode_ptr node = pm->GetNode("x", true);
    FGPropertyValue property(node);

    node->setAttribute(SGPropertyNode::WRITE, false);

    pm->Tie("x", &value);
    TS_ASSERT(!node->getAttribute(SGPropertyNode::WRITE)); // READ ONLY
    TS_ASSERT(!property.IsConstant()); // but not constant.

    // Since the property is declared READ ONLY, calls to
    // SGPropertyNode::setDoubleValue are ignored.
    node->setDoubleValue(1.0);
    TS_ASSERT_EQUALS(property.GetValue(), 0.0);

    // However FGPropertyValue can be modified by altering the variable which
    // it is tied to.
    value = 1.0;
    TS_ASSERT_EQUALS(property.GetValue(), 1.0);
    // And as soon as the property is untied, the FGProperty instance can be
    // made constant again.
    pm->Untie("x");
    node->setAttribute(SGPropertyNode::WRITE, false);
    TS_ASSERT(property.IsConstant());
  }

  void testConstructorLateBound() {
    auto pm = std::make_shared<FGPropertyManager>();
    FGPropertyValue property("x", pm, nullptr);

    TS_ASSERT_EQUALS(property.IsLateBound(), true);
    TS_ASSERT_EQUALS(property.GetName(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetNameWithSign(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetFullyQualifiedName(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetPrintableName(), std::string("x"));
    TS_ASSERT_EQUALS(property.IsConstant(), false);
    TS_ASSERT_EQUALS(property.IsLateBound(), true);
    // The property manager does not contain the property "x" so GetValue()
    // should throw an exception.
    TS_ASSERT_THROWS(property.GetValue(), BaseException&);
  }

  void testInstantiateLateBound() {
    auto pm = std::make_shared<FGPropertyManager>();
    FGPropertyValue property("x", pm, nullptr);

    TS_ASSERT_EQUALS(property.IsLateBound(), true);

    auto node = pm->GetNode("x", true);
    TS_ASSERT_EQUALS(property.GetValue(), 0.0);
    TS_ASSERT_EQUALS(property.IsLateBound(), false);
    TS_ASSERT_EQUALS(property.GetName(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetNameWithSign(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetFullyQualifiedName(), std::string("/x"));
    TS_ASSERT_EQUALS(property.GetPrintableName(), std::string("x"));

    // Check the link is two-way.
    node->setDoubleValue(1.3574);
    TS_ASSERT_EQUALS(property.GetValue(), 1.3574);

    property.SetValue(-2.01);
    TS_ASSERT_EQUALS(node->getDoubleValue(), -2.01);
  }

  void testSignedProperty() {
    auto pm = std::make_shared<FGPropertyManager>();
    FGPropertyValue property("-x", pm, nullptr);

    TS_ASSERT_EQUALS(property.IsLateBound(), true);
    TS_ASSERT_EQUALS(property.GetName(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetNameWithSign(), std::string("-x"));
    TS_ASSERT_EQUALS(property.GetFullyQualifiedName(), std::string("x"));
    TS_ASSERT_EQUALS(property.GetPrintableName(), std::string("x"));
    TS_ASSERT_EQUALS(property.IsConstant(), false);
    TS_ASSERT_EQUALS(property.IsLateBound(), true);

    auto node = pm->GetNode("x", true);
    node->setDoubleValue(1.234);
    TS_ASSERT_EQUALS(property.GetValue(), -1.234);
  }
};
