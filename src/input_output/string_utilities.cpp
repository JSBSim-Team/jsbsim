/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       string_utilities.cpp
 Author:       Bertrand Coconnier
 Date started: 12/28/22
 Purpose:      Utilities to manipulate strings.

 ------------- Copyright (C) 2022 Bertrand Coconnier -------------

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
This is the place where you create output routines to dump data for perusal
later.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <charconv>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include "FGJSBBase.h"
#include "string_utilities.h"

double atof_locale_c(const std::string& input)
{
  const char* first = input.c_str();
  const char* last = first + input.size();
  double value = 0.0;

  // Skip leading whitespaces
  while (isspace(*first)) ++first;
  //Ignoring the leading '+' sign
  if (*first == '+') ++first;

  std::from_chars_result result = std::from_chars(first, last, value);

  if (result.ec == std::errc())
    return value;

  // Error management
  std::stringstream s;

  if (result.ec == std::errc::invalid_argument)
    s << "Expecting numeric attribute value, but got: " << input;
  else if (result.ec == std::errc::result_out_of_range)
    s << "This number is too large: " << input;

  std::cerr << s.str() << std::endl;
  throw JSBSim::BaseException(s.str());
}
