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

void convertJSBSimToPyExc()
{
  try {
    if (!PyErr_Occurred())
      throw;
  }
  catch (const std::string &msg) {
    PyErr_SetString(PyExc_RuntimeError, msg.c_str());
  }
  catch (const char* msg) {
    PyErr_SetString(PyExc_RuntimeError, msg);
  }
  catch (const JSBSim::FloatingPointException& e) {
    PyErr_SetString(e.getPyExc(), e.what());
  }
}
