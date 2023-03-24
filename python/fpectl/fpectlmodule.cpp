/*
  ---------------------------------------------------------------------
  /                       Copyright (c) 1996.                           \
  |          The Regents of the University of California.                 |
  |                        All rights reserved.                           |
  |                                                                       |
  |   Permission to use, copy, modify, and distribute this software for   |
  |   any purpose without fee is hereby granted, provided that this en-   |
  |   tire notice is included in all copies of any software which is or   |
  |   includes  a  copy  or  modification  of  this software and in all   |
  |   copies of the supporting documentation for such software.           |
  |                                                                       |
  |   This  work was produced at the University of California, Lawrence   |
  |   Livermore National Laboratory under  contract  no.  W-7405-ENG-48   |
  |   between  the  U.S.  Department  of  Energy and The Regents of the   |
  |   University of California for the operation of UC LLNL.              |
  |                                                                       |
  |                              DISCLAIMER                               |
  |                                                                       |
  |   This  software was prepared as an account of work sponsored by an   |
  |   agency of the United States Government. Neither the United States   |
  |   Government  nor the University of California nor any of their em-   |
  |   ployees, makes any warranty, express or implied, or  assumes  any   |
  |   liability  or  responsibility  for the accuracy, completeness, or   |
  |   usefulness of any information,  apparatus,  product,  or  process   |
  |   disclosed,   or  represents  that  its  use  would  not  infringe   |
  |   privately-owned rights. Reference herein to any specific  commer-   |
  |   cial  products,  process,  or  service  by trade name, trademark,   |
  |   manufacturer, or otherwise, does not  necessarily  constitute  or   |
  |   imply  its endorsement, recommendation, or favoring by the United   |
  |   States Government or the University of California. The views  and   |
  |   opinions  of authors expressed herein do not necessarily state or   |
  |   reflect those of the United States Government or  the  University   |
  |   of  California,  and shall not be used for advertising or product   |
  \  endorsement purposes.                                              /
  ---------------------------------------------------------------------
*/

/*
  Floating point exception control module.

  This Python module provides bare-bones control over floating point
  units from several hardware manufacturers. Specifically, it allows
  the user to turn on the generation of SIGFPE whenever any of the
  three serious IEEE 754 exceptions (Division by Zero, Overflow,
  Invalid Operation) occurs. We currently ignore Underflow and
  Inexact Result exceptions, although those could certainly be added
  if desired.

  The module also establishes a signal handler for SIGFPE during
  initialization. This is an adaptation of the fpectl module
  (https://docs.python.org/2/library/fpectl.html) which code can be
  found in the Python distribution at Module/fpectlmodule.c

  The module has been adapted to modern OS APIs and simplified by the
  use of the C++ exception mechanism. The Python API remains the
  same.

  This module is only useful to you if it happens to include code
  specific for your hardware and software environment. If you can
  contribute OS-specific code for new platforms, or corrections for
  the code provided, it will be greatly appreciated.

  ** Version 1.0: September 20, 1996.  Lee Busby, LLNL.
  ** JSBSim adaptation: June 18, 2016. Bertrand Coconnier
  ** Added the display of stack trace: July 11, 2021. Bertrand Coconnier
  */

#ifdef LIBBFD_FOUND
#define BACKWARD_HAS_BFD 1
#endif
#ifdef BACKWARD_FOUND
#include "backward.hpp"
#endif
#include "fpectlmodule.h"

#include <signal.h>
#include <iostream>

#if defined(_MSC_VER)
#  include <float.h>
static unsigned int fp_flags = 0;
#elif (defined(__GNUC__) || defined(__clang__)) && !defined(sgi)
#  include <fenv.h>
static int fp_flags = 0;
#endif

static PyOS_sighandler_t handler = 0;

static PyObject *fpe_error;
PyMODINIT_FUNC initfpectl(void);
static PyObject *turnon_sigfpe(PyObject *self,PyObject *args);
static PyObject *turnoff_sigfpe(PyObject *self,PyObject *args);
static PyObject *test_sigfpe(PyObject *self,PyObject *args);

static PyMethodDef fpectl_methods[] = {
  {"turnon_sigfpe", (PyCFunction) turnon_sigfpe, METH_VARARGS},
  {"turnoff_sigfpe", (PyCFunction) turnoff_sigfpe, METH_VARARGS},
  {"test_sigfpe", (PyCFunction) test_sigfpe, METH_VARARGS},
  {0,0}
};

