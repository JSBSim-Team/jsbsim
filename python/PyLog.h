/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       PyLog.h
 Author:       Bertrand Coconnier
 Date started: 11/02/24

 ------------- Copyright (C) 2024 Bertrand Coconnier -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.
*/

#include <Python.h>
#include "input_output/FGLog.h"

#ifndef PYLOG_H
#define PYLOG_H

namespace JSBSim {
/// Pointers to the Python counterparts of C++ classes.
extern PyObject* FGLogger_PyClass;
extern PyObject* LogLevel_PyClass;
extern PyObject* LogFormat_PyClass;

class PyLogger : public FGLogger
{
public:
  explicit PyLogger(PyObject* logger);
  ~PyLogger() override { Py_XDECREF(logger_pyclass); }
  void SetLevel(LogLevel level) override;
  void FileLocation(const std::string& filename, int line) override;
  void Message(const std::string& message) override;
  void Format(LogFormat format) override;
  void Flush(void) override { CallMethod("flush", nullptr); }

private:
  PyObject* CallMethod(const char* method_name, PyObject* args);
  PyObject* CallMethod1(const char* method_name, PyObject* arg);

  PyObject* logger_pyclass = nullptr;
};
}
#endif
