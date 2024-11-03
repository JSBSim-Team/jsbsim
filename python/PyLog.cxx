/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       PyLog.cxx
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

#include <cassert>
#include "PyLog.h"

namespace JSBSim {
// These pointers are initialized in jsbsim.pyx
PyObject* FGLogger_PyClass;
PyObject* LogLevel_PyClass;
PyObject* LogFormat_PyClass;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

PyLogger::PyLogger(PyObject* logger)
{
  if (PyObject_IsInstance(logger, FGLogger_PyClass)) {
    logger_pyclass = logger;
    Py_INCREF(logger_pyclass);
  } else {
    PyErr_SetString(PyExc_TypeError, "The logger must be an instance of FGLogger");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::SetLevel(LogLevel level) {
  PyObject* py_level;

  switch (level)
  {
  case LogLevel::BULK:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "BULK");
    break;
  case LogLevel::DEBUG:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "DEBUG");
    break;
  case LogLevel::INFO:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "INFO");
    break;
  case LogLevel::WARN:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "WARN");
    break;
  case LogLevel::ERROR:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "ERROR");
    break;
  case LogLevel::FATAL:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "FATAL");
    break;
  case LogLevel::STDOUT:
    py_level = PyObject_GetAttrString(LogLevel_PyClass, "STDOUT");
    break;
  }

  PyObject* result = CallMethod1("set_level", py_level);
  if (result == Py_None) FGLogger::SetLevel(level);
  Py_DECREF(py_level);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::FileLocation(const std::string& filename, int line)
{
  PyObject* py_filename = PyUnicode_FromString(filename.c_str());
  PyObject* py_line = PyLong_FromLong(line);
  PyObject* args = PyTuple_Pack(2, py_filename, py_line);
  CallMethod("file_location", args);
  Py_DECREF(args);
  Py_DECREF(py_filename);
  Py_DECREF(py_line);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::Message(const std::string& message)
{
  PyObject* msg = PyUnicode_FromString(message.c_str());
  CallMethod1("message", msg);
  Py_DECREF(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::Format(LogFormat format)
{
  PyObject* py_format;

  switch (format)
  {
  case LogFormat::RESET:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "RESET");
    break;
  case LogFormat::RED:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "RED");
    break;
  case LogFormat::BLUE:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "BLUE");
    break;
  case LogFormat::CYAN:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "CYAN");
    break;
  case LogFormat::GREEN:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "GREEN");
    break;
  case LogFormat::DEFAULT:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "DEFAULT");
    break;
  case LogFormat::BOLD:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "BOLD");
    break;
  case LogFormat::NORMAL:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "NORMAL");
    break;
  case LogFormat::UNDERLINE_ON:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "UNDERLINE_ON");
    break;
  case LogFormat::UNDERLINE_OFF:
    py_format = PyObject_GetAttrString(LogFormat_PyClass, "UNDERLINE_OFF");
    break;
  }

  CallMethod1("format", py_format);
  Py_DECREF(py_format);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

PyObject* PyLogger::CallMethod1(const char* method_name, PyObject* arg)
{
  PyObject* args = PyTuple_Pack(1, arg);
  PyObject* result = CallMethod(method_name, args);
  Py_DECREF(args);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

PyObject* PyLogger::CallMethod(const char* method_name, PyObject* args)
{
  PyObject* method = PyObject_GetAttrString(logger_pyclass, method_name);
  assert(method); // This should not fail as the constructor has checked the type of logger_pyclass.

  PyObject* result = PyObject_CallObject(method, args);
  if (!result) PyErr_Print();
  Py_DECREF(method);

  return result;
}

} // namespace JSBSim
