/*
 * FGLinearization.cpp
 * Copyright (C) James Goppert 2011 <james.goppert@gmail.com>
 *
 * FGLinearization.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGLinearization.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FGInitialCondition.h"
#include "FGLinearization.h"
#include "input_output/FGLog.h"


namespace JSBSim {

FGLinearization::FGLinearization(FGFDMExec * fdm)
    : aircraft_name(fdm->GetAircraft()->GetAircraftName())
{
    FGStateSpace ss(fdm);
    ss.x.add(new FGStateSpace::Vt);
    ss.x.add(new FGStateSpace::Alpha);
    ss.x.add(new FGStateSpace::Theta);
    ss.x.add(new FGStateSpace::Q);

    // get propulsion pointer to determine type/ etc.
    auto engine0 = fdm->GetPropulsion()->GetEngine(0);
    FGThruster * thruster0 = engine0->GetThruster();

    if (thruster0->GetType()==FGThruster::ttPropeller)
    {
        ss.x.add(new FGStateSpace::Rpm0);
        // TODO add variable prop pitch property
        // if (variablePropPitch) ss.x.add(new FGStateSpace::PropPitch);
        int numEngines = fdm->GetPropulsion()->GetNumEngines();
        if (numEngines>1) ss.x.add(new FGStateSpace::Rpm1);
        if (numEngines>2) ss.x.add(new FGStateSpace::Rpm2);
        if (numEngines>3) ss.x.add(new FGStateSpace::Rpm3);
        if (numEngines>4) {
            FGLogging log(fdm->GetLogger(), LogLevel::ERROR);
            log << "More than 4 engines not currently handled\n";
        }
    }
    ss.x.add(new FGStateSpace::Beta);
    ss.x.add(new FGStateSpace::Phi);
    ss.x.add(new FGStateSpace::P);
    ss.x.add(new FGStateSpace::Psi);
    ss.x.add(new FGStateSpace::R);
    ss.x.add(new FGStateSpace::Latitude);
    ss.x.add(new FGStateSpace::Longitude);
    ss.x.add(new FGStateSpace::Alt);

    ss.u.add(new FGStateSpace::ThrottleCmd);
    ss.u.add(new FGStateSpace::DaCmd);
    ss.u.add(new FGStateSpace::DeCmd);
    ss.u.add(new FGStateSpace::DrCmd);

    // state feedback
    ss.y = ss.x;

    x0 = ss.x.get();
    u0 = ss.u.get();
    y0 = x0; // state feedback

    fdm->SuspendIntegration();
    ss.linearize(x0, u0, y0, A, B, C, D);
    fdm->ResumeIntegration();

    x_names = ss.x.getName();
    u_names = ss.u.getName();
    y_names = ss.y.getName();
    x_units = ss.x.getUnit();
    u_units = ss.u.getUnit();
    y_units = ss.y.getUnit();
}

void FGLinearization::WriteScicoslab() const {
    auto path = std::string(aircraft_name+"_lin.sce");
    WriteScicoslab(path);
}

void FGLinearization::WriteScicoslab(std::string& path) const {
    int width=20;
    int precision=10;
    std::ofstream scicos(path.c_str());
    scicos.precision(precision);
    width=20;
    scicos  << std::scientific
            << aircraft_name << ".x0=..\n" << std::setw(width) << x0 << ";\n"
            << aircraft_name << ".u0=..\n" << std::setw(width) << u0 << ";\n"
            << aircraft_name << ".sys = syslin('c',..\n"
            << std::setw(width) << A << ",..\n"
            << std::setw(width) << B << ",..\n"
            << std::setw(width) << C << ",..\n"
            << std::setw(width) << D << ");\n"
            << aircraft_name << ".tfm = ss2tf(" << aircraft_name << ".sys);\n"
            << std::endl;
    scicos.close();

}

} // JSBSim

// vim:ts=4:sw=4
