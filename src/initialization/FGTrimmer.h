/*
 * FGTrimmer.h
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGTrimmer.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGTrimmer.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JSBSim_FGTrimmer_H
#define JSBSim_FGTrimmer_H

#include "math/FGNelderMead.h"
#include "FGFDMExec.h"
#include "models/FGInertial.h"

namespace JSBSim
{

class FGTrimmer : public FGNelderMead::Function
{
public:
    struct Constraints
    {
        Constraints() :
                velocity(100), altitude(1000), gamma(0),
                rollRate(0), pitchRate(0), yawRate(0),
                coordinatedTurn(true), stabAxisRoll(true)
        {
        }
        double velocity, altitude, gamma;
        double rollRate, pitchRate, yawRate;
        bool coordinatedTurn, stabAxisRoll;
    };
    FGTrimmer(FGFDMExec * fdm, Constraints * constraints);
    ~FGTrimmer();
    std::vector<double> constrain(const std::vector<double> & v);
    void printSolution(const std::vector<double> & v);
    void printState();
    double compute_cost();
    double eval(const std::vector<double> & v);
    static void limit(double min, double max, double &val)
    {
        if (val<min) val=min;
        else if (val>max) val=max;
    }
    void setFdm(FGFDMExec * fdm) {m_fdm = fdm; }
    FGFDMExec* getFdm() { return m_fdm; }
private:
    FGFDMExec * m_fdm;
    Constraints * m_constraints;
};

} // JSBSim

#endif

// vim:ts=4:sw=4
