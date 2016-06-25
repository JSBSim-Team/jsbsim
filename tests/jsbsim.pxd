# PyJSBSim a JSBSim python interface using cython.
#
# Copyright (c) 2013 James Goppert
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>

from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "ExceptionManagement.h":
    cdef void convertJSBSimToPyExc()

cdef extern from "models/FGPropulsion.h" namespace "JSBSim":
    cdef cppclass c_FGPropulsion "JSBSim::FGPropulsion":
        c_FGPropulsion(c_FGFDMExec* fdm)
        void InitRunning(int n)
        int GetNumEngines()

cdef extern from "initialization/FGInitialCondition.h" namespace "JSBSim":
    cdef cppclass c_FGInitialCondition "JSBSim::FGInitialCondition":
        c_FGInitialCondition(c_FGFDMExec* fdm)
        c_FGInitialCondition(c_FGInitialCondition* ic)
        bool Load(string rstfile, bool useStoredPath)

cdef extern from "math/FGMatrix33.h" namespace "JSBSim":
    cdef cppclass c_FGMatrix33 "JSBSim::FGMatrix33":
        c_FGMatrix33()
        c_FGMatrix33(const c_FGMatrix33& m)
        double Entry(unsigned int row, unsigned int col) const

cdef extern from "math/FGColumnVector3.h" namespace "JSBSim":
    cdef cppclass c_FGColumnVector3 "JSBSim::FGColumnVector3":
        c_FGColumnVector3()
        c_FGColumnVector3(const c_FGColumnVector3& m)
        double Entry(unsigned int idx) const

cdef extern from "models/FGPropagate.h" namespace "JSBSim":
    cdef cppclass c_FGPropagate "JSBSim::FGPropagate":
        c_FGPropagate(c_FGFDMExec* fdm)
        c_FGMatrix33& GetTl2b()
        c_FGMatrix33& GetTec2b()
        c_FGColumnVector3& GetUVW()

cdef extern from "input_output/FGPropertyManager.h" namespace "JSBSim":
    cdef cppclass c_FGPropertyManager "JSBSim::FGPropertyManager":
        c_FGPropertyManager()
        bool HasNode(string path)

cdef extern from "FGFDMExec.h" namespace "JSBSim":
    cdef cppclass c_FGFDMExec "JSBSim::FGFDMExec":
        c_FGFDMExec(int root, int fdmctr)
        void Unbind()
        bool Run() except +convertJSBSimToPyExc
        bool RunIC() except +convertJSBSimToPyExc
        bool LoadModel(string model,
                       bool add_model_to_path)
        bool LoadModel(string aircraft_path,
                       string engine_path,
                       string systems_path,
                       string model,
                       bool add_model_to_path)
        bool LoadScript(string script, double delta_t, string initfile) except +convertJSBSimToPyExc
        bool SetEnginePath(string path)
        bool SetAircraftPath(string path)
        bool SetSystemsPath(string path)
        void SetRootDir(string path)
        string GetEnginePath()
        string GetAircraftPath()
        string GetSystemsPath()
        string GetRootDir()
        string GetFullAircraftPath()
        double GetPropertyValue(string property)
        void SetPropertyValue(string property, double value) except +convertJSBSimToPyExc
        string GetModelName()
        bool SetOutputDirectives(string fname) except +
        #void ForceOutput(int idx=0)
        void SetLoggingRate(double rate)
        bool SetOutputFileName(int n, string fname)
        string GetOutputFileName(int n)
        void DoTrim(int mode) except +
        void DisableOutput()
        void EnableOutput()
        void Hold()
        void EnableIncrementThenHold(int time_steps)
        void CheckIncrementalHold()
        void Resume()
        bool Holding()
        void ResetToInitialConditions(int mode)
        void SetDebugLevel(int level)
        string QueryPropertyCatalog(string check)
        void PrintPropertyCatalog()
        void SetTrimStatus(bool status)
        bool GetTrimStatus()
        string GetPropulsionTankReport()
        double GetSimTime()
        double GetDeltaT()
        void SuspendIntegration()
        void ResumeIntegration()
        bool IntegrationSuspended()
        bool Setsim_time(double cur_time)
        void Setdt(double delta_t)
        double IncrTime()
        int GetDebugLevel()
        c_FGPropulsion* GetPropulsion()
        c_FGInitialCondition* GetIC()
        c_FGPropagate* GetPropagate()
        c_FGPropertyManager* GetPropertyManager()