// Since the Backward-cpp signal handler for Windows calls abort(), we need to
// intercept SIGABRT and throw an exception instead. This avoids an ungraceful
// abortion of the Python interpreter.
static void finalize_signal_handling(int signo)
{
  throw JSBSim::FloatingPointException(fpe_error,
                                       "Caught signal SIGFPE in JSBSim");
}

#ifdef BACKWARD_FOUND
#ifdef __GNUC__
// Setup the Backward-cpp signal handler for FPE only.
static backward::SignalHandling sh({SIGFPE,});
static struct sigaction backward_action;
auto _dummy = sigaction(SIGFPE, nullptr, &backward_action);
// Replaces the default signal handler of Backward-cpp by our own: it uses
// backward::SignalHandling::handleSignal() to display the stack trace then
// throw a Python exception instead of calling exit() - which is the default
// behavior of Backward-cpp.
static void sigfpe_handler(int signo, siginfo_t *info, void *_ctx)
{
  sh.handleSignal(signo, info, _ctx);
  finalize_signal_handling(signo);
}
#elif defined(_MSC_VER)
static backward::SignalHandling sh;
// Here, we replace the SIGABRT signal handler by our own. We also get a copy of
// the Backward-cpp default signal handler in the process.
static PyOS_sighandler_t sigfpe_handler = PyOS_setsig(SIGABRT, finalize_signal_handling);
#endif // _MSC_VER
#else
// Our default signal handler. It is used when Backward-cpp is not installed.
static void sigfpe_handler(int signo)
{
  PyOS_setsig(SIGFPE, sigfpe_handler);
  finalize_signal_handling(signo);
}
#endif // BACKWARD_FOUND

static PyObject *turnon_sigfpe(PyObject *self, PyObject *args)
{
#ifdef _MSC_VER
  _clearfp();
  fp_flags = _controlfp(_controlfp(0, 0) & ~(_EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW),
                        _MCW_EM);
  handler = PyOS_setsig(SIGFPE, sigfpe_handler);
#elif defined(__clang__)
  fp_flags = feraiseexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

#elif defined(__GNUC__) && !defined(sgi)
  fp_flags = feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#ifdef BACKWARD_FOUND
  handler = PyOS_setsig(SIGFPE, nullptr);
  backward_action.sa_sigaction = &sigfpe_handler;
  sigaction(SIGFPE, &backward_action, nullptr);
#else
  handler = PyOS_setsig(SIGFPE, sigfpe_handler);
#endif
#endif

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *turnoff_sigfpe(PyObject *self, PyObject *args)
{
#ifdef _MSC_VER
  _controlfp(fp_flags, _MCW_EM);

#elif defined(__clang__)
  feraiseexcept(fp_flags);

#elif defined(__GNUC__) && !defined(sgi)
  fedisableexcept(fp_flags);
#ifdef BACKWARD_FOUND
  sigaction(SIGFPE, nullptr, nullptr);
#endif
#endif

  PyOS_setsig(SIGFPE, handler);
  Py_INCREF (Py_None);
  return Py_None;
}

struct module_state {
    PyObject *error;
};

static struct PyModuleDef fpectl = {
        PyModuleDef_HEAD_INIT,
        "fpectl",
        NULL,
        sizeof(struct module_state),
        fpectl_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_fpectl(void)
{
  PyObject *m = PyModule_Create(&fpectl);

  if (m == NULL)
    return NULL;

  PyObject *d = PyModule_GetDict(m);
  fpe_error = PyErr_NewException((char*)"fpectl.FloatingPointError",
                                 PyExc_FloatingPointError, NULL);
  if (fpe_error != NULL)
    PyDict_SetItemString(d, "FloatingPointError", fpe_error);

  return m;
}

static PyObject *test_sigfpe(PyObject *self, PyObject *args)
{
  try {
    std::cout << "Executing faulty FP calculation..." << std::endl;
    double x = sqrt(-1.0);
    return PyFloat_FromDouble(x);
  }
  catch(const JSBSim::FloatingPointException &e) {
    PyErr_SetString(fpe_error, e.what());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}
