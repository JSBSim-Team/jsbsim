# PyJSBSim a JSBSim python interface using cython.
#
# Copyright (c) 2013 James Goppert
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

import os, platform, numpy

cdef convertToNumpyMat(const c_FGMatrix33& m):
    return numpy.mat([[m.Entry(1, 1), m.Entry(1, 2), m.Entry(1, 3)],
                      [m.Entry(2, 1), m.Entry(2, 2), m.Entry(2, 3)],
                      [m.Entry(3, 1), m.Entry(3, 2), m.Entry(3, 3)]])


cdef convertToNumpyVec(const c_FGColumnVector3& v):
    return numpy.mat([v.Entry(1), v.Entry(2), v.Entry(3)]).T

cdef class FGPropagate:

    cdef c_FGPropagate *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_Tl2b(self):
        return convertToNumpyMat(self.thisptr.GetTl2b())

    def get_Tec2b(self):
        return convertToNumpyMat(self.thisptr.GetTec2b())

    def get_uvw(self):
        return convertToNumpyVec(self.thisptr.GetUVW())

cdef class FGPropertyManager:

     cdef c_FGPropertyManager *thisptr

     def __init__(self):
         self.thisptr = NULL

     def hasNode(self, path):
         return self.thisptr.HasNode(path.encode())

cdef class FGGroundReactions:

    cdef c_FGGroundReactions *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_gear_unit(self, gear):
        lgear = FGLGear()
        lgear.thisptr = self.thisptr.GetGearUnit(gear)
        return lgear

    def get_num_gear_units(self):
        return self.thisptr.GetNumGearUnits()

cdef class FGLGear:

    cdef c_FGLGear *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_steer_norm(self):
        return self.thisptr.GetSteerNorm()

cdef class FGAuxiliary:

    cdef c_FGAuxiliary *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_Tw2b(self):
        return convertToNumpyMat(self.thisptr.GetTw2b())

    def get_Tb2w(self):
        return convertToNumpyMat(self.thisptr.GetTb2w())

cdef class FGAerodynamics:

    cdef c_FGAerodynamics *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_moments_MRC(self):
        return convertToNumpyVec(self.thisptr.GetMomentsMRC())

    def get_forces(self):
        return convertToNumpyVec(self.thisptr.GetForces())

cdef class FGAircraft:

    cdef c_FGAircraft *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_xyz_rp(self):
        return convertToNumpyVec(self.thisptr.GetXYZrp())

cdef class FGMassBalance:

    cdef c_FGMassBalance *thisptr

    def __init__(self):
        self.thisptr = NULL

    def get_xyz_cg(self):
        return convertToNumpyVec(self.thisptr.GetXYZcg())

