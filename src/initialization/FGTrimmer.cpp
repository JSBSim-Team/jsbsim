/*
 * FGTrimmer.cpp
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGTrimmer.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGTrimmer.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FGTrimmer.h"
#include "models/FGFCS.h"
#include "models/FGPropulsion.h"
#include "models/FGAccelerations.h"
#include "models/propulsion/FGEngine.h"
#include "models/propulsion/FGThruster.h"
#include "models/propulsion/FGTank.h"
#include "models/FGMassBalance.h"
#include "models/FGAuxiliary.h"
#include "models/FGAircraft.h"
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
#include "simgear/misc/stdint.hxx"

namespace JSBSim
{

FGTrimmer::FGTrimmer(FGFDMExec * fdm, Constraints * constraints) :
        m_fdm(fdm), m_constraints(constraints)
{
}

std::vector<double> FGTrimmer::constrain(const std::vector<double> & v)
{
    // unpack design vector
    double throttle = v[0];
    double elevator = v[1];
    double alpha = v[2];
    double aileron = v[3];
    double rudder = v[4];
    double beta = v[5];

    // initialize constraints
    double vt = m_constraints->velocity;
    double altitude = m_constraints->altitude;
    double phi = 0.0, theta = 0.0, psi = 0.0*M_PI/180.0;
    double p = 0.0, q = 0.0, r= 0.0;

    // precomputation
    double sGam = sin(m_constraints->gamma);
    double sBeta = sin(beta);
    double cBeta = cos(beta);
    double tAlpha = tan(alpha);
    double cAlpha = cos(alpha);

    // rate of climb constraint
    double a = cAlpha*cBeta;
    double b = sin(phi)*sBeta+cos(phi)*sin(alpha)*cBeta;
    theta = atan((a*b+sGam*sqrt(a*a-sGam*sGam+b*b))/(a*a-sGam*sGam));

    // turn coordination constraint, lewis pg. 190
    double gd = m_fdm->GetInertial()->gravity();
    double gc = m_constraints->yawRate*vt/gd;
    a = 1 - gc*tAlpha*sBeta;
    b = sGam/cBeta;
    double c = 1 + gc*gc*cBeta*cBeta;
    phi = atan((gc*cBeta*((a-b*b)+
                b*tAlpha*sqrt(c*(1-b*b)+gc*gc*sBeta*sBeta)))/
               (cAlpha*(a*a-b*b*(1+c*tAlpha*tAlpha))));

    // turn rates
    if (m_constraints->rollRate != 0.0) // rolling
    {
        p = m_constraints->rollRate;
        q = 0.0;
        if (m_constraints->stabAxisRoll) // stability axis roll
        {
            r = m_constraints->rollRate*tan(alpha);
        }
        else // body axis roll
        {
            r = m_constraints->rollRate;
        }
    }
    else if (m_constraints->yawRate != 0.0) // yawing
    {
        p = -m_constraints->yawRate*sin(theta);
        q = m_constraints->yawRate*sin(phi)*cos(theta);
        r = m_constraints->yawRate*cos(phi)*cos(theta);
    }
    else if (m_constraints->pitchRate != 0.0) // pitching
    {
        p = 0.0;
        q = m_constraints->pitchRate;
        r = 0.0;
    }

    // state
    m_fdm->GetIC()->SetVtrueFpsIC(vt);
    m_fdm->GetIC()->SetAlphaRadIC(alpha);
    m_fdm->GetIC()->SetThetaRadIC(theta);
    m_fdm->GetIC()->SetFlightPathAngleRadIC(m_constraints->gamma);
    m_fdm->GetIC()->SetQRadpsIC(q);
    // thrust handled below
    m_fdm->GetIC()->SetBetaRadIC(beta);
    m_fdm->GetIC()->SetPhiRadIC(phi);
    m_fdm->GetIC()->SetPRadpsIC(p);
    m_fdm->GetIC()->SetRRadpsIC(r);

    // actuator states handled below

    // nav state
    m_fdm->GetIC()->SetAltitudeASLFtIC(altitude);
    m_fdm->GetIC()->SetPsiRadIC(psi);
    //m_fdm->GetIC()->SetLatitudeRadIC(0);
    //m_fdm->GetIC()->SetLongitudeRadIC(0);

    // apply state
    m_fdm->RunIC();

    // set controls
    m_fdm->GetFCS()->SetDeCmd(elevator);
    m_fdm->GetFCS()->SetDePos(ofNorm,elevator);

    m_fdm->GetFCS()->SetDaCmd(aileron);
    m_fdm->GetFCS()->SetDaLPos(ofNorm,aileron);
    m_fdm->GetFCS()->SetDaRPos(ofNorm,aileron);

    m_fdm->GetFCS()->SetDrPos(ofNorm,rudder);
    m_fdm->GetFCS()->SetDrCmd(rudder);

    for (int i=0; i<m_fdm->GetPropulsion()->GetNumEngines(); i++)
    {
        FGEngine * engine = m_fdm->GetPropulsion()->GetEngine(i);
        m_fdm->GetPropulsion()->GetEngine(i)->InitRunning();
        m_fdm->GetFCS()->SetThrottleCmd(i,throttle);
        m_fdm->GetFCS()->SetThrottlePos(i,throttle);
    }


    // wait for steady-state
    //double thrust0 = m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetThrust();
    //double dThrustMag0 = 0;
    //for(int i=0;;i++) {
        //m_fdm->RunIC();
        //m_fdm->Run();
        //double thrust = m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetThrust();
        //double dThrustMag = std::abs(thrust - thrust0);
        //double d2Thrust = dThrustMag - dThrustMag0;
        //thrust0= thrust;
        //dThrustMag0 = dThrustMag;
        //if (d2Thrust < std::numeric_limits<double>::epsilon() ) {
            //// thrust difference has converged to minimum
            //// if d2Thrust > 0 clearly, more interations won't help
            //// so not using abs(d2Thrust)
            //break;
        //} else if (i> 1000) {
            //std::cout << "thrust failed to converge" << std::endl;
            //std::cout << "difference: " << dThrustMag << std::endl;
            //throw std::runtime_error("thrust failed to converge");
            //break;
        //}
    //}
    //m_fdm->RunIC();

    std::vector<double> data;
    data.push_back(phi);
    data.push_back(theta);
    return data;
}

void FGTrimmer::printSolution(std::ostream & stream, const std::vector<double> & v)
{
    eval(v);

    double dt = m_fdm->GetDeltaT();
    double thrust = m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetThrust();
    double elevator = m_fdm->GetFCS()->GetDePos(ofNorm);
    double aileron = m_fdm->GetFCS()->GetDaLPos(ofNorm);
    double rudder = m_fdm->GetFCS()->GetDrPos(ofNorm);
    double throttle = m_fdm->GetFCS()->GetThrottlePos(0);
    double lat = m_fdm->GetPropagate()->GetLatitudeDeg();
    double lon = m_fdm->GetPropagate()->GetLongitudeDeg();
    double vt = m_fdm->GetAuxiliary()->GetVt();

    // run a step to compute derivatives
    //for (int i=0;i<10000;i++) {
        //while (old - m_fdm->GetPropulsion()->GetEngine(i)->CalcFuelNeed() < 1e-5) {
            //m_fdm->RunIC();
            //m_fdm->Run();
        //}
    //}

    double dthrust = (m_fdm->GetPropulsion()->GetEngine(0)->
            GetThruster()->GetThrust()-thrust)/dt;
    double delevator = (m_fdm->GetFCS()->GetDePos(ofNorm)-elevator)/dt;
    double daileron = (m_fdm->GetFCS()->GetDaLPos(ofNorm)-aileron)/dt;
    double drudder = (m_fdm->GetFCS()->GetDrPos(ofNorm)-rudder)/dt;
    double dthrottle = (m_fdm->GetFCS()->GetThrottlePos(0)-throttle)/dt;
    double dlat = (m_fdm->GetPropagate()->GetLatitudeDeg()-lat)/dt;
    double dlon = (m_fdm->GetPropagate()->GetLongitudeDeg()-lon)/dt;
    double dvt = (m_fdm->GetAuxiliary()->GetVt()-vt)/dt;

    // reinitialize with correct state
    eval(v);

    // state
    stream << std::setw(10)

              // aircraft state
              << "\naircraft state"
              << "\n\tvt\t\t:\t" << vt
              << "\n\talpha, deg\t:\t" << m_fdm->GetIC()->GetAlphaDegIC()
              << "\n\ttheta, deg\t:\t" << m_fdm->GetIC()->GetThetaDegIC()
              << "\n\tq, rad/s\t:\t" << m_fdm->GetIC()->GetQRadpsIC()
              << "\n\tthrust, lbf\t:\t" << m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetThrust()
              << "\n\tbeta, deg\t:\t" << m_fdm->GetIC()->GetBetaDegIC()
              << "\n\tphi, deg\t:\t" << m_fdm->GetIC()->GetPhiDegIC()
              << "\n\tp, rad/s\t:\t" << m_fdm->GetIC()->GetPRadpsIC()
              << "\n\tr, rad/s\t:\t" << m_fdm->GetIC()->GetRRadpsIC()
              << "\n\tmass (lbm)\t:\t" << m_fdm->GetMassBalance()->GetWeight()

              // actuator states
              << "\n\nactuator state"
              << "\n\tthrottle, %\t:\t" << 100*throttle
              << "\n\televator, %\t:\t" << 100*elevator
              << "\n\taileron, %\t:\t" << 100*aileron
              << "\n\trudder, %\t:\t" << 100*rudder

              // nav state
              << "\n\nnav state"
              << "\n\taltitude, ft\t:\t" << m_fdm->GetIC()->GetAltitudeASLFtIC()
              << "\n\tpsi, deg\t:\t" << m_fdm->GetIC()->GetPsiDegIC()
              << "\n\tlat, deg\t:\t" << lat
              << "\n\tlon, deg\t:\t" << lon

              // d/dt aircraft state
              << "\n\naircraft d/dt state"
              << std::scientific

              << "\n\td/dt vt\t\t\t:\t" << dvt
              << "\n\td/dt alpha, deg/s\t:\t" << m_fdm->GetAuxiliary()->Getadot()*180/M_PI
              << "\n\td/dt theta, deg/s\t:\t" << m_fdm->GetAuxiliary()->GetEulerRates(2)*180/M_PI
              << "\n\td/dt q, rad/s^2\t\t:\t" << m_fdm->GetAccelerations()->GetPQRdot(2)
              << "\n\td/dt thrust, lbf\t:\t" << dthrust
              << "\n\td/dt beta, deg/s\t:\t" << m_fdm->GetAuxiliary()->Getbdot()*180/M_PI
              << "\n\td/dt phi, deg/s\t\t:\t" << m_fdm->GetAuxiliary()->GetEulerRates(1)*180/M_PI
              << "\n\td/dt p, rad/s^2\t\t:\t" << m_fdm->GetAccelerations()->GetPQRdot(1)
              << "\n\td/dt r, rad/s^2\t\t:\t" << m_fdm->GetAccelerations()->GetPQRdot(3)

              // d/dt actuator states
              << "\n\nd/dt actuator state"
              << "\n\td/dt throttle, %/s\t:\t" << dthrottle
              << "\n\td/dt elevator, %/s\t:\t" << delevator
              << "\n\td/dt aileron, %/s\t:\t" << daileron
              << "\n\td/dt rudder, %/s\t:\t" << drudder

              // nav state
              << "\n\nd/dt nav state"
              << "\n\td/dt altitude, ft/s\t:\t" << m_fdm->GetPropagate()->Gethdot()
              << "\n\td/dt psi, deg/s\t\t:\t" << m_fdm->GetAuxiliary()->GetEulerRates(3)
              << "\n\td/dt lat, deg/s\t\t:\t" << dlat
              << "\n\td/dt lon, deg/s\t\t:\t" << dlon
              << std::fixed

              << "\n\npropulsion system state"
              << std::scientific << std::setw(10);

              for (int i=0;i<m_fdm->GetPropulsion()->GetNumTanks();i++) {
                  stream 
                    << "\n\ttank " << i << ": fuel (lbm)\t\t\t:\t" 
                    << m_fdm->GetPropulsion()->GetTank(i)->GetContents();
              }

              for (int i=0;i<m_fdm->GetPropulsion()->GetNumEngines();i++) {
                  m_fdm->GetPropulsion()->GetEngine(i)->CalcFuelNeed();
                  stream
                    << "\n\tengine " << i
                    << "\n\t\tfuel flow rate (lbm/s)\t\t:\t" << m_fdm->GetPropulsion()->GetEngine(i)->GetFuelFlowRate()
                    << "\n\t\tfuel flow rate (gph)\t\t:\t" << m_fdm->GetPropulsion()->GetEngine(i)->GetFuelFlowRateGPH()
                    << "\n\t\tstarved\t\t\t\t:\t" << m_fdm->GetPropulsion()->GetEngine(i)->GetStarved()
                    << "\n\t\trunning\t\t\t\t:\t" << m_fdm->GetPropulsion()->GetEngine(i)->GetRunning()
                    << std::endl;
              }
}

void FGTrimmer::printState(std::ostream & stream)
{
    // state
    stream << std::setw(10)

              // interval method comparison
              //<< "\n\ninterval method comparison"
              //<< "\nAngle of Attack: \t:\t" << m_fdm->GetAuxiliary()->Getalpha(ofDeg) << "\twdot: " << m_fdm->GetAccelerations()->GetUVWdot(3)
              //<< "\nThrottle: \t:\t" << 100*m_fdm->GetFCS()->GetThrottlePos(0) << "\tudot: " << m_fdm->GetAccelerations()->GetUVWdot(1)
              //<< "\nPitch Trim: \t:\t" << 100*m_fdm->GetFCS()->GetDePos(ofNorm) << "\tqdot: " << m_fdm->GetAccelerations()->GetPQRdot(2)
              //<< "\nSideslip: \t:\t" << m_fdm->GetAuxiliary()->Getbeta(ofDeg) << "\tvdot: " << m_fdm->GetAccelerations()->GetUVWdot(2)
              //<< "\nAilerons: \t:\t" << 100*m_fdm->GetFCS()->GetDaLPos(ofNorm) << "\tpdot: " << m_fdm->GetAccelerations()->GetPQRdot(1)
              //<< "\nRudder: \t:\t" << 100*m_fdm->GetFCS()->GetDrPos(ofNorm) << "\trdot: " << m_fdm->GetAccelerations()->GetPQRdot(3)

              << "\n\naircraft state"
              << "\nvt\t\t:\t" << m_fdm->GetAuxiliary()->GetVt()
              << "\nalpha, deg\t:\t" << m_fdm->GetAuxiliary()->Getalpha(ofDeg)
              << "\ntheta, deg\t:\t" << m_fdm->GetPropagate()->GetEuler(2)*180/M_PI
              << "\nq, rad/s\t:\t" << m_fdm->GetPropagate()->GetPQR(2)
              << "\nthrust, lbf\t:\t" << m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetThrust()
              << "\nbeta, deg\t:\t" << m_fdm->GetAuxiliary()->Getbeta(ofDeg)
              << "\nphi, deg\t:\t" << m_fdm->GetPropagate()->GetEuler(1)*180/M_PI
              << "\np, rad/s\t:\t" << m_fdm->GetPropagate()->GetPQR(1)
              << "\nr, rad/s\t:\t" << m_fdm->GetPropagate()->GetPQR(3)

              // actuator states
              << "\n\nactuator state"
              << "\nthrottle, %\t:\t" << 100*m_fdm->GetFCS()->GetThrottlePos(0)
              << "\nelevator, %\t:\t" << 100*m_fdm->GetFCS()->GetDePos(ofNorm)
              << "\naileron, %\t:\t" << 100*m_fdm->GetFCS()->GetDaLPos(ofNorm)
              << "\nrudder, %\t:\t" << 100*m_fdm->GetFCS()->GetDrPos(ofNorm)

              // nav state
              << "\n\nnav state"
              << "\naltitude, ft\t:\t" << m_fdm->GetPropagate()->GetAltitudeASL()
              << "\npsi, deg\t:\t" << m_fdm->GetPropagate()->GetEuler(3)*180/M_PI
              << "\nlat, deg\t:\t" << m_fdm->GetPropagate()->GetLatitudeDeg()
              << "\nlon, deg\t:\t" << m_fdm->GetPropagate()->GetLongitudeDeg()

              // input
              << "\n\ninput"
              << "\nthrottle cmd, %\t:\t" << 100*m_fdm->GetFCS()->GetThrottleCmd(0)
              << "\nelevator cmd, %\t:\t" << 100*m_fdm->GetFCS()->GetDeCmd()
              << "\naileron cmd, %\t:\t" << 100*m_fdm->GetFCS()->GetDaCmd()
              << "\nrudder cmd, %\t:\t" << 100*m_fdm->GetFCS()->GetDrCmd()

              << std::endl;

}

double FGTrimmer::eval(const std::vector<double> & v)
{
    double cost = 0;
    double cost0 = -1;

    double dvt=0;
    double dalpha = 0;
    double dbeta = 0;
    double dp = 0;
    double dq = 0;
    double dr = 0;

    double dvt0 = 0;
    double dalpha0 = 0;
    double dbeta0 = 0;
    double dp0 = 0;
    double dq0 = 0;
    double dr0 = 0;

    uint16_t steadyCount = 0;

    for(int i=0;;i++) {
        constrain(v);

        dvt = (m_fdm->GetPropagate()->GetUVW(1)*m_fdm->GetAccelerations()->GetUVWdot(1) +
               m_fdm->GetPropagate()->GetUVW(2)*m_fdm->GetAccelerations()->GetUVWdot(2) +
               m_fdm->GetPropagate()->GetUVW(3)*m_fdm->GetAccelerations()->GetUVWdot(3))/
              m_fdm->GetAuxiliary()->GetVt(); // from lewis, vtrue dot
        dalpha = m_fdm->GetAuxiliary()->Getadot();
        dbeta = m_fdm->GetAuxiliary()->Getbdot();
        dp = m_fdm->GetAccelerations()->GetPQRdot(1);
        dq = m_fdm->GetAccelerations()->GetPQRdot(2);
        dr = m_fdm->GetAccelerations()->GetPQRdot(3);

        if(m_fdm->GetDebugLevel() > 1) {
            std::cout
                << "dvt: " << dvt
                << "\tdalpha: " << dalpha
                << "\tdbeta: " << dbeta
                << "\tdp: " << dp
                << "\tdq: " << dq
                << "\tdr: " << dr
                << std::endl;
        }

        cost = dvt*dvt +
               100.0*(dalpha*dalpha + dbeta*dbeta) +
               10.0*(dp*dp + dq*dq + dr*dr);

        double deltaCost = std::abs(cost - cost0);
        cost0 = cost;
        dvt0 = dvt;
        dalpha0 = dalpha;
        dbeta0 = dbeta;
        dp0 = dp;
        dq0 = dq;
        dr0 = dr;

        if (deltaCost < 1e-10) {
            if (steadyCount++ > 3) break;
        } else if (i> 1000) {
            std::cout << "deltaCost: " << std::scientific << std::setw(10) 
                << deltaCost << std::endl;
            break;
            //throw std::runtime_error("cost failed to converge");
        } else {
            steadyCount=0;
        }

    }

    return cost;
}

} // JSBSim


// vim:ts=4:sw=4:expandtab
