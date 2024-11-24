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

#include "math/FGStateSpace.h"
#include <iomanip>
#include <fstream>
#include "models/FGAircraft.h"
#include "models/propulsion/FGEngine.h"
#include "models/propulsion/FGTurbine.h"
#include "models/propulsion/FGTurboProp.h"
#include <fstream>
#include <cstdlib>

namespace JSBSim {

template <class T>
using Vector2D = std::vector<std::vector<T>>;

/** \brief Class used to create linear models from FGFDMExec instances.
 */
class JSBSIM_API FGLinearization
{
    Vector2D<double> A,B,C,D;
    std::vector<double> x0, u0, y0;
    std::vector<std::string> x_names, u_names, y_names, x_units, u_units, y_units;
    std::string aircraft_name;
public:
    /**
     * @param fdmPtr Already configured FGFDMExec instance used to create the new linear model.
     */
    FGLinearization(FGFDMExec * fdmPtr);

    /**
     * Write Scicoslab source file with the state space model to a
     * file in the current working directory.
     */
    void WriteScicoslab() const;

    /**
     * Write Scicoslab source file with the state space model to the given path.
     *
     * @param path
     */
    void WriteScicoslab(std::string& path) const;

    /**
     * Get the state space model matrices.
     *
     * @param A_ System matrix
     * @param B_ Input matrix
     * @param C_ Output matrix
     * @param D_ Feedforward matrix
     */
    const Vector2D<double>& GetSystemMatrix() const { return A; };
    const Vector2D<double>& GetInputMatrix() const { return B; };
    const Vector2D<double>& GetOutputMatrix() const { return C; };
    const Vector2D<double>& GetFeedforwardMatrix() const { return D; };

    const std::vector<double>& GetInitialState() const { return x0; };
    const std::vector<double>& GetInitialInput() const { return u0; };
    const std::vector<double>& GetInitialOutput() const { return y0; };

    const std::vector<std::string>& GetStateNames() const { return x_names; };
    const std::vector<std::string>& GetInputNames() const { return u_names; };
    const std::vector<std::string>& GetOutputNames() const { return y_names; };

    const std::vector<std::string>& GetStateUnits() const { return x_units; };
    const std::vector<std::string>& GetInputUnits() const { return u_units; };
    const std::vector<std::string>& GetOutputUnits() const { return y_units; };

};

} // JSBSim

#endif //FGLinearization_H_

// vim:ts=4:sw=4
