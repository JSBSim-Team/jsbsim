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

#include <errno.h>
#ifdef __APPLE__
#include <xlocale.h>
#else
#include <locale.h>
#endif
#include <sstream>

#include "FGJSBBase.h"
#include "string_utilities.h"

#ifdef _WIN32
typedef _locale_t locale_t;
#define freelocale _free_locale
#define strtod_l _strtod_l
#endif

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
double atof_locale_c(const std::string& input)
{
  const char* first = input.c_str();

  // Skip leading whitespaces
  while (isspace(*first)) ++first;
  //Ignoring the leading '+' sign
  if (*first == '+') ++first;

  CNumericLocale numeric_c;
  errno = 0;          // Reset the error code
  double value = strtod_l(first, nullptr, numeric_c.Locale);

  // Error management
  std::stringstream s;

  if (fabs(value) == HUGE_VAL && errno == ERANGE)
    s << "This number is too large: " << input;
  else if (fabs(value) == 0 && errno == EINVAL)
    s << "Expecting numeric attribute value, but got: " << input;
  else
    return value;

  std::cerr << s.str() << std::endl;
  throw JSBSim::BaseException(s.str());
}
