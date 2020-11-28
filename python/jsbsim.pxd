# cython: language_level=2
#
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

cdef extern from "initialization/FGInitialCondition.h" namespace "JSBSim":
    cdef cppclass c_FGInitialCondition "JSBSim::FGInitialCondition":
        c_FGInitialCondition(c_FGInitialCondition* ic)
        bool Load(const c_SGPath& rstfile, bool useStoredPath)

cdef extern from "input_output/FGPropertyManager.h" namespace "JSBSim":
    cdef cppclass c_FGPropertyManager "JSBSim::FGPropertyManager":
        c_FGPropertyManager()
        bool HasNode(string path) except +convertJSBSimToPyExc

cdef extern from "math/FGColumnVector3.h" namespace "JSBSim":
    cdef cppclass c_FGColumnVector3 "JSBSim::FGColumnVector3":
        c_FGColumnVector3()
        c_FGColumnVector3(const c_FGColumnVector3& m)
        double Entry(unsigned int idx) const

cdef extern from "math/FGMatrix33.h" namespace "JSBSim":
    cdef cppclass c_FGMatrix33 "JSBSim::FGMatrix33":
        c_FGMatrix33()
        c_FGMatrix33(const c_FGMatrix33& m)
        double Entry(unsigned int row, unsigned int col) const

cdef extern from "models/FGAerodynamics.h" namespace "JSBSim":
    cdef cppclass c_FGAerodynamics "JSBSim::FGAerodynamics":
        c_FGColumnVector3& GetMomentsMRC()
        c_FGColumnVector3& GetForces()

cdef extern from "models/FGAircraft.h" namespace "JSBSim":
    cdef cppclass c_FGAircraft "JSBSim::FGAircraft":
        c_FGColumnVector3& GetXYZrp()

cdef extern from "models/FGAtmosphere.h" namespace "JSBSim":
    cdef enum eTemperature "JSBSim::FGAtmosphere::eTemperature":
        eNoTempUnit = 0,
        eFahrenheit = 1,
        eCelsius    = 2,
        eRankine    = 3,
        eKelvin     = 4

    cdef enum ePressure "JSBSim::FGAtmosphere::ePressure":
        eNoPressUnit= 0,
        ePSF        = 1,
        eMillibars  = 2,
        ePascals    = 3,
        eInchesHg   = 4

    cdef cppclass c_FGAtmosphere "JSBSim::FGAtmosphere":
        double GetTemperature(double h)
        void SetTemperature(double t, double h, eTemperature unit)
        void SetPressureSL(ePressure unit, double pressure)

cdef extern from "models/FGAuxiliary.h" namespace "JSBSim":
    cdef cppclass c_FGAuxiliary "JSBSim::FGAuxiliary":
        c_FGMatrix33& GetTw2b()
        c_FGMatrix33& GetTb2w()

cdef extern from "models/FGGroundReactions.h" namespace "JSBSim":
    cdef cppclass c_FGGroundReactions "JSBSim::FGGroundReactions":
        c_FGLGear* GetGearUnit(int gear)
        int GetNumGearUnits()

cdef extern from "models/FGLGear.h" namespace "JSBSim":
    cdef cppclass c_FGLGear "JSBSim::FGLGear":
        double GetSteerNorm()
        double GetBodyXForce()
        double GetBodyYForce()
        double GetBodyZForce()
        c_FGColumnVector3& GetLocation()
        c_FGColumnVector3& GetActingLocation()

cdef extern from "models/FGMassBalance.h" namespace "JSBSim":
    cdef cppclass c_FGMassBalance "JSBSim::FGMassBalance":
        c_FGColumnVector3& GetXYZcg()
        c_FGMatrix33& GetJ()
        c_FGMatrix33& GetJinv()

cdef extern from "models/FGPropagate.h" namespace "JSBSim":
    cdef cppclass c_FGPropagate "JSBSim::FGPropagate":
        c_FGMatrix33& GetTl2b()
        c_FGMatrix33& GetTec2b()
        c_FGColumnVector3& GetUVW()

cdef extern from "models/propulsion/FGEngine.h" namespace "JSBSim":
    cdef cppclass c_FGEngine "JSBSim::FGEngine":
        int InitRunning()

cdef extern from "models/FGPropulsion.h" namespace "JSBSim":
    cdef cppclass c_FGPropulsion "JSBSim::FGPropulsion":
        void InitRunning(int n)
        int GetNumEngines()
        c_FGEngine* GetEngine(unsigned int idx)
        bool GetSteadyState()

cdef extern from "simgear/misc/sg_path.hxx":
    cdef cppclass c_SGPath "SGPath":
        c_SGPath(const string& path, int* validator)
        c_SGPath(const c_SGPath& p)
        void set(const string& p)
        string utf8Str()

cdef extern from "FGJSBBase.h" namespace "JSBSim":
    cdef cppclass c_FGJSBBase "JSBSim::FGJSBBase":
        c_FGJSBBase()
        short debug_lvl
        string GetVersion()

cdef extern from "FGFDMExec.h" namespace "JSBSim":
    cdef cppclass c_FGFDMExec "JSBSim::FGFDMExec" (c_FGJSBBase):
        c_FGFDMExec(c_FGPropertyManager* root, unsigned int* fdmctr)
        void Unbind()
        bool Run() except +convertJSBSimToPyExc
        bool RunIC() except +convertJSBSimToPyExc
        bool LoadModel(string model,
                       bool add_model_to_path) except +convertJSBSimToPyExc
        bool LoadModel(const c_SGPath aircraft_path,
                       const c_SGPath engine_path,
                       const c_SGPath systems_path,
                       const string model,
                       bool add_model_to_path) except +convertJSBSimToPyExc
        bool LoadScript(const c_SGPath& script, double delta_t,
                        const c_SGPath& initfile) except +convertJSBSimToPyExc
        bool SetEnginePath(const c_SGPath& path)
        bool SetAircraftPath(const c_SGPath& path)
        bool SetSystemsPath(const c_SGPath& path)
        void SetRootDir(const c_SGPath& path)
        const c_SGPath& GetEnginePath()
        const c_SGPath& GetAircraftPath()
        const c_SGPath& GetSystemsPath()
        const c_SGPath& GetRootDir()
        const c_SGPath& GetFullAircraftPath()
        double GetPropertyValue(string property) except +convertJSBSimToPyExc
        void SetPropertyValue(string property, double value) except +convertJSBSimToPyExc
        string GetModelName()
        bool SetOutputDirectives(const c_SGPath& fname) except +
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
        void PrintSimulationConfiguration()
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
        c_FGGroundReactions* GetGroundReactions()
        c_FGAuxiliary* GetAuxiliary()
        c_FGAerodynamics* GetAerodynamics()
        c_FGAircraft* GetAircraft()
        c_FGAtmosphere* GetAtmosphere()
        c_FGMassBalance* GetMassBalance()
