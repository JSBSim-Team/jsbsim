/* Convert JSBSim exceptions to Python exceptions
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

#include <string>
#include "fpectlmodule.h"
#include "FGFDMExec.h"
#include "GeographicLib/Constants.hpp"
#include "math/FGMatrix33.h"
#include "math/FGTable.h"

#ifndef EXCEPTIONMANAGEMENT_H
#define EXCEPTIONMANAGEMENT_H

namespace JSBSim {
// Pointers to Python exception classes.
// Their initialization take place in jsbsim.pyx
PyObject* base_error;
PyObject* trimfailure_error;
PyObject* geographic_error;
PyObject* table_error;

void convertJSBSimToPyExc()
{
  try {
    if (!PyErr_Occurred())
      throw;
  }
  catch (const JSBSim::TrimFailureException& e) {
    PyErr_SetString(trimfailure_error, e.what());
  }
  catch (const GeographicLib::GeographicErr& e) {
    PyErr_SetString(geographic_error, e.what());
  }
  catch (const JSBSim::BaseException& e) {
    PyErr_SetString(base_error, e.what());
  }
  catch (const JSBSim::FloatingPointException& e) {
    PyErr_SetString(e.getPyExc(), e.what());
  }
  catch (const std::string &msg) {
    PyErr_SetString(PyExc_RuntimeError, msg.c_str());
  }
  catch (const char* msg) {
    PyErr_SetString(PyExc_RuntimeError, msg);
  }
}
}
#endif // EXCEPTIONMANAGEMENT_H
