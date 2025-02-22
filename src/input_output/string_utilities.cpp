/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       string_utilities.cpp
 Author:       Bertrand Coconnier / Sean McLeod
 Date started: 12/28/22
 Purpose:      Utilities to manipulate strings.

 ------------ Copyright (C) 2022 Bertrand Coconnier, Sean McLeod,  -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
String handling conveniences such as trim, is_number, split, etc.; these new
capabilities have been incorporated into the source code where the
string::find() functions were formerly used.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef __CYGWIN__
#define _GNU_SOURCE 1
#endif
#include <errno.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <regex>
#ifdef __APPLE__
#include <xlocale.h>
#else
#include <locale.h>
#endif

#include "FGJSBBase.h"
#include "string_utilities.h"

#ifdef _WIN32
typedef _locale_t locale_t;
#define freelocale _free_locale
#define strtod_l _strtod_l
#endif

using namespace std;

namespace JSBSim {
struct CNumericLocale
{
  CNumericLocale()
  {
#ifdef _WIN32
    Locale = _create_locale(LC_NUMERIC, "C");
#else
    Locale = newlocale(LC_NUMERIC_MASK, "C", 0);
#endif
  }

  ~CNumericLocale()
  {
    freelocale(Locale);
  }

  locale_t Locale;
};

/* A locale independent version of atof().
 * Whatever is the current locale of the application, atof_locale_c() reads
 * numbers assuming that the decimal point is the period (.)
 */
double atof_locale_c(const string& input)
{
  static const std::regex number_format(R"(^\s*[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?\s*$)");
  const char* first = input.c_str();

  // Skip leading whitespaces
  while (isspace(*first)) ++first;

  if (!*first)
    throw InvalidNumber("Expecting a numeric attribute value, but only got spaces");

  if (!std::regex_match(input, number_format))
    throw InvalidNumber("Expecting a numeric attribute value, but got: " + input);

  CNumericLocale numeric_c;
  errno = 0;          // Reset the error code
  double value = strtod_l(first, nullptr, numeric_c.Locale);

  // Error management
  std::stringstream s;

  if (fabs(value) == HUGE_VAL && errno == ERANGE)
    s << "This number is too large: " << input;
  else if (fabs(value) == 0 && errno == EINVAL)
    s << "Expecting a numeric attribute value, but got: " << input;
  else
    return value;

  throw InvalidNumber(s.str());
}


std::string& trim_left(std::string& str)
{
  while (!str.empty() && isspace((unsigned char)str[0])) {
    str = str.erase(0,1);
  }
  return str;
}

std::string& trim_right(std::string& str)
{
  while (!str.empty() && isspace((unsigned char)str[str.size()-1])) {
    str = str.erase(str.size()-1,1);
  }
  return str;
}

std::string& trim(std::string& str)
{
  if (str.empty()) return str;
  std::string temp_str = trim_right(str);
  return str = trim_left(temp_str);
}

std::string& trim_all_space(std::string& str)
{
  for (size_t i=0; i<str.size(); i++) {
    if (isspace((unsigned char)str[i])) {
      str = str.erase(i,1);
      --i;
    }
  }
  return str;
}

std::string& to_upper(std::string& str)
{
  for (size_t i=0; i<str.size(); i++) str[i] = toupper(str[i]);
  return str;
}

std::string& to_lower(std::string& str)
{
  for (size_t i=0; i<str.size(); i++) str[i] = tolower(str[i]);
  return str;
}

bool is_number(const std::string& str)
{
  try {
    atof_locale_c(str);
  } catch (InvalidNumber&) {
    return false;
  }

  return true;
}

std::vector <std::string> split(std::string str, char d)
{
  std::vector <std::string> str_array;
  size_t index=0;
  std::string temp = "";

  trim(str);
  index = str.find(d);
  while (index != std::string::npos) {
    temp = str.substr(0,index);
    trim(temp);
    if (!temp.empty()) str_array.push_back(temp);
    str = str.erase(0,index+1);
    index = str.find(d);
  }
  if (!str.empty()) {
    temp = trim(str);
    if (!temp.empty()) str_array.push_back(temp);
  }

  return str_array;
}

std::string replace(std::string str, const std::string& oldstr, const std::string& newstr)
{
  std::string temp = str;
  size_t old_idx = str.find(oldstr);
  if (old_idx != std::string::npos) {
    temp = str.replace(old_idx, 1, newstr);
  }
  return temp;
}
};
