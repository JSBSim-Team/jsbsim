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
#include "PyLogger.h"

namespace JSBSim {
// These pointers are initialized in jsbsim.pyx
PyObject* FGLogger_PyClass;
PyObject* LogLevel_PyClass;
PyObject* LogFormat_PyClass;

void ResetLogger(void) { SetLogger(std::make_shared<FGLogConsole>()); }

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

PyLogger::PyLogger(PyObject* logger)
{
  if (PyObject_IsInstance(logger, FGLogger_PyClass)) {
    logger_pyclass = logger;
    Py_INCREF(logger);

    convert_level_enums[(int)LogLevel::BULK] = PyObject_GetAttrString(LogLevel_PyClass, "BULK");
    convert_level_enums[(int)LogLevel::DEBUG] = PyObject_GetAttrString(LogLevel_PyClass, "DEBUG");
    convert_level_enums[(int)LogLevel::INFO] = PyObject_GetAttrString(LogLevel_PyClass, "INFO");
    convert_level_enums[(int)LogLevel::WARN] = PyObject_GetAttrString(LogLevel_PyClass, "WARN");
    convert_level_enums[(int)LogLevel::ERROR] = PyObject_GetAttrString(LogLevel_PyClass, "ERROR");
    convert_level_enums[(int)LogLevel::FATAL] = PyObject_GetAttrString(LogLevel_PyClass, "FATAL");
    convert_level_enums[(int)LogLevel::STDOUT] = PyObject_GetAttrString(LogLevel_PyClass, "STDOUT");

    convert_format_enums[(int)LogFormat::RESET] = PyObject_GetAttrString(LogFormat_PyClass, "RESET");
    convert_format_enums[(int)LogFormat::RED] = PyObject_GetAttrString(LogFormat_PyClass, "RED");
    convert_format_enums[(int)LogFormat::BLUE] = PyObject_GetAttrString(LogFormat_PyClass, "BLUE");
    convert_format_enums[(int)LogFormat::CYAN] = PyObject_GetAttrString(LogFormat_PyClass, "CYAN");
    convert_format_enums[(int)LogFormat::GREEN] = PyObject_GetAttrString(LogFormat_PyClass, "GREEN");
    convert_format_enums[(int)LogFormat::DEFAULT] = PyObject_GetAttrString(LogFormat_PyClass, "DEFAULT");
    convert_format_enums[(int)LogFormat::BOLD] = PyObject_GetAttrString(LogFormat_PyClass, "BOLD");
    convert_format_enums[(int)LogFormat::NORMAL] = PyObject_GetAttrString(LogFormat_PyClass, "NORMAL");
    convert_format_enums[(int)LogFormat::UNDERLINE_ON] = PyObject_GetAttrString(LogFormat_PyClass, "UNDERLINE_ON");
    convert_format_enums[(int)LogFormat::UNDERLINE_OFF] = PyObject_GetAttrString(LogFormat_PyClass, "UNDERLINE_OFF");
  } else {
    PyErr_SetString(PyExc_TypeError, "The logger must be an instance of FGLogger");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::SetLevel(LogLevel level) {
  const int idx = static_cast<int>(level);
  assert(idx >=0 && idx <= 6);
  PyObjectPtr py_level = convert_level_enums[idx];
  PyObjectPtr result = CallPythonMethodWithArguments("set_level", py_level);
  if (result) FGLogger::SetLevel(level);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::FileLocation(const std::string& filename, int line)
{
  PyObjectPtr py_filename = PyUnicode_FromString(filename.c_str());
  PyObjectPtr py_line = PyLong_FromLong(line);
  PyObjectPtr args = PyTuple_Pack(2, py_filename.get(), py_line.get());
  CallPythonMethodWithTuple("file_location", args);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::Message(const std::string& message)
{
  PyObjectPtr msg = PyUnicode_FromString(message.c_str());
  CallPythonMethodWithArguments("message", msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void PyLogger::Format(LogFormat format)
{
  const int idx = static_cast<int>(format);
  assert(idx >= 0 && idx <= 9);
  PyObjectPtr py_format = convert_format_enums[idx];
  CallPythonMethodWithArguments("format", py_format);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

PyObjectPtr PyLogger::CallPythonMethodWithArguments(const char* method_name, const PyObjectPtr& arg)
{
  PyObjectPtr tuple = PyTuple_Pack(1, arg.get());
  return CallPythonMethodWithTuple(method_name, tuple);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

PyObjectPtr PyLogger::CallPythonMethodWithTuple(const char* method_name, const PyObjectPtr& tuple)
{
  PyObjectPtr method = PyObject_GetAttrString(logger_pyclass.get(), method_name);
  assert(method); // This should not fail as the constructor has checked the type of logger_pyclass.

  PyObjectPtr result = PyObject_CallObject(method.get(), tuple.get());

  if (!result) PyErr_Print();

  return result;
}
} // namespace JSBSim
