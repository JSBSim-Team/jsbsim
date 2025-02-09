#include <string>
#include <cxxtest/TestSuite.h>
#include "FGJSBBase.h"
#include <input_output/string_utilities.h>

using namespace JSBSim;

class StringUtilitiesTest : public CxxTest::TestSuite
{
public:
  void testTrim() {
    const std::string s_ref(" \t  xx\t\tyy  zz \t  ");
    const std::string all_spaces("  \t \t\t  ");
    std::string s = s_ref;
    TS_ASSERT_EQUALS(trim_left(s), std::string("xx\t\tyy  zz \t  "));
    TS_ASSERT_EQUALS(trim_left(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(trim_left(s), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(trim_right(s), std::string(" \t  xx\t\tyy  zz"));
    TS_ASSERT_EQUALS(trim_right(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(trim_right(s), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(trim(s), std::string("xx\t\tyy  zz"));
    TS_ASSERT_EQUALS(trim(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(trim(s), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(trim_all_space(s), std::string("xxyyzz"));
    TS_ASSERT_EQUALS(trim_all_space(empty), empty);
    s = all_spaces;
    TS_ASSERT_EQUALS(trim_all_space(s), empty);
  }

  void testStringCase() {
    const std::string s_ref(" MiXed\tCaSE; ");
    std::string s = s_ref;
    TS_ASSERT_EQUALS(to_upper(s), std::string(" MIXED\tCASE; "));
    TS_ASSERT_EQUALS(to_upper(empty), empty);
    s = s_ref;
    TS_ASSERT_EQUALS(to_lower(s), std::string(" mixed\tcase; "));
    TS_ASSERT_EQUALS(to_lower(empty), empty);
  }

  void testNumberString() {
    TS_ASSERT(is_number("1.0"));
    TS_ASSERT(is_number("1526"));
    TS_ASSERT(is_number(".01256"));
    TS_ASSERT(is_number("-1.0e+1"));
    TS_ASSERT(!is_number(empty));
    TS_ASSERT(!is_number("125x5#"));
    TS_ASSERT(!is_number("x"));
    TS_ASSERT(!is_number("1.0.0"));
    TS_ASSERT(!is_number("1.0e"));
    TS_ASSERT(!is_number("1.0e+"));
    TS_ASSERT(!is_number("1.0e1.0"));
    TS_ASSERT(!is_number("--1"));
    TS_ASSERT(!is_number("++1"));
    TS_ASSERT(!is_number("1+"));
    TS_ASSERT(!is_number("1-"));
    TS_ASSERT(!is_number(""));
    TS_ASSERT(!is_number(" "));
    TS_ASSERT(!is_number("3.14a"));
    TS_ASSERT(!is_number("-.1e-"));
  }

  void testSplit() {
    std::vector <std::string> list = split(empty,',');
    TS_ASSERT_EQUALS(list.size(), 0);
    list = split(std::string(",,,,,"),',');
    TS_ASSERT_EQUALS(list.size(), 0);
    list = split(std::string(" xx yy zz "),',');
    TS_ASSERT_EQUALS(list.size(), 1);
    TS_ASSERT_EQUALS(list[0], std::string("xx yy zz"));
    list = split(std::string(" xx yy zz "),' ');
    TS_ASSERT_EQUALS(list.size(), 3);
    TS_ASSERT_EQUALS(list[0], std::string("xx"));
    TS_ASSERT_EQUALS(list[1], std::string("yy"));
    TS_ASSERT_EQUALS(list[2], std::string("zz"));
    list = split(",xx,,yy,zz,",',');
    TS_ASSERT_EQUALS(list.size(), 3);
    TS_ASSERT_EQUALS(list[0], std::string("xx"));
    TS_ASSERT_EQUALS(list[1], std::string("yy"));
    TS_ASSERT_EQUALS(list[2], std::string("zz"));
  }

  void testReplace() {
    TS_ASSERT_EQUALS(replace(empty, std::string("x"), std::string("a")), empty);
    TS_ASSERT_EQUALS(replace(std::string(" xyzzu "), std::string("x"),
                             std::string("a")), std::string(" ayzzu "));
    TS_ASSERT_EQUALS(replace(std::string("xyzzu"), std::string("x"),
                             std::string("a")), std::string("ayzzu"));
    TS_ASSERT_EQUALS(replace(std::string("xyzzu"), std::string("u"),
                             std::string("a")), std::string("xyzza"));
    TS_ASSERT_EQUALS(replace(std::string("xyzzu"), std::string("z"),
                             std::string("y")), std::string("xyyzu"));
    TS_ASSERT_EQUALS(replace(std::string("xyzzu"), std::string("yzz"),
                             std::string("ab")), std::string("xabzzu"));
    TS_ASSERT_EQUALS(replace(std::string("xyzzu"), std::string("b"),
                             std::string("w")), std::string("xyzzu"));
  }

  void testAtofLocaleC() {
    // Test legal numbers
    TS_ASSERT_EQUALS(atof_locale_c("0.0"), 0.0);
    TS_ASSERT_EQUALS(atof_locale_c("+1"), 1.0);
    TS_ASSERT_EQUALS(atof_locale_c("1"), 1.0);
    TS_ASSERT_EQUALS(atof_locale_c("-1"), -1.0);
    TS_ASSERT_EQUALS(atof_locale_c(" 123.4"), 123.4);
    TS_ASSERT_EQUALS(atof_locale_c("123.4 "), 123.4);
    TS_ASSERT_EQUALS(atof_locale_c(" 123.4 "), 123.4);
    TS_ASSERT_EQUALS(atof_locale_c(".25"), 0.25);
    TS_ASSERT_EQUALS(atof_locale_c("1.e1"), 10.);
    TS_ASSERT_EQUALS(atof_locale_c("1e1"), 10.);
    TS_ASSERT_EQUALS(atof_locale_c(".1e1"), 1.);
    TS_ASSERT_EQUALS(atof_locale_c("+.1e1"), 1.);
    TS_ASSERT_EQUALS(atof_locale_c("-.1e1"), -1.);
    TS_ASSERT_EQUALS(atof_locale_c("31.4e1"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("+3.14e+2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("+3.14e2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("+3.14e-2"), 0.0314);
    TS_ASSERT_EQUALS(atof_locale_c("3.14e+2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("3.14e2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("3.14e-2"), 0.0314);
    TS_ASSERT_EQUALS(atof_locale_c("-3.14e+2"), -314);
    TS_ASSERT_EQUALS(atof_locale_c("-3.14e2"), -314);
    TS_ASSERT_EQUALS(atof_locale_c("-3.14e-2"), -0.0314);
    TS_ASSERT_EQUALS(atof_locale_c("+3.14E+2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("+3.14E2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("+3.14E-2"), 0.0314);
    TS_ASSERT_EQUALS(atof_locale_c("3.14E+2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("3.14E2"), 314);
    TS_ASSERT_EQUALS(atof_locale_c("3.14E-2"), 0.0314);
    TS_ASSERT_EQUALS(atof_locale_c("-3.14E+2"), -314);
    TS_ASSERT_EQUALS(atof_locale_c("-3.14E2"), -314);
    TS_ASSERT_EQUALS(atof_locale_c("-3.14E-2"), -0.0314);
    // Test rounded down numbers
    TS_ASSERT_EQUALS(atof_locale_c("1E-999"), 0.0);
    TS_ASSERT_EQUALS(atof_locale_c("-1E-999"), 0.0);
    // Test invalid numbers
    TS_ASSERT_THROWS(atof_locale_c("1E+999"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("-1E+999"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("invalid"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("1.0.0"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("1E-"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("E-2"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("."), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c(".E"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c(".E2"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c(".E-2"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("1.2E"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("1.2E+"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("1.2E1.0"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("--1"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c("++1"), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c(""), InvalidNumber&);
    TS_ASSERT_THROWS(atof_locale_c(" "), InvalidNumber&);
  }

private:
  std::string empty;
};
