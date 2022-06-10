#include <string>
#include <cxxtest/TestSuite.h>
#include <input_output/string_utilities.h>

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

private:
  std::string empty;
};
