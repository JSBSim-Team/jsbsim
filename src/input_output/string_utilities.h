/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       string_utilities.h
 Author:       Jon S. Berndt
 Date started: 06/01/09

 ------------- Copyright (C) 2009  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
06/01/09  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <vector>
#include <stdio.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STRINGUTILS "$Id: string_utilities.h,v 1.14 2010/08/21 17:13:47 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#if !defined(BASE)
  extern std::string& trim_left(std::string& str);
  extern std::string& trim_right(std::string& str);
  extern std::string& trim(std::string& str);
  extern std::string& trim_all_space(std::string& str);
  extern std::string& to_upper(std::string& str);
  extern std::string& to_lower(std::string& str);
  extern bool is_number(const std::string& str);
  std::vector <std::string> split(std::string str, char d);
  extern std::string to_string(int);
  extern std::string replace(std::string str, const std::string& old, const std::string& newstr);
#else
  #include <cctype>

  using namespace std;

  string& trim_left(string& str)
  {
    while (str.size() && isspace((unsigned char)str[0])) {
      str = str.erase(0,1);
    }
    return str;
  }

  string& trim_right(string& str)
  {
    while (str.size() && isspace((unsigned char)str[str.size()-1])) {
      str = str.erase(str.size()-1,1);
    }
    return str;
  }

  string& trim(string& str)
  {
    if (str.size() == 0) return str;
    string temp_str = trim_right(str);
    return str = trim_left(temp_str);
  }

  string& trim_all_space(string& str)
  {
    for (size_t i=0; i<str.size(); i++) {
      if (isspace((unsigned char)str[i])) {
        str = str.erase(i,1);
        --i;
      }
    }
    return str;
  }

  string& to_upper(string& str)
  {
    for (size_t i=0; i<str.size(); i++) str[i] = toupper(str[i]);
    return str;
  }

  string& to_lower(string& str)
  {
    for (size_t i=0; i<str.size(); i++) str[i] = tolower(str[i]);
    return str;
  }

  bool is_number(const string& str)
  {
    return (str.find_first_not_of("+-.0123456789Ee") == string::npos);
  }

  vector <string> split(string str, char d)
  {
    vector <string> str_array;
    size_t index=0;
    string temp = "";

    trim(str);
    index = str.find(d);
    while (index != string::npos) {
      temp = str.substr(0,index);
      trim(temp);
      if (temp.size() > 0) str_array.push_back(temp);
      str = str.erase(0,index+1);
      index = str.find(d);
    }
    if (str.size() > 0) {
      temp = trim(str);
      if (temp.size() > 0) str_array.push_back(temp);
    }

    return str_array;
  }

  string to_string(int i)
  {
    char buffer[32];
    sprintf(buffer, "%d", i);
    return string(buffer);
  }

  string replace(string str, const string& oldstr, const string& newstr)
  {
    int old_idx;
    string temp;
    old_idx = str.find(oldstr);
    if (old_idx >= 0) {
      temp = str.replace(old_idx, 1, newstr);
    }
    return temp;
  }

#endif

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif

