/*
 * FGStateSpace.cpp
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGStateSpace.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGStateSpace.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "initialization/FGInitialCondition.h"
#include "FGStateSpace.h"
#include <limits>
#include <iomanip>
#include <string>

namespace JSBSim
{

void FGStateSpace::linearize(
    std::vector<double> x0,
    std::vector<double> u0,
    std::vector<double> y0,
    std::vector< std::vector<double> > & A,
    std::vector< std::vector<double> > & B,
    std::vector< std::vector<double> > & C,
    std::vector< std::vector<double> > & D)
{
    double h = 1e-4;

    // A, d(x)/dx
    numericalJacobian(A,x,x,x0,x0,h,true);
    // B, d(x)/du
    numericalJacobian(B,x,u,x0,u0,h,true);
    // C, d(y)/dx
    numericalJacobian(C,y,x,y0,x0,h);
    // D, d(y)/du
    numericalJacobian(D,y,u,y0,u0,h);

}

void FGStateSpace::numericalJacobian(std::vector< std::vector<double> >  & J, ComponentVector & y,
                                     ComponentVector & x, const std::vector<double> & y0, const std::vector<double> & x0, double h, bool computeYDerivative)
{
    size_t nX = x.getSize();
    size_t nY = y.getSize();
    double f1 = 0, f2 = 0, fn1 = 0, fn2 = 0;
    J.resize(nY);
    for (unsigned int iY=0;iY<nY;iY++)
    {
        J[iY].resize(nX);
        for (unsigned int iX=0;iX<nX;iX++)
        {
            x.set(x0);
            x.set(iX,x.get(iX)+h);
            if (computeYDerivative) f1 = y.getDeriv(iY);
            else f1 = y.get(iY);

            x.set(x0);
            x.set(iX,x.get(iX)+2*h);
            if (computeYDerivative) f2 = y.getDeriv(iY);
            else f2 = y.get(iY);

            x.set(x0);
            x.set(iX,x.get(iX)-h);
            if (computeYDerivative) fn1 = y.getDeriv(iY);
            else fn1 = y.get(iY);

            x.set(x0);
            x.set(iX,x.get(iX)-2*h);
            if (computeYDerivative) fn2 = y.getDeriv(iY);
            else fn2 = y.get(iY);

			double diff1 = f1-fn1;
			double diff2 = f2-fn2;

			// correct for angle wrap
			if (x.getComp(iX)->getUnit().compare("rad") == 0) {
				while(diff1 > M_PI) diff1 -= 2*M_PI;
				if(diff1 < -M_PI) diff1 += 2*M_PI;
				if(diff2 > M_PI) diff2 -= 2*M_PI;
				if(diff2 < -M_PI) diff2 += 2*M_PI;
			} else if (x.getComp(iX)->getUnit().compare("deg") == 0) {
				if(diff1 > 180) diff1 -= 360;
				if(diff1 < -180) diff1 += 360;
				if(diff2 > 180) diff2 -= 360;
				if(diff2 < -180) diff2 += 360;
			}
            J[iY][iX] = (8*diff1-diff2)/(12*h); // 3rd order taylor approx from lewis, pg 203

            x.set(x0);

            if (m_fdm->GetDebugLevel() > 1)
            {
                std::cout << std::scientific << "\ty:\t" << y.getName(iY) << "\tx:\t"
                          << x.getName(iX)
                          << "\tfn2:\t" << fn2 << "\tfn1:\t" << fn1
                          << "\tf1:\t" << f1 << "\tf2:\t" << f2
                          << "\tf1-fn1:\t" << f1-fn1
                          << "\tf2-fn2:\t" << f2-fn2
                          << "\tdf/dx:\t" << J[iY][iX]
                          << std::fixed << std::endl;
            }
        }
    }
}

std::ostream &operator<<( std::ostream &out, const FGStateSpace::Component &c )
{
    out << "\t" << c.getName()
    << "\t" << c.getUnit()
    << "\t:\t" << c.get();
    return out;
}

std::ostream &operator<<( std::ostream &out, const FGStateSpace::ComponentVector &v )
{
    for (unsigned int i=0; i< v.getSize(); i++)
    {
        out << *(v.getComp(i)) << "\n";
    }
    out << "";
    return out;
}

std::ostream &operator<<( std::ostream &out, const FGStateSpace &ss )
{
    out << "\nX:\n" << ss.x
    << "\nU:\n" << ss.u
    << "\nY:\n" << ss.y;
    return out;
}

std::ostream &operator<<( std::ostream &out, const std::vector< std::vector<double> > &vec2d )
{
    std::streamsize width = out.width();
    size_t nI = vec2d.size();
    out << std::left << std::setw(1) << "[" << std::right;
    for (unsigned int i=0;i<nI;i++)
    {
		//std::cout << "i: " << i << std::endl;
        size_t nJ = vec2d[i].size();
        for (unsigned int j=0;j<nJ;j++)
        {
			//std::cout << "j: " << j << std::endl;
            if (i==0 && j==0) out << std::setw(width-1) << vec2d[i][j];
            else out << std::setw(width) << vec2d[i][j];

            if (j==nJ-1)
            {
                if ( i==nI-1 ) out << "]";
                else out <<  ";\n";
            }
            else out << ",";
        }
        out << std::flush;
    }
    return out;
}

std::ostream &operator<<( std::ostream &out, const std::vector<double> &vec )
{
    std::streamsize width = out.width();
    size_t nI = vec.size();
    out << std::left << std::setw(1) << "[" << std::right;
    for (unsigned int i=0;i<nI;i++)
    {
        if (i==0) out << std::setw(width-1) << vec[i];
        else out << std::setw(width) << vec[i];

        if ( i==nI-1 ) out << "]";
        else out <<  ";\n";
    }
    out << std::flush;
    return out;
}


} // JSBSim


// vim:ts=4:sw=4