# this is the python wrapper class
cdef class FGFDMExec:

    cdef c_FGFDMExec *thisptr      # hold a C++ instance which we're wrapping

    def __cinit__(self, root_dir=None):
        # this hides startup message
        # os.environ["JSBSIM_DEBUG"]=str(0)
        self.thisptr = new c_FGFDMExec(NULL, NULL)
        if self.thisptr is NULL:
            raise MemoryError()
        if root_dir is None:
            self.find_root_dir()
        else:
            if not os.path.isdir(root_dir):
                raise IOError("Can't find root directory: {0}".format(root_dir))
            self.set_root_dir(root_dir)

    def simulate(self, record_properties=[], t_final=1, dt=1.0/120, verbose=False):
        y = {}
        t = []
        self.set_dt(dt)
        self.run_ic()
        for prop in record_properties:
            y[prop] = []
        while self.get_sim_time() < t_final:
            if self.run() is False:
                break
            if verbose:
                print 't:', self.get_sim_time()
            t.append(self.get_sim_time())
            print 'dt: ', self.get_delta_t()
            for prop in record_properties:
                y[prop].append(self.get_property_value(prop))
        return (t,y)

    def find_root_dir(self, search_paths=[], verbose=False):
        root_dir = None
        search_paths.append(os.environ.get("JSBSIM"))
        if platform.system() == "Linux":
            search_paths.append("/usr/local/share/JSBSim/")
            search_paths.append("/usr/share/JSBSim/")
        elif platform.system() == "Windows":
            #TODO add some windows search paths
            pass
        elif platform.system() == "Darwin":
            search_paths.append("/opt/local/share/JSBSim/")
            search_paths.append("/usr/local/share/JSBSim/")

        if verbose:
            print "search_paths"
        for path in search_paths:
            if verbose:
                print '\t', path
            if path is not None and os.path.isdir(path):
                root_dir = path
                break
        if root_dir is None:
            raise IOError("Could not find JSBSim root, try "
                          "defining JSBSIM environment variable")
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
        return self.get_property_value(key)

    def __setitem__(self, key, value):
        self.set_property_value(key, value)

    def run(self):
        """
        This function executes each scheduled model in succession.
        @param return true if successful, false if sim should be ended
        """
        return self.thisptr.Run()

    def run_ic(self):
        """
        Initializes the sim from the initial condition object and executes
        each scheduled model without integrating i.e. dt=0.
        @param return true if successful
        """
        return  self.thisptr.RunIC()

    def load_model(self, model, add_model_to_path=True):
        """
        Loads an aircraft model.
        @param AircraftPath path to the aircraft/ directory. For instance:
            "aircraft". Under aircraft, then, would be directories for various
            modeled aircraft such as C172/, x15/, etc.
        @param EnginePath path to the directory under which engine config
            files are kept, for instance "engine"
        @param SystemsPath path to the directory under which systems config
            files are kept, for instance "systems"
        @param model the name of the aircraft model itself. This file will
            be looked for in the directory specified in the AircraftPath variable,
            and in turn under the directory with the same name as the model. For
            instance: "aircraft/x15/x15.xml"
        @param addModelToPath set to true to add the model name to the
            AircraftPath, defaults to true
        @return true if successful
        """
        model = model.encode()
        return self.thisptr.LoadModel(model, add_model_to_path)

    def load_model_with_paths(self, model, aircraft_path,
                   engine_path, systems_path, add_model_to_path=True):
        """
        Loads an aircraft model.  The paths to the aircraft and engine
        config file directories must be set prior to calling this.  See
        below.
        @param model the name of the aircraft model itself. This file will
            be looked for in the directory specified in the AircraftPath variable,
            and in turn under the directory with the same name as the model. For
            instance: "aircraft/x15/x15.xml"
        @param addModelToPath set to true to add the model name to the
            AircraftPath, defaults to true
        @return true if successful
        """
        model = model.encode()
        aircraft_path = aircraft_path.encode()
        engine_path = engine_path.encode()
        systems_path = systems_path.encode()

        return self.thisptr.LoadModel(c_SGPath(aircraft_path, NULL),
            c_SGPath(engine_path, NULL), c_SGPath(systems_path, NULL), model,
            add_model_to_path)

    def load_script(self, script, delta_t=0.0, initfile=""):
        """
        Loads a script
        @param Script The full path name and file name for the script to be loaded.
        @param deltaT The simulation integration step size, if given.  If no value is supplied
            then 0.0 is used and the value is expected to be supplied in
            the script file itself.
        @param initfile The initialization file that will override the initialization file
            specified in the script file. If no file name is given on the command line,
            the file specified in the script will be used. If an initialization file
            is not given in either place, an error will result.
        @return true if successfully loads; false otherwise. */
        """
        script = script.encode()
        initfile = initfile.encode()

        return self.thisptr.LoadScript(c_SGPath(script, NULL), delta_t,
                                       c_SGPath(initfile,NULL))

    def set_engine_path(self, path):
        """
        Sets the path to the engine config file directories.
        @param path path to the directory under which engine config
            files are kept, for instance "engine"
        """
        path = path.encode()
        return self.thisptr.SetEnginePath(c_SGPath(path, NULL))

    def set_aircraft_path(self, path):
        """
        Sets the path to the aircraft config file directories.
        @param path path to the aircraft directory. For instance:
            "aircraft". Under aircraft, then, would be directories for various
            modeled aircraft such as C172/, x15/, etc.
        """
        path = path.encode()
        return self.thisptr.SetAircraftPath(c_SGPath(path, NULL))

    def set_systems_path(self, path):
        """
        Sets the path to the systems config file directories.
        @param path path to the directory under which systems config
            files are kept, for instance "systems"
        """
        path = path.encode()
        return self.thisptr.SetSystemsPath(c_SGPath(path, NULL))

    def set_root_dir(self, path):
        """
        Sets the root directory where JSBSim starts looking for its system directories.
        @param path the string containing the root directory.
        """
        path = path.encode()
        self.thisptr.SetRootDir(c_SGPath(path, NULL))

        # this is a hack to fix a bug in JSBSim
        self.set_engine_path("engine")
        self.set_aircraft_path("aircraft")
        self.set_systems_path("systems")

    def get_engine_path(self):
        """
        Retrieves the engine path
        """
        return self.thisptr.GetEnginePath().utf8Str()

    def get_aircraft_path(self):
        """
        Retrieves the aircraft path
        """
        return self.thisptr.GetAircraftPath().utf8Str()

    def get_systems_path(self):
        """
        Retrieves the systems path
        """
        return self.thisptr.GetSystemsPath().utf8Str()

    def get_full_aircraft_path(self):
        """
        Retrieves the full aircraft path name
        """
        return self.thisptr.GetFullAircraftPath().utf8Str()

    def get_root_dir(self):
        """
        Retrieves the Root Directory.
        @return the string representing the root (base) JSBSim directory.
        """
        return self.thisptr.GetRootDir().utf8Str()

    def get_property_value(self, name):
        """
        Retrieves the value of a property.
        @param property the name of the property
        @result the value of the specified property
        """
        name = name.encode()
        return self.thisptr.GetPropertyValue(name)

    def set_property_value(self, name, value):
        """
        Sets a property value.
        @param property the property to be set
        @param value the value to set the property to *
        """
        self.thisptr.SetPropertyValue(name.encode(), value)

    def get_model_name(self):
        """
        Retrieves the model name.
        """
        return self.thisptr.GetModelName()

    def set_output_directive(self, fname):
        """
        Sets the output (logging) mechanism for this run.
        Calling this function passes the name of an output directives file to
        the FGOutput object associated with this run. The call to this function
        should be made prior to loading an aircraft model. This call results in an
        FGOutput object being built as the first Output object in the FDMExec-managed
        list of Output objects that may be created for an aircraft model. If this call
        is made after an aircraft model is loaded, there is no effect. Any Output
        objects added by the aircraft model itself (in an &lt;output> element) will be
        added after this one. Care should be taken not to refer to the same file
        name.
        An output directives file contains an &lt;output> &lt;/output> element, within
        which should be specified the parameters or parameter groups that should
        be logged.
        @param fname the filename of an output directives file.
        """
        return self.thisptr.SetOutputDirectives(c_SGPath(fname.encode(), NULL))

    #def force_output(self, index):
        #"""
        #Forces the specified output object to print its items once
        #"""
        #self.thisptr.ForceOutput(index)

    def set_logging_rate(self, rate):
        """
        Sets the logging rate for all output objects (if any).
        """
        self.thisptr.SetLoggingRate(rate)

    def set_output_filename(self, n, fname):
        """
        Sets (or overrides) the output filename
        @param n index of file
        @param fname the name of the file to output data to
        @return true if successful, false if there is no output specified for the flight model
        """
        return self.thisptr.SetOutputFileName(n, fname.encode())

    def get_output_filename(self, n):
        """
        Retrieves the current output filename.
        @param n index of file
        @return the name of the output file for the first output specified by the flight model.
            If none is specified, the empty string is returned.
        """
        return self.thisptr.GetOutputFileName(n)

    def do_trim(self, mode):
        """
        Executes trimming in the selected mode.
        @param mode Specifies how to trim:
            - tLongitudinal=0
            - tFull
            - tGround
            - tPullup
            - tCustom
            - tTurn
            - tNone
        """
        self.thisptr.DoTrim(mode)

    def disable_output(self):
        """
        Disables data logging to all outputs.
        """
        self.thisptr.DisableOutput()

    def enable_output(self):
        """
        Enables data logging to all outputs.
        """
        self.thisptr.EnableOutput()

    def hold(self):
        """
        Pauses execution by preventing time from incrementing.
        """
        self.thisptr.Hold()

    def enable_increment_then_hold(self, time_steps):
        """
        Turn on hold after increment
        """
        self.thisptr.EnableIncrementThenHold(time_steps)

    def check_incremental_hold(self):
        """
        Checks if required to hold afer increment
        """
        self.thisptr.CheckIncrementalHold()

    def resume(self):
        """
        Resumes execution from a "Hold".
        """
        self.thisptr.Resume()

    def holding(self):
        """
        Returns true if the simulation is Holding (i.e. simulation time is not moving).
        """
        return self.thisptr.Holding()

    def reset_to_initial_conditions(self, mode):
        """
        Resets the initial conditions object and prepares the simulation to run
        again. If mode is set to 1 the output instances will take special actions
        such as closing the current output file and open a new one with a
        different name.
        @param mode Sets the reset mode.*/
        """
        self.thisptr.ResetToInitialConditions(mode)

    def set_debug_level(self, level):
        """
        Sets the debug level.
        """
        self.thisptr.SetDebugLevel(level)

    def query_property_catalog(self, check):
        """
        Retrieves property or properties matching the supplied string.
        A string is returned that contains a carriage return delimited list of all
        strings in the property catalog that matches the supplied check string.
        @param check The string to search for in the property catalog.
        @return the carriage-return-delimited string containing all matching strings
            in the catalog.
        """
        return (self.thisptr.QueryPropertyCatalog(check)).rstrip().split('\n')

    def get_property_catalog(self, check):
        """
        Retrieves the property catalog as a dictionary.
        """
        catalog = {}
        for item in self.query_property_catalog(check):
            catalog[item] = self.get_property_value(item)
        return catalog

    def print_property_catalog(self):
        """
        Print the contents of the property catalog for the loaded aircraft.
        """
        self.thisptr.PrintPropertyCatalog()

    def set_trim_status(self, status):
        self.thisptr.SetTrimStatus(status)

    def get_trim_status(self):
        return self.thisptr.GetTrimStatus()

    def get_propulsion_tank_report(self):
        return self.thisptr.GetPropulsionTankReport()

    def get_sim_time(self):
        """
        Returns the cumulative simulation time in seconds.
        """
        return self.thisptr.GetSimTime()

    def get_delta_t(self):
        """
        Returns the simulation delta T.
        """
        return self.thisptr.GetDeltaT()

    def suspend_integration(self):
        """
        Suspends the simulation and sets the delta T to zero.
        """
        self.thisptr.SuspendIntegration()

    def resume_integration(self):
        """
        Resumes the simulation by resetting delta T to the correct value.
        """
        self.thisptr.ResumeIntegration()

    def integration_suspended(self):
        """
        Returns the simulation suspension state.
        @return true if suspended, false if executing
        """
        return self.thisptr.IntegrationSuspended()

    def set_sim_time(self, time):
        """
        Sets the current sim time.
        @param time the current time
        @return the current simulation time.
        """
        return self.thisptr.Setsim_time(time)

    def set_dt(self, dt):
        """
        Sets the integration time step for the simulation executive.
        @param dt the time step in seconds.
        """
        self.thisptr.Setdt(dt)

    def incr_time(self):
        """
        Increments the simulation time if not in Holding mode. The Frame counter
        is also incremented.
        @return the new simulation time.
        """
        return self.thisptr.IncrTime()

    def get_debug_level(self):
        """
        Retrieves the current debug level setting.
        """
        return self.thisptr.GetDebugLevel()

    def propulsion_init_running(self, n):
        self.thisptr.GetPropulsion().InitRunning(n)

    def propulsion_get_num_engines(self):
        return self.thisptr.GetPropulsion().GetNumEngines()

    def load_ic(self, rstfile, useStoredPath):
        rstfile = rstfile.encode()
        return self.thisptr.GetIC().Load(c_SGPath(rstfile, NULL), useStoredPath)

    def get_propagate(self):
        propagate = FGPropagate()
        propagate.thisptr = self.thisptr.GetPropagate()
        return propagate

    def get_property_manager(self):
        pm = FGPropertyManager()
        pm.thisptr = self.thisptr.GetPropertyManager()
        return pm

    def get_ground_reactions(self):
        grndreact = FGGroundReactions()
        grndreact.thisptr = self.thisptr.GetGroundReactions()
        return grndreact

    def get_auxiliary(self):
        auxiliary = FGAuxiliary()
        auxiliary.thisptr = self.thisptr.GetAuxiliary()
        return auxiliary

    def get_aerodynamics(self):
        aerodynamics = FGAerodynamics()
        aerodynamics.thisptr = self.thisptr.GetAerodynamics()
        return aerodynamics

    def get_aircraft(self):
        aircraft = FGAircraft()
        aircraft.thisptr = self.thisptr.GetAircraft()
        return aircraft

    def get_mass_balance(self):
        massbalance = FGMassBalance()
        massbalance.thisptr = self.thisptr.GetMassBalance()
        return massbalance
