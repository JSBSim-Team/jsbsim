#include <memory>

#include <cxxtest/TestSuite.h>
#include <input_output/FGPropertyManager.h>

using namespace JSBSim;


class FGPropertyManagerTest : public CxxTest::TestSuite
{
public:
  void testConstructor() {
    auto pm = std::make_shared<FGPropertyManager>();
    auto root = pm->GetNode();

    TS_ASSERT_EQUALS(root->getNameString(), "");
    TS_ASSERT_EQUALS(GetFullyQualifiedName(root), "/");
  }
};
