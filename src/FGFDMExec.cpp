
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFDMExec.cpp
 Author:       Jon S. Berndt
 Date started: 11/17/98
 Purpose:      Schedules and runs the model routines.

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class wraps up the simulation scheduling routines.

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iomanip>

#include "FGFDMExec.h"
#include "models/atmosphere/FGStandardAtmosphere.h"
#include "models/atmosphere/FGWinds.h"
#include "models/FGFCS.h"
#include "models/FGPropulsion.h"
#include "models/FGMassBalance.h"
#include "models/FGExternalReactions.h"
#include "models/FGBuoyantForces.h"
#include "models/FGAerodynamics.h"
#include "models/FGInertial.h"
#include "models/FGAircraft.h"
#include "models/FGAccelerations.h"
#include "models/FGAuxiliary.h"
#include "models/FGInput.h"
#include "initialization/FGTrim.h"
#include "input_output/FGScript.h"
#include "input_output/FGXMLFileRead.h"
#include "initialization/FGInitialCondition.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Constructor

FGFDMExec::FGFDMExec(FGPropertyManager* root, std::shared_ptr<unsigned int> fdmctr)
  : RandomSeed(0), RandomGenerator(make_shared<RandomNumberGenerator>(RandomSeed)),
    FDMctr(fdmctr)
{
  Frame           = 0;
  disperse        = 0;

  RootDir = "";

  modelLoaded = false;
  IsChild = false;
  holding = false;
  Terminate = false;
  HoldDown = false;

  IncrementThenHolding = false;  // increment then hold is off by default
  TimeStepsUntilHold = -1;

  sim_time = 0.0;
  dT = 1.0/120.0; // a default timestep size. This is needed for when JSBSim is
                  // run in standalone mode with no initialization file.

  AircraftPath = "aircraft";
  EnginePath = "engine";
  SystemsPath = "systems";

  try {
    char* num = getenv("JSBSIM_DEBUG");
    if (num) debug_lvl = atoi(num); // set debug level
  } catch (...) {                   // if error set to 1
    debug_lvl = 1;
  }

  if (!FDMctr) {
    FDMctr = std::make_shared<unsigned int>(); // Create and initialize the child FDM counter
    *FDMctr = 0;
  }

  // Store this FDM's ID
  IdFDM = *FDMctr; // The main (parent) JSBSim instance is always the "zeroth"

  // Prepare FDMctr for the next child FDM id
  (*FDMctr)++;       // instance. "child" instances are loaded last.

  if (root == nullptr)          // Then this is the root FDM
    Root = new FGPropertyNode();
  else
    Root = root->GetNode();

  FGPropertyNode* instanceRoot = Root->GetNode("/fdm/jsbsim", IdFDM, true);
  instance = std::make_shared<FGPropertyManager>(instanceRoot);

  try {
    char* num = getenv("JSBSIM_DISPERSE");
    if (num) {
      if (atoi(num) != 0) disperse = 1;  // set dispersions on
    }
  } catch (...) {                        // if error set to false
    disperse = 0;
    std::cerr << "Could not process JSBSIM_DISPERSIONS environment variable: Assumed NO dispersions." << endl;
  }

  Debug(0);
  // this is to catch errors in binding member functions to the property tree.
  try {
    Allocate();
  }
  catch (const string& msg) {
    cerr << endl << "Caught error: " << msg << endl;
    throw;
  }
  catch (const BaseException& e) {
    cout << endl << "Caught error: " << e.what() << endl;
    throw;
  }

  trim_status = false;
  ta_mode     = 99;
  trim_completed = 0;

  Constructing = true;
  typedef int (FGFDMExec::*iPMF)(void) const;
  instance->Tie("simulation/do_simple_trim", this, (iPMF)0, &FGFDMExec::DoTrim);
  instance->Tie("simulation/reset", this, (iPMF)0, &FGFDMExec::ResetToInitialConditions);
  instance->Tie("simulation/disperse", this, &FGFDMExec::GetDisperse);
  instance->Tie("simulation/randomseed", this, (iPMF)&FGFDMExec::SRand, &FGFDMExec::SRand);
  instance->Tie("simulation/terminate", (int *)&Terminate);
  instance->Tie("simulation/pause", (int *)&holding);
  instance->Tie("simulation/sim-time-sec", this, &FGFDMExec::GetSimTime);
  instance->Tie("simulation/dt", this, &FGFDMExec::GetDeltaT);
  instance->Tie("simulation/jsbsim-debug", this, &FGFDMExec::GetDebugLevel, &FGFDMExec::SetDebugLevel);
  instance->Tie("simulation/frame", (int *)&Frame);
  instance->Tie("simulation/trim-completed", (int *)&trim_completed);
  instance->Tie("forces/hold-down", this, &FGFDMExec::GetHoldDown, &FGFDMExec::SetHoldDown);

  Constructing = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFDMExec::~FGFDMExec()
{
  try {
    Unbind();
    DeAllocate();
  } catch (const string& msg ) {
    cout << "Caught error: " << msg << endl;
  }

  if (!FDMctr) (*FDMctr)--;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFDMExec::Setsim_time(double cur_time) {
  sim_time = cur_time;
  Inertial->SetTime(sim_time);
  return sim_time;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFDMExec::IncrTime(void) {
  if (!holding && !IntegrationSuspended()) {
    sim_time += dT;
    Inertial->SetTime(sim_time);
    Frame++;
  }
  return sim_time;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::Allocate(void)
{
  bool result=true;

  Models.resize(eNumStandardModels);

  // First build the inertial model since some other models are relying on
  // the inertial model and the ground callback to build themselves.
  // Note that this does not affect the order in which the models will be
  // executed later.
  Models[eInertial]          = std::make_shared<FGInertial>(this);

  // See the eModels enum specification in the header file. The order of the
  // enums specifies the order of execution. The Models[] vector is the primary
  // storage array for the list of models.
  Models[ePropagate]         = std::make_shared<FGPropagate>(this);
  Models[eInput]             = std::make_shared<FGInput>(this);
  Models[eAtmosphere]        = std::make_shared<FGStandardAtmosphere>(this);
  Models[eWinds]             = std::make_shared<FGWinds>(this);
  Models[eSystems]           = std::make_shared<FGFCS>(this);
  Models[eMassBalance]       = std::make_shared<FGMassBalance>(this);
  Models[eAuxiliary]         = std::make_shared<FGAuxiliary>(this);
  Models[ePropulsion]        = std::make_shared<FGPropulsion>(this);
  Models[eAerodynamics]      = std::make_shared<FGAerodynamics> (this);
  Models[eGroundReactions]   = std::make_shared<FGGroundReactions>(this);
  Models[eExternalReactions] = std::make_shared<FGExternalReactions>(this);
  Models[eBuoyantForces]     = std::make_shared<FGBuoyantForces>(this);
  Models[eAircraft]          = std::make_shared<FGAircraft>(this);
  Models[eAccelerations]     = std::make_shared<FGAccelerations>(this);
  Models[eOutput]            = std::make_shared<FGOutput>(this);

  // Assign the Model shortcuts for internal executive use only.
  Propagate         = static_cast<FGPropagate*>(Models[ePropagate].get());
  Inertial          = static_cast<FGInertial*>(Models[eInertial].get());
  Input             = static_cast<FGInput*>(Models[eInput].get());
  Atmosphere        = static_cast<FGAtmosphere*>(Models[eAtmosphere].get());
  Winds             = static_cast<FGWinds*>(Models[eWinds].get());
  FCS               = static_cast<FGFCS*>(Models[eSystems].get());
  MassBalance       = static_cast<FGMassBalance*>(Models[eMassBalance].get());
  Auxiliary         = static_cast<FGAuxiliary*>(Models[eAuxiliary].get());
  Propulsion        = static_cast<FGPropulsion*>(Models[ePropulsion].get());
  Aerodynamics      = static_cast<FGAerodynamics*>(Models[eAerodynamics].get());
  GroundReactions   = static_cast<FGGroundReactions*>(Models[eGroundReactions].get());
  ExternalReactions = static_cast<FGExternalReactions*>(Models[eExternalReactions].get());
  BuoyantForces     = static_cast<FGBuoyantForces*>(Models[eBuoyantForces].get());
  Aircraft          = static_cast<FGAircraft*>(Models[eAircraft].get());
  Accelerations     = static_cast<FGAccelerations*>(Models[eAccelerations].get());
  Output            = static_cast<FGOutput*>(Models[eOutput].get());

  // Initialize planet (environment) constants
  LoadPlanetConstants();

  // Initialize models
  InitializeModels();

  IC = std::make_shared<FGInitialCondition>(this);
  IC->bind(instance.get());

  modelLoaded = false;

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGPropagate> FGFDMExec::GetPropagate(void) const
{
  return static_pointer_cast<FGPropagate>(Models[ePropagate]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGInertial> FGFDMExec::GetInertial(void) const
{
  return static_pointer_cast<FGInertial>(Models[eInertial]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGInput> FGFDMExec::GetInput(void) const
{
  return static_pointer_cast<FGInput>(Models[eInput]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGAtmosphere> FGFDMExec::GetAtmosphere(void) const
{
  return static_pointer_cast<FGAtmosphere>(Models[eAtmosphere]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGWinds> FGFDMExec::GetWinds(void) const
{
  return static_pointer_cast<FGWinds>(Models[eWinds]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGFCS> FGFDMExec::GetFCS(void) const
{
  return static_pointer_cast<FGFCS>(Models[eSystems]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGMassBalance> FGFDMExec::GetMassBalance(void) const
{
  return static_pointer_cast<FGMassBalance>(Models[eMassBalance]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGAuxiliary> FGFDMExec::GetAuxiliary(void) const
{
  return static_pointer_cast<FGAuxiliary>(Models[eAuxiliary]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGPropulsion> FGFDMExec::GetPropulsion(void) const
{
  return static_pointer_cast<FGPropulsion>(Models[ePropulsion]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGAerodynamics> FGFDMExec::GetAerodynamics(void) const
{
  return static_pointer_cast<FGAerodynamics>(Models[eAerodynamics]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGGroundReactions> FGFDMExec::GetGroundReactions(void) const
{
  return static_pointer_cast<FGGroundReactions>(Models[eGroundReactions]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGExternalReactions> FGFDMExec::GetExternalReactions(void) const
{
  return static_pointer_cast<FGExternalReactions>(Models[eExternalReactions]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGBuoyantForces> FGFDMExec::GetBuoyantForces(void) const
{
  return static_pointer_cast<FGBuoyantForces>(Models[eBuoyantForces]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGAircraft> FGFDMExec::GetAircraft(void) const
{
  return static_pointer_cast<FGAircraft>(Models[eAircraft]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGAccelerations> FGFDMExec::GetAccelerations(void) const
{
  return static_pointer_cast<FGAccelerations>(Models[eAccelerations]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGOutput> FGFDMExec::GetOutput(void) const
{
  return static_pointer_cast<FGOutput>(Models[eOutput]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::InitializeModels(void)
{
  for (unsigned int i = 0; i < Models.size(); i++) {
    // The Input/Output models must not be initialized prior to IC loading
    if (i == eInput || i == eOutput) continue;

    LoadInputs(i);
    Models[i]->InitModel();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::DeAllocate(void)
{

  Models.clear();
  modelLoaded = false;
  return modelLoaded;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::Run(void)
{
  bool success=true;

  Debug(2);

  for (auto &ChildFDM: ChildFDMList) {
    ChildFDM->AssignState(Propagate); // Transfer state to the child FDM
    ChildFDM->Run();
  }

  IncrTime();

  // returns true if success, false if complete
  if (Script && !IntegrationSuspended()) success = Script->RunScript();

  for (unsigned int i = 0; i < Models.size(); i++) {
    LoadInputs(i);
    Models[i]->Run(holding);
  }

  if (Terminate) success = false;

  return success;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::LoadInputs(unsigned int idx)
{
  switch(idx) {
  case ePropagate:
    Propagate->in.vPQRidot     = Accelerations->GetPQRidot();
    Propagate->in.vUVWidot     = Accelerations->GetUVWidot();
    Propagate->in.DeltaT       = dT;
    break;
  case eInput:
    break;
  case eInertial:
    Inertial->in.Position      = Propagate->GetLocation();
    break;
  case eAtmosphere:
    Atmosphere->in.altitudeASL = Propagate->GetAltitudeASL();
    break;
  case eWinds:
    Winds->in.AltitudeASL      = Propagate->GetAltitudeASL();
    Winds->in.DistanceAGL      = Propagate->GetDistanceAGL();
    Winds->in.Tl2b             = Propagate->GetTl2b();
    Winds->in.Tw2b             = Auxiliary->GetTw2b();
    Winds->in.V                = Auxiliary->GetVt();
    Winds->in.totalDeltaT      = dT * Winds->GetRate();
    break;
  case eAuxiliary:
    Auxiliary->in.Pressure     = Atmosphere->GetPressure();
    Auxiliary->in.Density      = Atmosphere->GetDensity();
    Auxiliary->in.DensitySL    = Atmosphere->GetDensitySL();
    Auxiliary->in.PressureSL   = Atmosphere->GetPressureSL();
    Auxiliary->in.Temperature  = Atmosphere->GetTemperature();
    Auxiliary->in.SoundSpeed   = Atmosphere->GetSoundSpeed();
    Auxiliary->in.KinematicViscosity = Atmosphere->GetKinematicViscosity();
    Auxiliary->in.DistanceAGL  = Propagate->GetDistanceAGL();
    Auxiliary->in.Mass         = MassBalance->GetMass();
    Auxiliary->in.Tl2b         = Propagate->GetTl2b();
    Auxiliary->in.Tb2l         = Propagate->GetTb2l();
    Auxiliary->in.vPQR         = Propagate->GetPQR();
    Auxiliary->in.vPQRi        = Propagate->GetPQRi();
    Auxiliary->in.vPQRidot     = Accelerations->GetPQRidot();
    Auxiliary->in.vUVW         = Propagate->GetUVW();
    Auxiliary->in.vUVWdot      = Accelerations->GetUVWdot();
    Auxiliary->in.vVel         = Propagate->GetVel();
    Auxiliary->in.vBodyAccel   = Accelerations->GetBodyAccel();
    Auxiliary->in.ToEyePt      = MassBalance->StructuralToBody(Aircraft->GetXYZep());
    Auxiliary->in.VRPBody      = MassBalance->StructuralToBody(Aircraft->GetXYZvrp());
    Auxiliary->in.RPBody       = MassBalance->StructuralToBody(Aircraft->GetXYZrp());
    Auxiliary->in.vFw          = Aerodynamics->GetvFw();
    Auxiliary->in.vLocation    = Propagate->GetLocation();
    Auxiliary->in.CosTht       = Propagate->GetCosEuler(eTht);
    Auxiliary->in.SinTht       = Propagate->GetSinEuler(eTht);
    Auxiliary->in.CosPhi       = Propagate->GetCosEuler(ePhi);
    Auxiliary->in.SinPhi       = Propagate->GetSinEuler(ePhi);
    Auxiliary->in.TotalWindNED = Winds->GetTotalWindNED();
    Auxiliary->in.TurbPQR      = Winds->GetTurbPQR();
    break;
  case eSystems:
    // Dynamic inputs come into the components that FCS manages through properties
    break;
  case ePropulsion:
    Propulsion->in.Pressure         = Atmosphere->GetPressure();
    Propulsion->in.PressureRatio    = Atmosphere->GetPressureRatio();
    Propulsion->in.Temperature      = Atmosphere->GetTemperature();
    Propulsion->in.DensityRatio     = Atmosphere->GetDensityRatio();
    Propulsion->in.Density          = Atmosphere->GetDensity();
    Propulsion->in.Soundspeed       = Atmosphere->GetSoundSpeed();
    Propulsion->in.TotalPressure    = Auxiliary->GetTotalPressure();
    Propulsion->in.Vc               = Auxiliary->GetVcalibratedKTS();
    Propulsion->in.Vt               = Auxiliary->GetVt();
    Propulsion->in.qbar             = Auxiliary->Getqbar();
    Propulsion->in.TAT_c            = Auxiliary->GetTAT_C();
    Propulsion->in.AeroUVW          = Auxiliary->GetAeroUVW();
    Propulsion->in.AeroPQR          = Auxiliary->GetAeroPQR();
    Propulsion->in.alpha            = Auxiliary->Getalpha();
    Propulsion->in.beta             = Auxiliary->Getbeta();
    Propulsion->in.TotalDeltaT      = dT * Propulsion->GetRate();
    Propulsion->in.ThrottlePos      = FCS->GetThrottlePos();
    Propulsion->in.MixturePos       = FCS->GetMixturePos();
    Propulsion->in.ThrottleCmd      = FCS->GetThrottleCmd();
    Propulsion->in.MixtureCmd       = FCS->GetMixtureCmd();
    Propulsion->in.PropAdvance      = FCS->GetPropAdvance();
    Propulsion->in.PropFeather      = FCS->GetPropFeather();
    Propulsion->in.H_agl            = Propagate->GetDistanceAGL();
    Propulsion->in.PQRi             = Propagate->GetPQRi();

    break;
  case eAerodynamics:
    Aerodynamics->in.Alpha     = Auxiliary->Getalpha();
    Aerodynamics->in.Beta      = Auxiliary->Getbeta();
    Aerodynamics->in.Qbar      = Auxiliary->Getqbar();
    Aerodynamics->in.Vt        = Auxiliary->GetVt();
    Aerodynamics->in.Tb2w      = Auxiliary->GetTb2w();
    Aerodynamics->in.Tw2b      = Auxiliary->GetTw2b();
    Aerodynamics->in.RPBody    = MassBalance->StructuralToBody(Aircraft->GetXYZrp());
    break;
  case eGroundReactions:
    // There are no external inputs to this model.
    GroundReactions->in.Vground         = Auxiliary->GetVground();
    GroundReactions->in.VcalibratedKts  = Auxiliary->GetVcalibratedKTS();
    GroundReactions->in.Temperature     = Atmosphere->GetTemperature();
    GroundReactions->in.TakeoffThrottle = (FCS->GetThrottlePos().size() > 0) ? (FCS->GetThrottlePos(0) > 0.90) : false;
    GroundReactions->in.BrakePos        = FCS->GetBrakePos();
    GroundReactions->in.FCSGearPos      = FCS->GetGearPos();
    GroundReactions->in.EmptyWeight     = MassBalance->GetEmptyWeight();
    GroundReactions->in.Tb2l            = Propagate->GetTb2l();
    GroundReactions->in.Tec2l           = Propagate->GetTec2l();
    GroundReactions->in.Tec2b           = Propagate->GetTec2b();
    GroundReactions->in.PQR             = Propagate->GetPQR();
    GroundReactions->in.UVW             = Propagate->GetUVW();
    GroundReactions->in.DistanceAGL     = Propagate->GetDistanceAGL();
    GroundReactions->in.DistanceASL     = Propagate->GetAltitudeASL();
    GroundReactions->in.TotalDeltaT     = dT * GroundReactions->GetRate();
    GroundReactions->in.WOW             = GroundReactions->GetWOW();
    GroundReactions->in.Location        = Propagate->GetLocation();
    GroundReactions->in.vXYZcg          = MassBalance->GetXYZcg();
    break;
  case eExternalReactions:
    // There are no external inputs to this model.
    break;
  case eBuoyantForces:
    BuoyantForces->in.Density     = Atmosphere->GetDensity();
    BuoyantForces->in.Pressure    = Atmosphere->GetPressure();
    BuoyantForces->in.Temperature = Atmosphere->GetTemperature();
    BuoyantForces->in.gravity     = Inertial->GetGravity().Magnitude();
    break;
  case eMassBalance:
    MassBalance->in.GasInertia  = BuoyantForces->GetGasMassInertia();
    MassBalance->in.GasMass     = BuoyantForces->GetGasMass();
    MassBalance->in.GasMoment   = BuoyantForces->GetGasMassMoment();
    MassBalance->in.TanksWeight = Propulsion->GetTanksWeight();
    MassBalance->in.TanksMoment = Propulsion->GetTanksMoment();
    MassBalance->in.TankInertia = Propulsion->CalculateTankInertias();
    MassBalance->in.WOW         = GroundReactions->GetWOW();
    break;
  case eAircraft:
    Aircraft->in.AeroForce     = Aerodynamics->GetForces();
    Aircraft->in.PropForce     = Propulsion->GetForces();
    Aircraft->in.GroundForce   = GroundReactions->GetForces();
    Aircraft->in.ExternalForce = ExternalReactions->GetForces();
    Aircraft->in.BuoyantForce  = BuoyantForces->GetForces();
    Aircraft->in.AeroMoment    = Aerodynamics->GetMoments();
    Aircraft->in.PropMoment    = Propulsion->GetMoments();
    Aircraft->in.GroundMoment  = GroundReactions->GetMoments();
    Aircraft->in.ExternalMoment = ExternalReactions->GetMoments();
    Aircraft->in.BuoyantMoment = BuoyantForces->GetMoments();
    break;
  case eAccelerations:
    Accelerations->in.J        = MassBalance->GetJ();
    Accelerations->in.Jinv     = MassBalance->GetJinv();
    Accelerations->in.Ti2b     = Propagate->GetTi2b();
    Accelerations->in.Tb2i     = Propagate->GetTb2i();
    Accelerations->in.Tec2b    = Propagate->GetTec2b();
    Accelerations->in.Tec2i    = Propagate->GetTec2i();
    Accelerations->in.Moment   = Aircraft->GetMoments();
    Accelerations->in.GroundMoment  = GroundReactions->GetMoments();
    Accelerations->in.Force    = Aircraft->GetForces();
    Accelerations->in.GroundForce   = GroundReactions->GetForces();
    Accelerations->in.vGravAccel = Inertial->GetGravity();
    Accelerations->in.vPQRi    = Propagate->GetPQRi();
    Accelerations->in.vPQR     = Propagate->GetPQR();
    Accelerations->in.vUVW     = Propagate->GetUVW();
    Accelerations->in.vInertialPosition = Propagate->GetInertialPosition();
    Accelerations->in.DeltaT   = dT;
    Accelerations->in.Mass     = MassBalance->GetMass();
    Accelerations->in.MultipliersList = GroundReactions->GetMultipliersList();
    Accelerations->in.TerrainVelocity = Propagate->GetTerrainVelocity();
    Accelerations->in.TerrainAngularVel = Propagate->GetTerrainAngularVelocity();
    break;
  default:
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::LoadPlanetConstants(void)
{
  Propagate->in.vOmegaPlanet     = Inertial->GetOmegaPlanet();
  Accelerations->in.vOmegaPlanet = Inertial->GetOmegaPlanet();
  Propagate->in.SemiMajor        = Inertial->GetSemimajor();
  Propagate->in.SemiMinor        = Inertial->GetSemiminor();
  Propagate->in.GM               = Inertial->GetGM();
  Auxiliary->in.StandardGravity  = Inertial->GetStandardGravity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::LoadModelConstants(void)
{
  Winds->in.wingspan             = Aircraft->GetWingSpan();
  Aerodynamics->in.Wingarea      = Aircraft->GetWingArea();
  Aerodynamics->in.Wingchord     = Aircraft->Getcbar();
  Aerodynamics->in.Wingincidence = Aircraft->GetWingIncidence();
  Aerodynamics->in.Wingspan      = Aircraft->GetWingSpan();
  Auxiliary->in.Wingspan         = Aircraft->GetWingSpan();
  Auxiliary->in.Wingchord        = Aircraft->Getcbar();
  GroundReactions->in.vXYZcg     = MassBalance->GetXYZcg();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This call will cause the sim time to reset to 0.0

bool FGFDMExec::RunIC(void)
{
  SuspendIntegration(); // saves the integration rate, dt, then sets it to 0.0.
  Initialize(IC.get());

  Models[eInput]->InitModel();
  Models[eOutput]->InitModel();

  Run();
  Propagate->InitializeDerivatives();
  ResumeIntegration(); // Restores the integration rate to what it was.

  if (debug_lvl > 0) {
    MassBalance->GetMassPropertiesReport(0);

    cout << endl << fgblue << highint
         << "End of vehicle configuration loading." << endl
         << "-------------------------------------------------------------------------------"
         << reset << std::setprecision(6) << endl;
  }

  for (unsigned int n=0; n < Propulsion->GetNumEngines(); ++n) {
    if (IC->IsEngineRunning(n)) {
      try {
        Propulsion->InitRunning(n);
      } catch (const string& str) {
        cerr << str << endl;
        return false;
      }
    }
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::Initialize(const FGInitialCondition* FGIC)
{
  Propagate->SetInitialState(FGIC);
  Winds->SetWindNED(FGIC->GetWindNEDFpsIC());
  Run();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::ResetToInitialConditions(int mode)
{
  if (Constructing) return;

  // mode flags

  if (mode & START_NEW_OUTPUT) Output->SetStartNewOutput();

  InitializeModels();

  if (Script)
    Script->ResetEvents();
  else
    Setsim_time(0.0);

  if (!(mode & DONT_EXECUTE_RUN_IC))
    RunIC();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::SetHoldDown(bool hd)
{
  HoldDown = hd;
  Accelerations->SetHoldDown(hd);
  if (hd) {
    Propagate->in.vPQRidot = Accelerations->GetPQRidot();
    Propagate->in.vUVWidot = Accelerations->GetUVWidot();
  }
  Propagate->SetHoldDown(hd);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

vector <string> FGFDMExec::EnumerateFDMs(void)
{
  vector <string> FDMList;
  FDMList.push_back(Aircraft->GetAircraftName());

  for (auto &ChildFDM: ChildFDMList)
    FDMList.push_back(ChildFDM->exec->GetAircraft()->GetAircraftName());

  return FDMList;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::LoadScript(const SGPath& script, double deltaT,
                           const SGPath& initfile)
{
  Script = std::make_shared<FGScript>(this);
  return Script->LoadScript(GetFullPath(script), deltaT, initfile);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::LoadModel(const SGPath& AircraftPath, const SGPath& EnginePath,
                          const SGPath& SystemsPath, const string& model,
                          bool addModelToPath)
{
  FGFDMExec::AircraftPath = GetFullPath(AircraftPath);
  FGFDMExec::EnginePath = GetFullPath(EnginePath);
  FGFDMExec::SystemsPath = GetFullPath(SystemsPath);

  return LoadModel(model, addModelToPath);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::LoadModel(const string& model, bool addModelToPath)
{
  SGPath aircraftCfgFileName;
  bool result = false; // initialize result to false, indicating input file not yet read

  modelName = model; // Set the class modelName attribute

  if( AircraftPath.isNull() || EnginePath.isNull() || SystemsPath.isNull()) {
    cerr << "Error: attempted to load aircraft with undefined "
         << "aircraft, engine, and system paths" << endl;
    return false;
  }

  FullAircraftPath = AircraftPath;
  if (addModelToPath) FullAircraftPath.append(model);
  aircraftCfgFileName = FullAircraftPath/(model + ".xml");

  if (modelLoaded) {
    DeAllocate();
    Allocate();
  }

  int saved_debug_lvl = debug_lvl;
  FGXMLFileRead XMLFileRead;
  Element *document = XMLFileRead.LoadXMLDocument(aircraftCfgFileName); // "document" is a class member

  if (document) {
    if (IsChild) debug_lvl = 0;

    ReadPrologue(document);

    if (IsChild) debug_lvl = saved_debug_lvl;

    // Process the fileheader element in the aircraft config file. This element is OPTIONAL.
    Element* element = document->FindElement("fileheader");
    if (element) {
      result = ReadFileHeader(element);
      if (!result) {
        cerr << endl << "Aircraft fileheader element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    }

    if (IsChild) debug_lvl = 0;

    // Process the planet element. This element is OPTIONAL.
    element = document->FindElement("planet");
    if (element) {
      result = Models[eInertial]->Load(element);
      if (!result) {
        cerr << endl << "Planet element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
      // Reload the planet constants and re-initialize the models.
      LoadPlanetConstants();
      IC->InitializeIC();
      InitializeModels();
    }

    // Process the metrics element. This element is REQUIRED.
    element = document->FindElement("metrics");
    if (element) {
      result = Models[eAircraft]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft metrics element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    } else {
      cerr << endl << "No metrics element was found in the aircraft config file." << endl;
      return false;
    }

    // Process the mass_balance element. This element is REQUIRED.
    element = document->FindElement("mass_balance");
    if (element) {
      result = Models[eMassBalance]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft mass_balance element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    } else {
      cerr << endl << "No mass_balance element was found in the aircraft config file." << endl;
      return false;
    }

    // Process the ground_reactions element. This element is REQUIRED.
    element = document->FindElement("ground_reactions");
    if (element) {
      result = Models[eGroundReactions]->Load(element);
      if (!result) {
        cerr << endl << element->ReadFrom()
             << "Aircraft ground_reactions element has problems in file "
             << aircraftCfgFileName << endl;
        return result;
      }
    } else {
      cerr << endl << "No ground_reactions element was found in the aircraft config file." << endl;
      return false;
    }

    // Process the external_reactions element. This element is OPTIONAL.
    element = document->FindElement("external_reactions");
    if (element) {
      result = Models[eExternalReactions]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft external_reactions element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    }

    // Process the buoyant_forces element. This element is OPTIONAL.
    element = document->FindElement("buoyant_forces");
    if (element) {
      result = Models[eBuoyantForces]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft buoyant_forces element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    }

    // Process the propulsion element. This element is OPTIONAL.
    element = document->FindElement("propulsion");
    if (element) {
      result = Propulsion->Load(element);
      if (!result) {
        cerr << endl << "Aircraft propulsion element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
      for (unsigned int i=0; i < Propulsion->GetNumEngines(); i++)
        FCS->AddThrottle();
    }

    // Process the system element[s]. This element is OPTIONAL, and there may be more than one.
    element = document->FindElement("system");
    while (element) {
      result = Models[eSystems]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft system element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
      element = document->FindNextElement("system");
    }

    // Process the autopilot element. This element is OPTIONAL.
    element = document->FindElement("autopilot");
    if (element) {
      result = Models[eSystems]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft autopilot element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    }

    // Process the flight_control element. This element is OPTIONAL.
    element = document->FindElement("flight_control");
    if (element) {
      result = Models[eSystems]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft flight_control element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    }

    // Process the aerodynamics element. This element is OPTIONAL, but almost always expected.
    element = document->FindElement("aerodynamics");
    if (element) {
      result = Models[eAerodynamics]->Load(element);
      if (!result) {
        cerr << endl << "Aircraft aerodynamics element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    } else {
      cerr << endl << "No expected aerodynamics element was found in the aircraft config file." << endl;
    }

    // Process the input element. This element is OPTIONAL, and there may be more than one.
    element = document->FindElement("input");
    while (element) {
      if (!Input->Load(element))
        return false;

      element = document->FindNextElement("input");
    }

    // Process the output element[s]. This element is OPTIONAL, and there may be
    // more than one.
    element = document->FindElement("output");
    while (element) {
      if (!Output->Load(element))
        return false;

      element = document->FindNextElement("output");
    }

    // Lastly, process the child element. This element is OPTIONAL - and NOT YET SUPPORTED.
    element = document->FindElement("child");
    if (element) {
      result = ReadChild(element);
      if (!result) {
        cerr << endl << "Aircraft child element has problems in file " << aircraftCfgFileName << endl;
        return result;
      }
    }

    // Since all vehicle characteristics have been loaded, place the values in the Inputs
    // structure for the FGModel-derived classes.
    LoadModelConstants();

    modelLoaded = true;

    if (IsChild) debug_lvl = saved_debug_lvl;

  } else {
    cerr << fgred
         << "  JSBSim failed to open the configuration file: " << aircraftCfgFileName
         << fgdef << endl;
  }

  for (unsigned int i=0; i< Models.size(); i++) LoadInputs(i);

  if (result) {
    struct PropertyCatalogStructure masterPCS;
    masterPCS.base_string = "";
    masterPCS.node = Root;
    BuildPropertyCatalog(&masterPCS);
  }

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFDMExec::GetPropulsionTankReport() const
{
  return Propulsion->GetPropulsionTankReport();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::BuildPropertyCatalog(struct PropertyCatalogStructure* pcs)
{
  auto pcsNew = std::make_unique<struct PropertyCatalogStructure>();

  for (int i=0; i<pcs->node->nChildren(); i++) {
    string access="";
    pcsNew->base_string = pcs->base_string + "/" + pcs->node->getChild(i)->getNameString();
    int node_idx = pcs->node->getChild(i)->getIndex();
    if (node_idx != 0) {
      pcsNew->base_string = CreateIndexedPropertyName(pcsNew->base_string, node_idx);
    }
    if (pcs->node->getChild(i)->nChildren() == 0) {
      if (pcsNew->base_string.substr(0,12) == string("/fdm/jsbsim/")) {
        pcsNew->base_string = pcsNew->base_string.erase(0,12);
      }
      if (pcs->node->getChild(i)->getAttribute(SGPropertyNode::READ)) access="R";
      if (pcs->node->getChild(i)->getAttribute(SGPropertyNode::WRITE)) access+="W";
      PropertyCatalog.push_back(pcsNew->base_string+" ("+access+")");
    } else {
      pcsNew->node = (FGPropertyNode*)pcs->node->getChild(i);
      BuildPropertyCatalog(pcsNew.get());
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFDMExec::QueryPropertyCatalog(const string& in, const string& end_of_line)
{
  string results;
  for (auto &catalogElm: PropertyCatalog) {
    if (catalogElm.find(in) != string::npos) results += catalogElm + end_of_line;
  }
  if (results.empty()) return "No matches found"+end_of_line;
  return results;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::PrintPropertyCatalog(void)
{
  cout << endl;
  cout << "  " << fgblue << highint << underon << "Property Catalog for "
       << modelName << reset << endl << endl;
  for (auto &catalogElm: PropertyCatalog)
    cout << "    " << catalogElm << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::PrintSimulationConfiguration(void) const
{
  cout << endl << "Simulation Configuration" << endl << "------------------------" << endl;
  cout << MassBalance->GetName() << endl;
  cout << GroundReactions->GetName() << endl;
  cout << Aerodynamics->GetName() << endl;
  cout << Propulsion->GetName() << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadFileHeader(Element* el)
{
  bool result = true; // true for success

  if (debug_lvl == 0) return result;

  if (IsChild) {
    cout << endl <<highint << fgblue << "Reading child model: " << IdFDM << reset << endl << endl;
  }

  if (el->FindElement("description"))
    cout << "  Description:   " << el->FindElement("description")->GetDataLine() << endl;
  if (el->FindElement("author"))
    cout << "  Model Author:  " << el->FindElement("author")->GetDataLine() << endl;
  if (el->FindElement("filecreationdate"))
    cout << "  Creation Date: " << el->FindElement("filecreationdate")->GetDataLine() << endl;
  if (el->FindElement("version"))
    cout << "  Version:       " << el->FindElement("version")->GetDataLine() << endl;

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadPrologue(Element* el) // el for ReadPrologue is the document element
{
  bool result = true; // true for success

  if (!el) return false;

  string AircraftName = el->GetAttributeValue("name");
  Aircraft->SetAircraftName(AircraftName);

  if (debug_lvl & 1) cout << underon << "Reading Aircraft Configuration File"
            << underoff << ": " << highint << AircraftName << normint << endl;

  CFGVersion = el->GetAttributeValue("version");
  Release    = el->GetAttributeValue("release");

  if (debug_lvl & 1)
    cout << "                            Version: " << highint << CFGVersion
                                                    << normint << endl;
  if (CFGVersion != needed_cfg_version) {
    cerr << endl << fgred << "YOU HAVE AN INCOMPATIBLE CFG FILE FOR THIS AIRCRAFT."
            " RESULTS WILL BE UNPREDICTABLE !!" << endl;
    cerr << "Current version needed is: " << needed_cfg_version << endl;
    cerr << "         You have version: " << CFGVersion << endl << fgdef << endl;
    return false;
  }

  if (Release == "ALPHA" && (debug_lvl & 1)) {
    cout << endl << endl
         << highint << "This aircraft model is an " << fgred << Release
         << reset << highint << " release!!!" << endl << endl << reset
         << "This aircraft model may not even properly load, and probably"
         << " will not fly as expected." << endl << endl
         << fgred << highint << "Use this model for development purposes ONLY!!!"
         << normint << reset << endl << endl;
  } else if (Release == "BETA" && (debug_lvl & 1)) {
    cout << endl << endl
         << highint << "This aircraft model is a " << fgred << Release
         << reset << highint << " release!!!" << endl << endl << reset
         << "This aircraft model probably will not fly as expected." << endl << endl
         << fgblue << highint << "Use this model for development purposes ONLY!!!"
         << normint << reset << endl << endl;
  } else if (Release == "PRODUCTION" && (debug_lvl & 1)) {
    cout << endl << endl
         << highint << "This aircraft model is a " << fgblue << Release
         << reset << highint << " release." << endl << endl << reset;
  } else if (debug_lvl & 1) {
    cout << endl << endl
         << highint << "This aircraft model is an " << fgred << Release
         << reset << highint << " release!!!" << endl << endl << reset
         << "This aircraft model may not even properly load, and probably"
         << " will not fly as expected." << endl << endl
         << fgred << highint << "Use this model for development purposes ONLY!!!"
         << normint << reset << endl << endl;
  }

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadChild(Element* el)
{
  // Add a new childData object to the child FDM list
  // Populate that childData element with a new FDMExec object
  // Set the IsChild flag for that FDMExec object
  // Get the aircraft name
  // set debug level to print out no additional data for child objects
  // Load the model given the aircraft name
  // reset debug level to prior setting

  auto child = std::make_shared<childData>();

  auto pm = std::make_unique<FGPropertyManager>(Root);
  child->exec = std::make_unique<FGFDMExec>(pm.get(), FDMctr);
  child->exec->SetChild(true);

  string childAircraft = el->GetAttributeValue("name");
  string sMated = el->GetAttributeValue("mated");
  if (sMated == "false") child->mated = false; // child objects are mated by default.
  string sInternal = el->GetAttributeValue("internal");
  if (sInternal == "true") child->internal = true; // child objects are external by default.

  child->exec->SetAircraftPath( AircraftPath );
  child->exec->SetEnginePath( EnginePath );
  child->exec->SetSystemsPath( SystemsPath );
  child->exec->LoadModel(childAircraft);

  Element* location = el->FindElement("location");
  if (location) {
    child->Loc = location->FindElementTripletConvertTo("IN");
  } else {
    const string s("  No location was found for this child object!");
    cerr << el->ReadFrom() << endl << highint << fgred
         << s << reset << endl;
    throw BaseException(s);
  }

  Element* orientation = el->FindElement("orient");
  if (orientation) {
    child->Orient = orientation->FindElementTripletConvertTo("RAD");
  } else if (debug_lvl > 0) {
    cerr << endl << highint << "  No orientation was found for this child object! Assuming 0,0,0." << reset << endl;
  }

  ChildFDMList.push_back(child);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::shared_ptr<FGTrim> FGFDMExec::GetTrim(void)
{
  Trim = std::make_shared<FGTrim>(this,tNone);
  return Trim;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::CheckIncrementalHold(void)
{
  // Only check if increment then hold is on
  if( IncrementThenHolding ) {

    if (TimeStepsUntilHold == 0) {

      // Should hold simulation if TimeStepsUntilHold has reached zero
      holding = true;

      // Still need to decrement TimeStepsUntilHold as value of -1
      // indicates that incremental then hold is turned off
      IncrementThenHolding = false;
      TimeStepsUntilHold--;

    } else if ( TimeStepsUntilHold > 0 ) {
      // Keep decrementing until 0 is reached
      TimeStepsUntilHold--;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::DoTrim(int mode)
{
  if (Constructing) return;

  if (mode < 0 || mode > JSBSim::tNone)
    throw("Illegal trimming mode!");

  FGTrim trim(this, (JSBSim::TrimMode)mode);
  bool success = trim.DoTrim();

  if (debug_lvl > 0)
    trim.Report();

  if (!success)
    throw TrimFailureException("Trim Failed");

  trim_completed = 1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::SRand(int sr)
{
  RandomSeed = sr;
  RandomGenerator->seed(RandomSeed);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGFDMExec::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1 && IdFDM == 0) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "\n\n     "
           << "JSBSim Flight Dynamics Model v" << JSBSim_version << endl;
      cout << "            [JSBSim-ML v" << needed_cfg_version << "]\n\n";
      cout << "JSBSim startup beginning ...\n\n";
      if (disperse == 1) cout << "Dispersions are ON." << endl << endl;
    } else if (from == 3) {
      cout << "\n\nJSBSim startup complete\n\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFDMExec" << endl;
    if (from == 1) cout << "Destroyed:    FGFDMExec" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
    if (from == 2) {
      cout << "================== Frame: " << Frame << "  Time: "
           << sim_time << " dt: " << dT << endl;
    }
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
