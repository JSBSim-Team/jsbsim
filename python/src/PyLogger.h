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

#include <map>
#include <Python.h>
#include "input_output/FGLog.h"

#ifndef PYLOGGER_H
#define PYLOGGER_H

namespace JSBSim {
/// Pointers to the Python counterparts of C++ classes.
extern PyObject* FGLogger_PyClass;
extern PyObject* LogLevel_PyClass;
extern PyObject* LogFormat_PyClass;

/// Reset the global logger to its default.
JSBSIM_API void ResetLogger(void);

// Helper class to manage the reference count of a PyObject.
class PyObjectPtr {
public:
  PyObjectPtr(PyObject* obj = nullptr) noexcept
    : object(obj) {}

  // Copy constructor
  PyObjectPtr(const PyObjectPtr& other) noexcept {
    object = other.object;
    Py_XINCREF(object);
  }

  // Move constructor
  PyObjectPtr(PyObjectPtr&& other) noexcept
    : object(other.object)
  {
    other.object = nullptr;
  }

  // Copy assignment operator
  PyObjectPtr& operator=(const PyObjectPtr& other) noexcept {
    if (this != &other) {
      Py_XDECREF(object); // Decrement the reference count of the current object
      object = other.object;
      Py_XINCREF(object); // Increment the reference count of the new object
    }
    return *this;
  }

  PyObjectPtr& operator=(PyObject* src) noexcept {
    Py_XDECREF(object);
    object = src;
    return *this;
  }

  // Move assignment operator
  PyObjectPtr& operator=(PyObjectPtr&& other) noexcept {
    if (this != &other) {
      Py_XDECREF(object); // Decrement the reference count of the current object
      object = other.object;
      other.object = nullptr; // Prevent the source from decrementing the reference count
    }
    return *this;
  }

  ~PyObjectPtr() noexcept { Py_XDECREF(object); }

  PyObject* get() const noexcept { return object; }
  operator bool() const noexcept { return object != nullptr; }

protected:
  PyObject* object;
};


class PyLogger : public FGLogger
{
public:
  explicit PyLogger(PyObject* logger);
  void SetLevel(LogLevel level) override;
  void FileLocation(const std::string& filename, int line) override;
  void Message(const std::string& message) override;
  void Format(LogFormat format) override;
  void Flush(void) override { CallPythonMethodWithTuple("flush", nullptr); }

private:
  PyObjectPtr CallPythonMethodWithTuple(const char* method_name, const PyObjectPtr& tuple);
  PyObjectPtr CallPythonMethodWithArguments(const char* method_name, const PyObjectPtr& arg);

  PyObjectPtr logger_pyclass;
  std::map<LogLevel, PyObjectPtr> convert_level_enums;
  std::map<LogFormat, PyObjectPtr> convert_format_enums;
};
}
#endif
