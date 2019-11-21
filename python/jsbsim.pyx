# PyJSBSim a JSBSim python interface using cython.
#
# Copyright (c) 2013 James Goppert
# Copyright (c) 2014-2019 Bertrand Coconnier
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>

"""An Open source flight dynamics & control software library

   @DoxMainPage"""

import os, platform, numpy

__version__='${PROJECT_VERSION}'

cdef convertToNumpyMat(const c_FGMatrix33& m):
    return numpy.mat([[m.Entry(1, 1), m.Entry(1, 2), m.Entry(1, 3)],
                      [m.Entry(2, 1), m.Entry(2, 2), m.Entry(2, 3)],
                      [m.Entry(3, 1), m.Entry(3, 2), m.Entry(3, 3)]])


cdef convertToNumpyVec(const c_FGColumnVector3& v):
    return numpy.mat([v.Entry(1), v.Entry(2), v.Entry(3)]).T

cdef class FGPropagate:
    """@Dox(JSBSim::FGPropagate)"""

    cdef c_FGPropagate *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_Tl2b(self):
        """@Dox(JSBSim::FGPropagate::GetTl2b)"""
        return convertToNumpyMat(self.thisptr.GetTl2b())

    def get_Tec2b(self):
        """@Dox(JSBSim::FGPropagate::GetTec2b)"""
        return convertToNumpyMat(self.thisptr.GetTec2b())

    def get_uvw(self):
        """@Dox(JSBSim::FGPropagate::GetUVW)"""
        return convertToNumpyVec(self.thisptr.GetUVW())

cdef class FGPropertyManager:
    """@Dox(JSBSim::FGPropertyManager)"""

    cdef c_FGPropertyManager *thisptr
    cdef bool thisptr_owner

    def __cinit__(self, new_instance=False):
        if new_instance:
            self.thisptr = new c_FGPropertyManager()
            if self.thisptr is NULL:
                raise MemoryError()
            self.thisptr_owner = True
        else:
            self.thisptr = NULL
            self.thisptr_owner = False

    def __dealloc__(self):
        if self.thisptr is not NULL and self.thisptr_owner:
            del self.thisptr
            self.thisptr = NULL
            self.thisptr_owner = False

    def hasNode(self, path):
        """@Dox(JSBSim::FGPropertyManager::HasNode)"""
        return self.thisptr.HasNode(path.encode())

cdef class FGGroundReactions:
    """@Dox(JSBSim::FGGroundReactions)"""

    cdef c_FGGroundReactions *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_gear_unit(self, gear):
        """@Dox(JSBSim::FGGroundReactions::GetGearUnit)"""
        lgear = FGLGear()
        lgear.thisptr = self.thisptr.GetGearUnit(gear)
        return lgear

    def get_num_gear_units(self):
        """@Dox(JSBSim::FGGroundReactions::GetNumGearUnits)"""
        return self.thisptr.GetNumGearUnits()

cdef class FGLGear:
    """@Dox(JSBSim::FGLGear)"""

    cdef c_FGLGear *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_steer_norm(self):
        """@Dox(JSBSim::FGLGear::GetSteerNorm)"""
        return self.thisptr.GetSteerNorm()

    def get_body_x_force(self):
        """@Dox(JSBSim::FGLGear::GetBodyXForce)"""
        return self.thisptr.GetBodyXForce()

    def get_body_y_force(self):
        """@Dox(JSBSim::FGLGear::GetBodyYForce)"""
        return self.thisptr.GetBodyYForce()

    def get_body_z_force(self):
        """@Dox(JSBSim::FGLGear::GetBodyZForce)"""
        return self.thisptr.GetBodyZForce()

    def get_location(self):
        return convertToNumpyVec(self.thisptr.GetLocation())

    def get_acting_location(self):
        return convertToNumpyVec(self.thisptr.GetActingLocation())

cdef class FGAuxiliary:
    """@Dox(JSBSim::FGAuxiliary)"""

    cdef c_FGAuxiliary *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_Tw2b(self):
        """@Dox(JSBSim::FGAuxiliary::GetTw2b)"""
        return convertToNumpyMat(self.thisptr.GetTw2b())

    def get_Tb2w(self):
        """@Dox(JSBSim::FGAuxiliary::GetTb2w)"""
        return convertToNumpyMat(self.thisptr.GetTb2w())

