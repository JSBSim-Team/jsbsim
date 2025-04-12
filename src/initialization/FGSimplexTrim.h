/*
 * FGSimplexTrim.h
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGSimplexTrim.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGSimplexTrim.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FGSimplexTrim_H_
#define FGSimplexTrim_H_

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

class FGSimplexTrim
{
public:
    FGSimplexTrim(FGFDMExec * fdmPtr, TrimMode mode);
private:
    template <class varType>
    void prompt(const std::string & str, varType & var)
    {
        std::cout << str + " [" << std::setw(10) << var << "]\t: ";
        if (std::cin.peek() != '\n')
        {
            std::cin >> var;
            std::cin.ignore(1000, '\n');
        }
        else std::cin.get();
    }

    class Callback : public JSBSim::FGNelderMead::Callback
    {
    private:
        std::ofstream _outputFile;
        JSBSim::FGTrimmer * _trimmer;
    public:
        Callback(std::string fileName, JSBSim::FGTrimmer * trimmer) :
            _outputFile((fileName + std::string("_simplexTrim.log")).c_str()),
            _trimmer(trimmer) {
        }
        virtual ~Callback() {
            _outputFile.close();
        }
        void eval(const std::vector<double> &v)
        {
            _outputFile << _trimmer->eval(v) << std::endl;
            // FGLogging log(_trimmer->getFdm()->GetLogger(), LogLevel::INFO);
            //log << "v: ";
            //for (int i=0;i<v.size();i++) log << v[i] << " ";
            //log << std::endl;
        }
    };
};

} // JSBSim

#endif //FGSimplexTrim_H_

// vim:ts=4:sw=4
