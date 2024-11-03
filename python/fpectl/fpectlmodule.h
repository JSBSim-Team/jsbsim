/* Python module to control floating point exceptions
 *
 * Copyright (c) 2016-2024 Bertrand Coconnier
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Further information about the GNU Lesser General Public License can also be
 * found on the world wide web at http://www.gnu.org.
 */

#include <Python.h>
#include <exception>
#include <string>

#ifndef FPCTLMODULE_H
#define FPCTLMODULE_H

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
#endif // FPCTLMODULE_H