cdef class FGAerodynamics:
    """@Dox(JSBSim::FGAerodynamics)"""

    cdef c_FGAerodynamics *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_moments_MRC(self):
        """@Dox(JSBSim::FGAerodynamics::GetMomentsMRC)"""
        return convertToNumpyVec(self.thisptr.GetMomentsMRC())

    def get_forces(self):
        """@Dox(JSBSim::FGAerodynamics::GetForces)"""
        return convertToNumpyVec(self.thisptr.GetForces())

cdef class FGAircraft:
    """@Dox(JSBSim::FGAircraft)"""

    cdef c_FGAircraft *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_xyz_rp(self):
        """@Dox(JSBSim::FGAircraft::GetXYZrp)"""
        return convertToNumpyVec(self.thisptr.GetXYZrp())

cdef class FGAtmosphere:
    """@Dox(JSBSim::FGAtmosphere)"""

    cdef c_FGAtmosphere *thisptr

    def __init__(self):
        self.thisptr = NULL

    def set_temperature(self, t, h, unit):
        """@Dox(JSBSim::FGAtmosphere::SetTemperature)"""
        return self.thisptr.SetTemperature(t, h, unit)

    def get_temperature(self, h):
        """@Dox(JSBSim::FGAtmosphere::GetTemperature(double))"""
        return self.thisptr.GetTemperature(h)

    def set_pressure_SL(self, unit, p):
        """@Dox(JSBSim::FGAtmosphere::SetPressureSL)"""
        return self.thisptr.SetPressureSL(unit, p)

cdef class FGMassBalance:
    """@Dox(JSBSim::FGMassBalance)"""

    cdef c_FGMassBalance *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_xyz_cg(self):
        """@Dox(JSBSim::FGMassBalance::GetXYZcg)"""
        return convertToNumpyVec(self.thisptr.GetXYZcg())

    def get_J(self):
        """@Dox(JSBSim::FGMassBalance::GetJ)"""
        return convertToNumpyMat(self.thisptr.GetJ())

    def get_Jinv(self):
        """@Dox(JSBSim::FGMassBalance::GetJinv)"""
        return convertToNumpyMat(self.thisptr.GetJinv())

cdef class FGJSBBase:
    """@Dox(JSBSim::FGJSBBase)"""

    cdef c_FGJSBBase *baseptr

    def __cinit__(self):
        if type(self) is FGJSBBase:
            self.baseptr = new c_FGJSBBase()

    def __dealloc__(self):
        if type(self) is FGJSBBase:
            del self.baseptr

    def get_version(self):
        """@Dox(JSBSim::FGJSBBase::GetVersion)"""
        return self.baseptr.GetVersion().decode('utf-8')

cdef class FGPropulsion:
    """@Dox(JSBSim::FGPropulsion)"""

    cdef c_FGPropulsion *thisptr

    def __init__(self):
        self.thisptr = NULL

    def init_running(self, n):
        """@Dox(JSBSim::FGPropulsion::InitRunning)"""
        self.thisptr.InitRunning(n)

    def get_num_engines(self):
        """@Dox(JSBSim::FGPropulsion::GetNumEngines)"""
        return self.thisptr.GetNumEngines()

    def get_engine(self, idx):
        """@Dox(JSBSim::FGPropulsion::GetEngine)"""
        engine = FGEngine()
        engine.thisptr = self.thisptr.GetEngine(idx)
        return engine

    def get_steady_state(self):
        """@Dox(JSBSim::FGPropulsion::GetSteadyState)"""
        return self.thisptr.GetSteadyState()

cdef class FGEngine:
    """@Dox(JSBSim::FGEngine)"""

    cdef c_FGEngine *thisptr

    def __init__(self):
        self.thisptr = NULL

    def init_running(self):
        """@Dox(JSBSim::FGEngine::InitRunning)"""
        return self.thisptr.InitRunning()

