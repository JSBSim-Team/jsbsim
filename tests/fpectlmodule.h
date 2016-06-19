/* Python module to control floating point exceptions
 *
 * Copyright (c) 2016 Bertrand Coconnier
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>
 */

#include "Python.h"
#include <exception>
#include <string>

namespace JSBSim {
class FloatingPointException: public std::exception
{
public:
  FloatingPointException(PyObject* _pyexc, const std::string& _msg)
    : pyexc(_pyexc), msg(_msg) {}
  const char* what() const throw() { return msg.c_str(); }
  PyObject* getPyExc() const { return pyexc; }
  ~FloatingPointException() throw() {}

private:
  PyObject* pyexc;
  std::string msg;
};
}
