/* Convert JSBSim exceptions to Python exceptions
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

#include <string>
#include "fpectl/fpectlmodule.h"
#include "FGFDMExec.h"
#include "GeographicLib/Constants.hpp"
#include "math/FGMatrix33.h"
#include "math/FGTable.h"

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