# this is the python wrapper class
cdef class FGFDMExec(FGJSBBase):
    """@Dox(JSBSim::FGFDMExec)"""

    cdef c_FGFDMExec *thisptr      # hold a C++ instance which we're wrapping

    def __cinit__(self, root_dir=None, FGPropertyManager pm_root=None):
        cdef c_FGPropertyManager* root
        # this hides startup message
        # os.environ["JSBSIM_DEBUG"]=str(0)

        if pm_root:
            root = pm_root.thisptr
        else:
            root = NULL

        self.thisptr = self.baseptr = new c_FGFDMExec(root, NULL)
        if self.thisptr is NULL:
            raise MemoryError()

        if root_dir:
            if not os.path.isdir(root_dir):
                raise IOError("Can't find root directory: {0}".format(root_dir))
            self.set_root_dir(root_dir)

    def __dealloc__(self):
        del self.thisptr

    def __repr__(self):
        return "FGFDMExec \n" \
            "root dir\t:\t{0}\n" \
            "aircraft path\t:\t{1}\n" \
            "engine path\t:\t{2}\n" \
            "systems path\t:\t{3}\n" \
                .format(
                self.get_root_dir(),
                self.get_aircraft_path(),
                self.get_engine_path(),
                self.get_systems_path())

    def __getitem__(self, key):
        _key = key.strip()
        pm = self.get_property_manager()
        if not pm.hasNode(_key):
            raise KeyError("No property named {}".format(_key))
        return self.get_property_value(_key)

    def __setitem__(self, key, value):
        self.set_property_value(key.strip(), value)

    def run(self):
        """@Dox(JSBSim::FGFDMExec::Run)"""
        return self.thisptr.Run()

    def run_ic(self):
        """@Dox(JSBSim::FGFDMExec::RunIC)"""
        return  self.thisptr.RunIC()

    def load_model(self, model, add_model_to_path=True):
        """@Dox(JSBSim::FGFDMExec::LoadModel(const std::string &, bool))"""
        return self.thisptr.LoadModel(model.encode(), add_model_to_path)

    def load_model_with_paths(self, model, aircraft_path,
                   engine_path, systems_path, add_model_to_path=True):
        """@Dox(JSBSim::FGFDMExec::LoadModel(const SGPath &, const SGPath &,
                                             const SGPath &, const std::string &,
                                             bool))"""
        return self.thisptr.LoadModel(c_SGPath(aircraft_path.encode(), NULL),
                                      c_SGPath(engine_path.encode(), NULL),
                                      c_SGPath(systems_path.encode(), NULL),
                                      model.encode(), add_model_to_path)

    def load_script(self, script, delta_t=0.0, initfile=""):
        """@Dox(JSBSim::FGFDMExec::LoadScript) """
        return self.thisptr.LoadScript(c_SGPath(script.encode(), NULL), delta_t,
                                       c_SGPath(initfile.encode(),NULL))

    def set_engine_path(self, path):
        """@Dox(JSBSim::FGFDMExec::SetEnginePath) """
        return self.thisptr.SetEnginePath(c_SGPath(path.encode(), NULL))

    def set_aircraft_path(self, path):
        """@Dox(JSBSim::FGFDMExec::SetAircraftPath)"""
        return self.thisptr.SetAircraftPath(c_SGPath(path.encode(), NULL))

    def set_systems_path(self, path):
        """@Dox(JSBSim::FGFDMExec::SetSystemsPath) """
        return self.thisptr.SetSystemsPath(c_SGPath(path.encode(), NULL))

    def set_root_dir(self, path):
        """@Dox(JSBSim::FGFDMExec::SetRootDir)"""
        self.thisptr.SetRootDir(c_SGPath(path.encode(), NULL))

        # this is a hack to fix a bug in JSBSim
        self.set_engine_path("engine")
        self.set_aircraft_path("aircraft")
        self.set_systems_path("systems")

    def get_engine_path(self):
        """@Dox(JSBSim::FGFDMExec::GetEnginePath)"""
        return self.thisptr.GetEnginePath().utf8Str().decode('utf-8')

    def get_aircraft_path(self):
        """@Dox(JSBSim::FGFDMExec::GetAircraftPath)"""
        return self.thisptr.GetAircraftPath().utf8Str().decode('utf-8')

    def get_systems_path(self):
        """@Dox(JSBSim::FGFDMExec::GetSystemsPath)"""
        return self.thisptr.GetSystemsPath().utf8Str().decode('utf-8')

    def get_full_aircraft_path(self):
        """@Dox(JSBSim::FGFDMExec::GetFullAircraftPath)"""
        return self.thisptr.GetFullAircraftPath().utf8Str().decode('utf-8')

    def get_root_dir(self):
        """@Dox(JSBSim::FGFDMExec::GetRootDir)"""
        return self.thisptr.GetRootDir().utf8Str().decode('utf-8')

    def get_property_value(self, name):
        """@Dox(JSBSim::FGFDMExec::GetPropertyValue) """
        return self.thisptr.GetPropertyValue(name.encode())

    def set_property_value(self, name, value):
        """@Dox(JSBSim::FGFDMExec::SetPropertyValue)"""
        self.thisptr.SetPropertyValue(name.encode(), value)

    def get_model_name(self):
        """@Dox(JSBSim::FGFDMExec::GetModelName)"""
        return self.thisptr.GetModelName()

    def set_output_directive(self, fname):
        """@Dox(JSBSim::FGFDMExec::SetOutputDirectives)"""
        return self.thisptr.SetOutputDirectives(c_SGPath(fname.encode(), NULL))

    # def force_output(self, index):
    #     """@Dox(JSBSim::FGFDMExec::ForceOutput)"""
    #     self.thisptr.ForceOutput(index)

    def set_logging_rate(self, rate):
        """@Dox(JSBSim::FGFDMExec::SetLoggingRate)"""
        self.thisptr.SetLoggingRate(rate)

    def set_output_filename(self, n, fname):
        """@Dox(JSBSim::FGFDMExec::SetOutputFileName)"""
        return self.thisptr.SetOutputFileName(n, fname.encode())

    def get_output_filename(self, n):
        """@Dox(JSBSim::FGFDMExec::GetOutputFileName)"""
        return self.thisptr.GetOutputFileName(n)

    def do_trim(self, mode):
        """@Dox(JSBSim::FGFDMExec::DoTrim) """
        self.thisptr.DoTrim(mode)

    def disable_output(self):
        """@Dox(JSBSim::FGFDMExec::DisableOutput)"""
        self.thisptr.DisableOutput()

    def enable_output(self):
        """@Dox(JSBSim::FGFDMExec::EnableOutput)"""
        self.thisptr.EnableOutput()

    def hold(self):
        """@Dox(JSBSim::FGFDMExec::Hold)"""
        self.thisptr.Hold()

    def enable_increment_then_hold(self, time_steps):
        """@Dox(JSBSim::FGFDMExec::EnableIncrementThenHold)"""
        self.thisptr.EnableIncrementThenHold(time_steps)

    def check_incremental_hold(self):
        """@Dox(JSBSim::FGFDMExec::CheckIncrementalHold)"""
        self.thisptr.CheckIncrementalHold()

    def resume(self):
        """@Dox(JSBSim::FGFDMExec::Resume)"""
        self.thisptr.Resume()

    def holding(self):
        """@Dox(JSBSim::FGFDMExec::Holding)"""
        return self.thisptr.Holding()

    def reset_to_initial_conditions(self, mode):
        """@Dox(JSBSim::FGFDMExec::ResetToInitialConditions)"""
        self.thisptr.ResetToInitialConditions(mode)

    def set_debug_level(self, level):
        """@Dox(JSBSim::FGFDMExec::SetDebugLevel)"""
        self.thisptr.SetDebugLevel(level)

    def query_property_catalog(self, check):
        """@Dox(JSBSim::FGFDMExec::QueryPropertyCatalog)"""
        catalog = (self.thisptr.QueryPropertyCatalog(check.encode())).decode('utf-8').rstrip().split('\n')
        if len(catalog) == 1 and catalog[0] == "No matches found":
            return []
        else:
            return catalog

    def get_property_catalog(self, check):
        """Retrieves the property catalog as a dictionary."""
        catalog = {}
        for item in self.query_property_catalog(check):
            property_name = item.split(" ")[0]  # remove any (RW) flags
            catalog[property_name] = self.get_property_value(property_name)
        return catalog

    def print_property_catalog(self):
        """@Dox(JSBSim::FGFDMExec::PrintPropertyCatalog)"""
        self.thisptr.PrintPropertyCatalog()

    def print_simulation_configuration(self):
        """@Dox(JSBSim::FGFDMExec::PrintSimulationConfiguration)"""
        self.thisptr.PrintSimulationConfiguration()

    def set_trim_status(self, status):
        """@Dox(JSBSim::FGFDMExec::SetTrimStatus)"""
        self.thisptr.SetTrimStatus(status)

    def get_trim_status(self):
        """@Dox(JSBSim::FGFDMExec::GetTrimStatus)"""
        return self.thisptr.GetTrimStatus()

    def get_propulsion_tank_report(self):
        """@Dox(JSBSim::FGFDMExec::GetPropulsionTankReport)"""
        return self.thisptr.GetPropulsionTankReport()

    def get_sim_time(self):
        """@Dox(JSBSim::FGFDMExec::GetSimTime)"""
        return self.thisptr.GetSimTime()

    def get_delta_t(self):
        """@Dox(JSBSim::FGFDMExec::GetDeltaT)"""
        return self.thisptr.GetDeltaT()

    def suspend_integration(self):
        """@Dox(JSBSim::FGFDMExec::SuspendIntegration)"""
        self.thisptr.SuspendIntegration()

    def resume_integration(self):
        """@Dox(JSBSim::FGFDMExec::ResumeIntegration)"""
        self.thisptr.ResumeIntegration()

    def integration_suspended(self):
        """@Dox(JSBSim::FGFDMExec::IntegrationSuspended)"""
        return self.thisptr.IntegrationSuspended()

    def set_sim_time(self, time):
        """@Dox(JSBSim::FGFDMExec::Setsim_time)"""
        return self.thisptr.Setsim_time(time)

    def set_dt(self, dt):
        """@Dox(JSBSim::FGFDMExec::Setdt)"""
        self.thisptr.Setdt(dt)

    def incr_time(self):
        """@Dox(JSBSim::FGFDMExec::IncrTime)"""
        return self.thisptr.IncrTime()

    def get_debug_level(self):
        """@Dox(JSBSim::FGFDMExec::GetDebugLevel) """
        return self.thisptr.GetDebugLevel()

    def load_ic(self, rstfile, useStoredPath):
        rstfile = rstfile.encode()
        return self.thisptr.GetIC().Load(c_SGPath(rstfile, NULL), useStoredPath)

    def get_propagate(self):
        """@Dox(JSBSim::FGFDMExec::GetPropagate)"""
        propagate = FGPropagate()
        propagate.thisptr = self.thisptr.GetPropagate()
        return propagate

    def get_property_manager(self):
        """@Dox(JSBSim::FGFDMExec::GetPropertyManager)"""
        pm = FGPropertyManager()
        pm.thisptr = self.thisptr.GetPropertyManager()
        return pm

    def get_ground_reactions(self):
        """@Dox(JSBSim::FGFDMExec::GetGroundReactions)"""
        grndreact = FGGroundReactions()
        grndreact.thisptr = self.thisptr.GetGroundReactions()
        return grndreact

    def get_auxiliary(self):
        """@Dox(JSBSim::FGFDMExec::GetAuxiliary)"""
        auxiliary = FGAuxiliary()
        auxiliary.thisptr = self.thisptr.GetAuxiliary()
        return auxiliary

    def get_aerodynamics(self):
        """@Dox(JSBSim::FGFDMExec::GetAerodynamics)"""
        aerodynamics = FGAerodynamics()
        aerodynamics.thisptr = self.thisptr.GetAerodynamics()
        return aerodynamics

    def get_aircraft(self):
        """@Dox(JSBSim::FGFDMExec::GetAircraft)"""
        aircraft = FGAircraft()
        aircraft.thisptr = self.thisptr.GetAircraft()
        return aircraft

    def get_mass_balance(self):
        """@Dox(JSBSim::FGFDMExec::GetMassBalance)"""
        massbalance = FGMassBalance()
        massbalance.thisptr = self.thisptr.GetMassBalance()
        return massbalance

    def get_atmosphere(self):
        """@Dox(JSBSim::FGFDMExec::GetAtmosphere)"""
        atmosphere = FGAtmosphere()
        atmosphere.thisptr = self.thisptr.GetAtmosphere()
        return atmosphere

    def get_propulsion(self):
        """@Dox(JSBSim::FGFDMExec::GetPropulsion)"""
        propulsion = FGPropulsion()
        propulsion.thisptr = self.thisptr.GetPropulsion()
        return propulsion
