/*
 * FGLinearization.h
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
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

#ifndef FGLinearization_H_
#define FGLinearization_H_

#include "initialization/FGTrimmer.h"
#include "math/FGStateSpace.h"
#include <iomanip>
#include <fstream>
#include "models/FGAircraft.h"
#include "models/propulsion/FGEngine.h"
#include "models/propulsion/FGTurbine.h"
#include "models/propulsion/FGTurboProp.h"
#include "math/FGNelderMead.h"
#include <stdexcept>
#include <fstream>
#include <cstdlib>

namespace JSBSim {

class FGLinearization
{
public:
    FGLinearization(FGFDMExec * fdmPtr, int mode);
};

} // JSBSim

#endif //FGLinearization_H_

// vim:ts=4:sw=4
