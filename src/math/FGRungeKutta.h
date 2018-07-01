/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGRungeKutta.h
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

HISTORY
--------------------------------------------------------------------------------


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGRUNGEKUTTA_H
#define FGRUNGEKUTTA_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/**
   Minimalistic implementation of some Runge-Kutta methods. Runge-Kutta methods
   are a standard for solving ordinary differential equation (ODE) initial
   value problems. The code follows closely  the description given on
   Wikipedia, see http://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods.
   
   For more powerfull routines see GNU Scientific Library (GSL)
   or GNU Plotutils 'ode'.
*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGRungeKuttaProblem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/**
   Abstract base for the function to solve.
*/
class FGRungeKuttaProblem {
  public:
    virtual double pFunc(double x, double y) = 0;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGRungeKutta
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/**
   Abstract base.
*/

class FGRungeKutta {

  public:

    enum eStates { eNoError=0, eMathError=1, eFaultyInit=2, eEvolve=4, eUnknown=8} ;

    int init(double x_start, double x_end, int intervals = 4);

    double evolve(double y_0, FGRungeKuttaProblem *pf);

    double getXEnd()      { return x_end; }
    double getError()     { return err; }

    int  getStatus()      { return status; }
    int  getIterations()  { return iterations; }
    void clearStatus()    { status = eNoError; }
    void setTrace(bool t) { trace_values = t; }

  protected:
    // avoid accidents
    FGRungeKutta():  status(eNoError), trace_values(false), iterations(0) {};
    virtual ~FGRungeKutta();

    FGRungeKuttaProblem *pfo;

    double h;
    double h05;  // h*0.5, halfwidth
    double err;

  private:

    virtual double approximate(double x, double y) = 0;

    bool sane_val(double x);

    static const double RealLimit;

    double x0, x1;
    double safer_x1;
    double x_end;

    int status;
    bool trace_values;
    int iterations;

};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGRK4
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/**
  Classical RK4.
*/

class FGRK4 : public FGRungeKutta {
    virtual ~FGRK4();
  private:
    double approximate(double x, double y);
};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGRKFehlberg
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/**
   Runge-Kutta-Fehlberg method.
   This is a semi adaptive implementation of rkf - the interval only
   shrinks. As a result interval calculations remain trivial, but
   sometimes too many calculations are performed.
   Rationale: this code is not meant to be a universal pain-reliever
   for ode's. Rather it provides some safety if the number of
   intervals is set too low, or the problem function behaves a bit
   nasty in rare conditions.
*/


class FGRKFehlberg : public FGRungeKutta {

  public:
    FGRKFehlberg() : shrink_avail(4), epsilon(1e-12) { };
    virtual ~FGRKFehlberg();
    double getEpsilon()          { return epsilon; }
    int    getShrinkAvail()      { return shrink_avail; }
    void   setEpsilon(double e)  { epsilon = e; }
    void   setShrinkAvail(int s) { shrink_avail = s; }

  private:

    double approximate(double x, double y);

    int    shrink_avail;
    double epsilon;

    static const double A2[], A3[], A4[], A5[], A6[];
    static const double B[],  Bs[], C[];

};


} // namespace JSBSim

#endif
