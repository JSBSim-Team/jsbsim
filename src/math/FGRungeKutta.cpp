/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGRungeKutta.cpp
 Author:       Thomas Kreitler
 Date started: 04/9/2010

 ------------- Copyright (C)  -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstdio>
#include <iostream>
#include <cmath>

#include "FGJSBBase.h"
#include "FGRungeKutta.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DEFINITIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

using std::cout;
using std::endl;

namespace JSBSim {

const double FGRungeKutta::RealLimit = 1e30;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRungeKutta::~FGRungeKutta() { };

int FGRungeKutta::init(double x_start, double x_end, int intervals)
{
  x0 = x_start;
  x1 = x_end;
  h  = (x_end - x_start)/intervals;
  safer_x1 = x1 - h*1e-6; // avoid 'intervals*h < x1'
  h05 = h*0.5;
  err = 0.0;
  
  if (x0>=x1) {
    status &= eFaultyInit;
  }
  return status;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/*
   Make sure that a numerical result is within +/-RealLimit.
   This is a hapless try to be portable.
   (There will be at least one architecture/compiler combination 
   where this will fail.)
*/

bool FGRungeKutta::sane_val(double x)
{
  // assuming +/- inf behave as expected and 'nan' comparisons yield to false
  if ( x < RealLimit && x > -RealLimit ) return true;
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRungeKutta::evolve(double y_0, FGRungeKuttaProblem *pf)
{
  double x = x0;
  double y = y_0;
  pfo = pf;

  iterations = 0;
  
  if (!trace_values) {
    while (x<safer_x1) {
      y  = approximate(x,y);
      if (!sane_val(y)) { status &= eMathError; }
      x += h;
      iterations++;
    }
  } else {
    while (x<safer_x1) {
      cout << x << " " << y << endl;
      y = approximate(x,y);
      if (!sane_val(y)) { status &= eMathError; }
      x += h;
      iterations++;
    }
    cout << x << " " << y << endl;
  }

  x_end = x; // twimc, store the last x used.
  return y;
}



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRK4::~FGRK4() { };

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRK4::approximate(double x, double y)
{
  double k1,k2,k3,k4;

  k1   =  pfo->pFunc(x      , y         ); 
  k2   =  pfo->pFunc(x + h05, y + h05*k1);
  k3   =  pfo->pFunc(x + h05, y + h05*k2);
  k4   =  pfo->pFunc(x + h  , y + h  *k3);

  y   +=  h/6.0 * ( k1 + 2.0*k2 + 2.0*k3 + k4 );

  return y;
}
 

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Butcher tableau
const double FGRKFehlberg::A2[] = { 0.0,     1.0/4.0 };
const double FGRKFehlberg::A3[] = { 0.0,     3.0/32.0,       9.0/32.0 };
const double FGRKFehlberg::A4[] = { 0.0,  1932.0/2197.0, -7200.0/2197.0,   7296.0/2197.0 };
const double FGRKFehlberg::A5[] = { 0.0,   439.0/216.0,     -8.0,          3680.0/513.0,    -845.0/4104.0  };
const double FGRKFehlberg::A6[] = { 0.0,    -8.0/27.0,       2.0,         -3544.0/2565.0,   1859.0/4104.0,  -11.0/40.0 };

const double FGRKFehlberg::C[]  = { 0.0, 0.0, 1.0/4.0, 3.0/8.0, 12.0/13.0, 1.0, 1.0/2.0 };

const double FGRKFehlberg::B[]  = { 0.0,     16.0/135.0,   0.0,   6656.0/12825.0,  28561.0/56430.0,   -9.0/50.0,  2.0/55.0 };
const double FGRKFehlberg::Bs[] = { 0.0,     25.0/216.0,   0.0,   1408.0/2565.0,    2197.0/4104.0,    -1.0/5.0,   0.0 };

// use this if truncation is an issue
// const double Ee[] = { 0.0, 1.0/360.0, 0.0, -128.0/4275.0, -2197.0/75240.0, 1.0/50.0, 2.0/55.0 };

FGRKFehlberg::~FGRKFehlberg() { };

double FGRKFehlberg::approximate(double x, double y)
{

  double k1,k2,k3,k4,k5,k6, as;

  double y4_val;
  double y5_val;
  double abs_err;
  double est_step;
  int done = 0;


  while (!done) {

    err  =  h*h*h*h*h; // h might change

    k1   =  pfo->pFunc(x          , y      ); 

    as   =  h*A2[1]*k1;
    k2   =  pfo->pFunc(x + C[2]*h , y + as ); 

    as   =  h*(A3[1]*k1 + A3[2]*k2);
    k3   =  pfo->pFunc(x + C[3]*h , y + as ); 

    as   =  h*(A4[1]*k1 + A4[2]*k2 + A4[3]*k3);
    k4   =  pfo->pFunc(x + C[4]*h , y + as ); 

    as   =  h*(A5[1]*k1 + A5[2]*k2 + A5[3]*k3 + A5[4]*k4);
    k5   =  pfo->pFunc(x + C[5]*h , y + as ); 

    as   =  h*(A6[1]*k1 + A6[2]*k2 + A6[3]*k3 + A6[4]*k4 + A6[5]*k5);
    k6   =  pfo->pFunc(x + C[6]*h , y + as ); 

    /* B[2]*k2 and Bs[2]*k2 are zero */
    y5_val  =  y + h * ( B[1]*k1 +  B[3]*k3 +  B[4]*k4 +  B[5]*k5 + B[6]*k6);
    y4_val  =  y + h * (Bs[1]*k1 + Bs[3]*k3 + Bs[4]*k4 + Bs[5]*k5);

    abs_err = fabs(y4_val-y5_val);
    // same in green
    // abs_err = h * (Ee[1] * k1 + Ee[3] * k3 + Ee[4] * k4 + Ee[5] * k5 + Ee[6] * k6);

    // estimate step size 
    if (abs_err > epsilon) {
      est_step = sqrt(sqrt(epsilon*h/abs_err));
    } else {
      est_step=2.0*h; // cheat
    }

    // check if a smaller step size is proposed

    if (shrink_avail>0 && est_step<h) {
        h/=2.0;
        shrink_avail--;
    } else {
      done = 1;
    }

  }

  return y4_val;
}

} // namespace JSBSim
