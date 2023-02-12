#include <string>
#include <cxxtest/TestSuite.h>
#include <input_output/string_utilities.h>
#include "FGJSBBase.h"

class StringUtilitiesTest : public CxxTest::TestSuite
{
public:
  void testTrim() {
    const std::string s_ref(" \t  xx\t\tyy  zz \t  ");
    const std::string all_spaces("  \t \t\t  ");
    std::string s = s_ref;
    TS_ASSERT_EQUALS(JSBSim::trim_left(s), std::string("xx\t\tyy  zz \t  "));
    TS_ASSERT_EQUALS(JSBSim::trim_left(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(JSBSim::trim_left(s), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(JSBSim::trim_right(s), std::string(" \t  xx\t\tyy  zz"));
    TS_ASSERT_EQUALS(JSBSim::trim_right(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(JSBSim::trim_right(s), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(JSBSim::trim(s), std::string("xx\t\tyy  zz"));
    TS_ASSERT_EQUALS(JSBSim::trim(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(JSBSim::trim(s), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(JSBSim::trim_all_space(s), std::string("xxyyzz"));
    TS_ASSERT_EQUALS(JSBSim::trim_all_space(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(JSBSim::trim_all_space(s), empty);
  }

  void testStringCase() {
    const std::string s_ref(" MiXed\tCaSE; ");
    std::string s = s_ref;
    TS_ASSERT_EQUALS(JSBSim::to_upper(s), std::string(" MIXED\tCASE; "));
    TS_ASSERT_EQUALS(JSBSim::to_upper(empty), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(JSBSim::to_lower(s), std::string(" mixed\tcase; "));
    TS_ASSERT_EQUALS(JSBSim::to_lower(empty), empty);
  }

  void testNumberString() {
    TS_ASSERT(JSBSim::is_number("1.0"));
    TS_ASSERT(JSBSim::is_number("1526"));
    TS_ASSERT(JSBSim::is_number(".01256"));
    TS_ASSERT(JSBSim::is_number("-1.0e+1"));
    TS_ASSERT(!JSBSim::is_number(empty));
    TS_ASSERT(!JSBSim::is_number("125x5#"));
  }

  void testSplit() {
    std::vector <std::string> list = JSBSim::split(empty,',');
    TS_ASSERT_EQUALS(list.size(), 0);
    list = JSBSim::split(std::string(",,,,,"),',');
    TS_ASSERT_EQUALS(list.size(), 0);
    list = JSBSim::split(std::string(" xx yy zz "),',');
    TS_ASSERT_EQUALS(list.size(), 1);
    TS_ASSERT_EQUALS(list[0], std::string("xx yy zz"));
    list = JSBSim::split(std::string(" xx yy zz "),' ');
    TS_ASSERT_EQUALS(list.size(), 3);
    TS_ASSERT_EQUALS(list[0], std::string("xx"));
    TS_ASSERT_EQUALS(list[1], std::string("yy"));
    TS_ASSERT_EQUALS(list[2], std::string("zz"));
    list = JSBSim::split(",xx,,yy,zz,",',');
    TS_ASSERT_EQUALS(list.size(), 3);
    TS_ASSERT_EQUALS(list[0], std::string("xx"));
    TS_ASSERT_EQUALS(list[1], std::string("yy"));
    TS_ASSERT_EQUALS(list[2], std::string("zz"));
  }

  void testReplace() {
    TS_ASSERT_EQUALS(JSBSim::replace(empty, std::string("x"), std::string("a")), empty);
    TS_ASSERT_EQUALS(JSBSim::replace(std::string(" xyzzu "), std::string("x"),
                             std::string("a")), std::string(" ayzzu "));
    TS_ASSERT_EQUALS(JSBSim::replace(std::string("xyzzu"), std::string("x"),
                             std::string("a")), std::string("ayzzu"));
    TS_ASSERT_EQUALS(JSBSim::replace(std::string("xyzzu"), std::string("u"),
                             std::string("a")), std::string("xyzza"));
    TS_ASSERT_EQUALS(JSBSim::replace(std::string("xyzzu"), std::string("z"),
                             std::string("y")), std::string("xyyzu"));
    TS_ASSERT_EQUALS(JSBSim::replace(std::string("xyzzu"), std::string("yzz"),
                             std::string("ab")), std::string("xabzzu"));
    TS_ASSERT_EQUALS(JSBSim::replace(std::string("xyzzu"), std::string("b"),
                             std::string("w")), std::string("xyzzu"));
  }

  void testAtofLocaleC() {
    TS_ASSERT_EQUALS(JSBSim::atof_locale_c("+1  "), 1.0);
    TS_ASSERT_EQUALS(JSBSim::atof_locale_c(" 123.4"), 123.4);
    TS_ASSERT_EQUALS(JSBSim::atof_locale_c("-3.14e-2"), -0.0314);
    TS_ASSERT_EQUALS(JSBSim::atof_locale_c("1E-999"), 0.0);
    TS_ASSERT_EQUALS(JSBSim::atof_locale_c("-1E-999"), 0.0);
    TS_ASSERT_EQUALS(JSBSim::atof_locale_c("0.0"), 0.0);
    TS_ASSERT_THROWS(JSBSim::atof_locale_c("1E+999"), JSBSim::BaseException&);
    TS_ASSERT_THROWS(JSBSim::atof_locale_c("-1E+999"), JSBSim::BaseException&);
  }

private:
  std::string empty;
};
