/*
 * FGStateSpace.h
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGStateSpace.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGStateSpace.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JSBSim_FGStateSpace_H
#define JSBSim_FGStateSpace_H

#include "FGFDMExec.h"
#include "models/FGPropulsion.h"
#include "models/FGAccelerations.h"
#include "models/propulsion/FGEngine.h"
#include "models/propulsion/FGThruster.h"
#include "models/propulsion/FGTurbine.h"
#include "models/propulsion/FGTurboProp.h"
#include "models/FGAuxiliary.h"
#include "models/FGFCS.h"
#include <fstream>
#include <iostream>
#include <limits>

namespace JSBSim
{

class FGStateSpace
{
public:

    // component class
    class Component
    {
    protected:
        FGStateSpace * m_stateSpace;
        FGFDMExec * m_fdm;
        std::string m_name, m_unit;
    public:
        Component(const std::string & name, const std::string & unit) :
                m_stateSpace(), m_fdm(), m_name(name), m_unit(unit) {};
        virtual ~Component() {};
        virtual double get() const = 0;
        virtual void set(double val) = 0;
        virtual double getDeriv() const
        {
            // by default should calculate using finite difference approx
            std::vector<double> x0 = m_stateSpace->x.get();
            double f0 = get();
            double dt0 = m_fdm->GetDeltaT();
            double time0 = m_fdm->GetSimTime();
            m_fdm->Setdt(1./120.);
            m_fdm->DisableOutput();
            m_fdm->Run();
            double f1 = get();
            m_stateSpace->x.set(x0);
            if (m_fdm->GetDebugLevel() > 1)
            {
                std::cout << std::scientific
                          << "name: " << m_name
                          << "\nf1: " << f0
                          << "\nf2: " << f1
                          << "\ndt: " << m_fdm->GetDeltaT()
                          << "\tdf/dt: " << (f1-f0)/m_fdm->GetDeltaT()
                          << std::fixed << std::endl;
            }
            double deriv = (f1-f0)/m_fdm->GetDeltaT();
            m_fdm->Setdt(dt0); // restore original value
            m_fdm->Setsim_time(time0);
            m_fdm->EnableOutput();
            return deriv;
        }
        void setStateSpace(FGStateSpace * stateSpace)
        {
            m_stateSpace = stateSpace;
        }
        void setFdm(FGFDMExec * fdm)
        {
            m_fdm = fdm;
        }
        const std::string & getName() const
        {
            return m_name;
        }
        const std::string & getUnit() const
        {
            return m_unit;
        }
    };

    // component vector class
    class ComponentVector
    {
    public:
        ComponentVector(FGFDMExec * fdm, FGStateSpace * stateSpace) :
                m_stateSpace(stateSpace), m_fdm(fdm), m_components() {}
        ComponentVector & operator=(ComponentVector & componentVector)
        {
            m_stateSpace = componentVector.m_stateSpace;
            m_fdm = componentVector.m_fdm;
            m_components = componentVector.m_components;
            return *this;
        }
        ComponentVector(const ComponentVector & componentVector) :
                m_stateSpace(componentVector.m_stateSpace),
                m_fdm(componentVector.m_fdm),
                m_components(componentVector.m_components)
        {
        }
        void add(Component * comp)
        {
            comp->setStateSpace(m_stateSpace);
            comp->setFdm(m_fdm);
            m_components.push_back(comp);
        }
        size_t getSize() const
        {
            return m_components.size();
        }
        Component * getComp(int i) const
        {
            return m_components[i];
        };
        Component * getComp(int i)
        {
            return m_components[i];
        };
        double get(int i) const
        {
            return m_components[i]->get();
        };
        void set(int i, double val)
        {
            m_components[i]->set(val);
            m_stateSpace->run();
        };
        double get(int i)
        {
            return m_components[i]->get();
        };
        std::vector<double> get() const
        {
            std::vector<double> val;
            for (unsigned int i=0;i<getSize();i++) val.push_back(m_components[i]->get());
            return val;
        }
        void get(double * array) const
        {
            for (unsigned int i=0;i<getSize();i++) array[i] = m_components[i]->get();
        }
        double getDeriv(int i)
        {
            return m_components[i]->getDeriv();
        };
        std::vector<double> getDeriv() const
        {
            std::vector<double> val;
            for (unsigned int i=0;i<getSize();i++) val.push_back(m_components[i]->getDeriv());
            return val;
        }
        void getDeriv(double * array) const
        {
            for (unsigned int i=0;i<getSize();i++) array[i] = m_components[i]->getDeriv();
        }
        void set(std::vector<double> vals)
        {
            for (unsigned int i=0;i<getSize();i++) m_components[i]->set(vals[i]);
            m_stateSpace->run();
        }
        void set(double * array)
        {
            for (unsigned int i=0;i<getSize();i++) m_components[i]->set(array[i]);
            m_stateSpace->run();
        }
        std::string getName(int i) const
        {
            return m_components[i]->getName();
        };
        std::vector<std::string> getName() const
        {
            std::vector<std::string> name;
            for (unsigned int i=0;i<getSize();i++) name.push_back(m_components[i]->getName());
            return name;
        }
        std::string getUnit(int i) const
        {
            return m_components[i]->getUnit();
        };
        std::vector<std::string> getUnit() const
        {
            std::vector<std::string> unit;
            for (unsigned int i=0;i<getSize();i++) unit.push_back(m_components[i]->getUnit());
            return unit;
        }
        void clear() {
            m_components.clear();
        }
    private:
        FGStateSpace * m_stateSpace;
        FGFDMExec * m_fdm;
        std::vector<Component *> m_components;
    };

    // component vectors
    ComponentVector x, u, y;

    // constructor
    FGStateSpace(FGFDMExec * fdm) : x(fdm,this), u(fdm,this), y(fdm,this), m_fdm(fdm) {};

    void setFdm(FGFDMExec * fdm) { m_fdm = fdm; }

    void run() {
        // initialize
        m_fdm->Initialize(m_fdm->GetIC().get());

        for (unsigned int i=0; i<m_fdm->GetPropulsion()->GetNumEngines(); i++) {
            m_fdm->GetPropulsion()->GetEngine(i)->InitRunning();
        }

        // wait for stable state
        double cost = stateSum();
        for(int i=0;i<1000;i++) {
            m_fdm->GetPropulsion()->GetSteadyState();
            m_fdm->SetTrimStatus(true);
            m_fdm->DisableOutput();
            m_fdm->SuspendIntegration();
            m_fdm->Run();
            m_fdm->SetTrimStatus(false);
            m_fdm->EnableOutput();
            m_fdm->ResumeIntegration();

            double costNew = stateSum();
            double dcost = fabs(costNew - cost);
            if (dcost < std::numeric_limits<double>::epsilon()) {
                if(m_fdm->GetDebugLevel() > 1) {
                    std::cout << "cost convergd, i: " << i << std::endl;
                }
                break;
            }
            if (i > 1000) {
                if(m_fdm->GetDebugLevel() > 1) {
                    std::cout << "cost failed to converge, dcost: "
                        << std::scientific
                        << dcost << std::endl;
                }
                break;
            }
            cost = costNew;
        }
    }

    double stateSum() {
        double sum = 0;
        for (unsigned int i=0;i<x.getSize();i++) sum += x.get(i);
        return sum;
    }

    void clear() {
        x.clear();
        u.clear();
        y.clear();
    }

    // deconstructor
    virtual ~FGStateSpace() {};

    // linearization function
    void linearize(std::vector<double> x0, std::vector<double> u0, std::vector<double> y0,
                   std::vector< std::vector<double> > & A,
                   std::vector< std::vector<double> > & B,
                   std::vector< std::vector<double> > & C,
                   std::vector< std::vector<double> > & D);


private:

    // compute numerical jacobian of a matrix
    void numericalJacobian(std::vector< std::vector<double> > & J, ComponentVector & y,
                           ComponentVector & x, const std::vector<double> & y0,
                           const std::vector<double> & x0, double h=1e-5, bool computeYDerivative = false);

    // flight dynamcis model
    FGFDMExec * m_fdm;

public:

    // components

    class Vt : public Component
    {
    public:
        Vt() : Component("Vt","ft/s") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->GetVt();
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetVtrueFpsIC(val);
        }
        double getDeriv() const
        {

            return (m_fdm->GetPropagate()->GetUVW(1)*m_fdm->GetAccelerations()->GetUVWdot(1) +
                    m_fdm->GetPropagate()->GetUVW(2)*m_fdm->GetAccelerations()->GetUVWdot(2) +
                    m_fdm->GetPropagate()->GetUVW(3)*m_fdm->GetAccelerations()->GetUVWdot(3))/
                   m_fdm->GetAuxiliary()->GetVt(); // from lewis, vtrue dot
        }

    };

    class VGround : public Component
    {
    public:
        VGround() : Component("VGround","ft/s") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->GetVground();
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetVgroundFpsIC(val);
        }
    };

    class AccelX : public Component
    {
    public:
        AccelX() : Component("AccelX","ft/s^2") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->GetPilotAccel(1);
        }
        void set(double val)
        {
            // XXX: not possible to implement currently
        }
    };

    class AccelY : public Component
    {
    public:
        AccelY() : Component("AccelY","ft/s^2") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->GetPilotAccel(2);
        }
        void set(double val)
        {
            // XXX: not possible to implement currently
        }
    };

    class AccelZ : public Component
    {
    public:
        AccelZ() : Component("AccelZ","ft/s^2") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->GetPilotAccel(3);
        }
        void set(double val)
        {
            // XXX: not possible to implement currently
        }
    };

    class Alpha : public Component
    {
    public:
        Alpha() : Component("Alpha","rad") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->Getalpha();
        }
        void set(double val)
        {
            double beta = m_fdm->GetIC()->GetBetaDegIC();
            double psi = m_fdm->GetIC()->GetPsiRadIC();
            double theta = m_fdm->GetIC()->GetThetaRadIC();
            m_fdm->GetIC()->SetAlphaRadIC(val);
            m_fdm->GetIC()->SetBetaRadIC(beta);
            m_fdm->GetIC()->SetPsiRadIC(psi);
            m_fdm->GetIC()->SetThetaRadIC(theta);
        }
        double getDeriv() const
        {
            return m_fdm->GetAuxiliary()->Getadot();
        }
    };

    class Theta : public Component
    {
    public:
        Theta() : Component("Theta","rad") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetEuler(2);
        }
        void set(double val)
        {
			m_fdm->GetIC()->SetFlightPathAngleRadIC(val-m_fdm->GetIC()->GetAlphaRadIC());
            //m_fdm->GetIC()->SetThetaRadIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetAuxiliary()->GetEulerRates(2);
        }
    };

    class Q : public Component
    {
    public:
        Q() : Component("Q","rad/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetPQR(2);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetQRadpsIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetAccelerations()->GetPQRdot(2);
        }
    };

    class Alt : public Component
    {
    public:
        Alt() : Component("Alt","ft") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetAltitudeASL();
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetAltitudeASLFtIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetPropagate()->Gethdot();
        }
    };

    class Beta : public Component
    {
    public:
        Beta() : Component("Beta","rad") {};
        double get() const
        {
            return m_fdm->GetAuxiliary()->Getbeta();
        }
        void set(double val)
        {
            double psi = m_fdm->GetIC()->GetPsiRadIC();
            m_fdm->GetIC()->SetBetaRadIC(val);
            m_fdm->GetIC()->SetPsiRadIC(psi);
        }
        double getDeriv() const
        {
            return m_fdm->GetAuxiliary()->Getbdot();
        }
    };

    class Phi : public Component
    {
    public:
        Phi() : Component("Phi","rad") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetEuler(1);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetPhiRadIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetAuxiliary()->GetEulerRates(1);
        }
    };

    class P : public Component
    {
    public:
        P() : Component("P","rad/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetPQR(1);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetPRadpsIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetAccelerations()->GetPQRdot(1);
        }
    };

    class R : public Component
    {
    public:
        R() : Component("R","rad/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetPQR(3);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetRRadpsIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetAccelerations()->GetPQRdot(3);
        }
    };

    class Psi : public Component
    {
    public:
        Psi() : Component("Psi","rad") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetEuler(3);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetPsiRadIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetAuxiliary()->GetEulerRates(3);
        }
    };

    class ThrottleCmd : public Component
    {
    public:
        ThrottleCmd() : Component("ThtlCmd","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetThrottleCmd(0);
        }
        void set(double val)
        {
            for (unsigned int i=0;i<m_fdm->GetPropulsion()->GetNumEngines();i++)
                m_fdm->GetFCS()->SetThrottleCmd(i,val);
            m_fdm->GetFCS()->Run(true);
        }
    };

    class ThrottlePos : public Component
    {
    public:
        ThrottlePos() : Component("ThtlPos","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetThrottlePos(0);
        }
        void set(double val)
        {
            for (unsigned int i=0;i<m_fdm->GetPropulsion()->GetNumEngines();i++)
                m_fdm->GetFCS()->SetThrottlePos(i,val);
        }
    };

    class DaCmd : public Component
    {
    public:
        DaCmd() : Component("DaCmd","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetDaCmd();
        }
        void set(double val)
        {
            m_fdm->GetFCS()->SetDaCmd(val);
            m_fdm->GetFCS()->Run(true);
        }
    };

    class DaPos : public Component
    {
    public:
        DaPos() : Component("DaPos","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetDaLPos();
        }
        void set(double val)
        {
            m_fdm->GetFCS()->SetDaLPos(ofRad,val);
            m_fdm->GetFCS()->SetDaRPos(ofRad,val); // TODO: check if this is neg.
        }
    };

    class DeCmd : public Component
    {
    public:
        DeCmd() : Component("DeCmd","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetDeCmd();
        }
        void set(double val)
        {
            m_fdm->GetFCS()->SetDeCmd(val);
            m_fdm->GetFCS()->Run(true);
        }
    };

    class DePos : public Component
    {
    public:
        DePos() : Component("DePos","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetDePos();
        }
        void set(double val)
        {
            m_fdm->GetFCS()->SetDePos(ofRad,val);
        }
    };

    class DrCmd : public Component
    {
    public:
        DrCmd() : Component("DrCmd","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetDrCmd();
        }
        void set(double val)
        {
            m_fdm->GetFCS()->SetDrCmd(val);
            m_fdm->GetFCS()->Run(true);
        }
    };

    class DrPos : public Component
    {
    public:
        DrPos() : Component("DrPos","norm") {};
        double get() const
        {
            return m_fdm->GetFCS()->GetDrPos();
        }
        void set(double val)
        {
            m_fdm->GetFCS()->SetDrPos(ofRad,val);
        }
    };

    class Rpm0 : public Component
    {
    public:
        Rpm0() : Component("Rpm0","rev/min") {};
        double get() const
        {
            return m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetRPM();
        }
        void set(double val)
        {
            m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->SetRPM(val);
        }
    };

    class Rpm1 : public Component
    {
    public:
        Rpm1() : Component("Rpm1","rev/min") {};
        double get() const
        {
            return m_fdm->GetPropulsion()->GetEngine(1)->GetThruster()->GetRPM();
        }
        void set(double val)
        {
            m_fdm->GetPropulsion()->GetEngine(1)->GetThruster()->SetRPM(val);
        }
    };

    class Rpm2 : public Component
    {
    public:
        Rpm2() : Component("Rpm2","rev/min") {};
        double get() const
        {
            return m_fdm->GetPropulsion()->GetEngine(2)->GetThruster()->GetRPM();
        }
        void set(double val)
        {
            m_fdm->GetPropulsion()->GetEngine(2)->GetThruster()->SetRPM(val);
        }
    };

    class Rpm3 : public Component
    {
    public:
        Rpm3() : Component("Rpm3","rev/min") {};
        double get() const
        {
            return m_fdm->GetPropulsion()->GetEngine(3)->GetThruster()->GetRPM();
        }
        void set(double val)
        {
            m_fdm->GetPropulsion()->GetEngine(3)->GetThruster()->SetRPM(val);
        }
    };

    class PropPitch : public Component
    {
    public:
        PropPitch() : Component("Prop Pitch","deg") {};
        double get() const
        {
            return m_fdm->GetPropulsion()->GetEngine(0)->GetThruster()->GetPitch();
        }
        void set(double val)
        {
            for (unsigned int i=0;i<m_fdm->GetPropulsion()->GetNumEngines();i++)
                m_fdm->GetPropulsion()->GetEngine(i)->GetThruster()->SetPitch(val);
        }
    };

    class Longitude : public Component
    {
    public:
        Longitude() : Component("Longitude","rad") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetLongitude();
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetLongitudeRadIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetPropagate()->GetVel(2)/(cos(m_fdm->GetPropagate()->GetLatitude())*m_fdm->GetPropagate()->GetRadius());
        }
    };

    class Latitude : public Component
    {
    public:
        Latitude() : Component("Latitude","rad") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetLatitude();
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetLatitudeRadIC(val);
        }
        double getDeriv() const
        {
            return m_fdm->GetPropagate()->GetVel(1)/(m_fdm->GetPropagate()->GetRadius());
        }
    };

    class Pi : public Component
    {
    public:
        Pi() : Component("P inertial","rad/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetPQRi(1);
        }
        void set(double val)
        {
            //Set PQR from PQRi
            //VState.vPQR = VState.vPQRi - Ti2b * vOmegaEarth;
            m_fdm->GetIC()->SetQRadpsIC(val + \
                    (m_fdm->GetPropagate()->GetPQR(1) - m_fdm->GetPropagate()->GetPQRi(1)));
        }
        double getDeriv() const
        {
            return m_fdm->GetAccelerations()->GetPQRdot(1);
        }
    };

    class Qi : public Component
    {
    public:
        Qi() : Component("Q inertial","rad/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetPQRi(2);
        }
        void set(double val)
        {
            //Set PQR from PQRi
            //VState.vPQR = VState.vPQRi - Ti2b * vOmegaEarth;
            m_fdm->GetIC()->SetQRadpsIC(val + \
                    (m_fdm->GetPropagate()->GetPQR(2) - m_fdm->GetPropagate()->GetPQRi(2)));
        }
        double getDeriv() const
        {
            return m_fdm->GetAccelerations()->GetPQRdot(2);
        }
    };

    class Ri : public Component
    {
    public:
        Ri() : Component("R inertial","rad/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetPQRi(3);
        }
        void set(double val)
        {
            //Set PQR from PQRi
            //VState.vPQR = VState.vPQRi - Ti2b * vOmegaEarth;
            m_fdm->GetIC()->SetQRadpsIC(val + \
                    (m_fdm->GetPropagate()->GetPQR(3) - m_fdm->GetPropagate()->GetPQRi(3)));
        }
        double getDeriv() const
        {
            return m_fdm->GetAccelerations()->GetPQRdot(3);
        }
    };

    class Vn : public Component
    {
    public:
        Vn() : Component("Vel north","feet/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetVel(1);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetVNorthFpsIC(val);
        }
        double getDeriv() const
        {
            //get NED accel from body accel
            return (m_fdm->GetPropagate()->GetTb2l()*m_fdm->GetAccelerations()->GetUVWdot())(1);
        }
    };

    class Ve : public Component
    {
    public:
        Ve() : Component("Vel east","feet/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetVel(2);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetVEastFpsIC(val);
        }
        double getDeriv() const
        {
            //get NED accel from body accel
            return (m_fdm->GetPropagate()->GetTb2l()*m_fdm->GetAccelerations()->GetUVWdot())(2);
        }
    };

    class Vd : public Component
    {
    public:
        Vd() : Component("Vel down","feet/s") {};
        double get() const
        {
            return m_fdm->GetPropagate()->GetVel(3);
        }
        void set(double val)
        {
            m_fdm->GetIC()->SetVDownFpsIC(val);
        }
        double getDeriv() const
        {
            //get NED accel from body accel
            return (m_fdm->GetPropagate()->GetTb2l()*m_fdm->GetAccelerations()->GetUVWdot())(3);
        }
    };

    class COG : public Component
    {
    public:
        COG() : Component("Course Over Ground","rad") {};
        double get() const
        {
            //cog = atan2(Ve,Vn)
            return atan2(m_fdm->GetPropagate()->GetVel(2),m_fdm->GetPropagate()->GetVel(1));
        }
        void set(double val)
        {
            //set Vn and Ve according to vGround and COG
            m_fdm->GetIC()->SetVNorthFpsIC(m_fdm->GetAuxiliary()->GetVground()*cos(val));
            m_fdm->GetIC()->SetVEastFpsIC(m_fdm->GetAuxiliary()->GetVground()*sin(val));
        }
        double getDeriv() const
        {
            double Vn = m_fdm->GetPropagate()->GetVel(1);
            double Vndot = (m_fdm->GetPropagate()->GetTb2l()*m_fdm->GetAccelerations()->GetUVWdot())(1);
            double Ve = m_fdm->GetPropagate()->GetVel(2);
            double Vedot = (m_fdm->GetPropagate()->GetTb2l()*m_fdm->GetAccelerations()->GetUVWdot())(2); 

            //dCOG/dt = dCOG/dVe*dVe/dt + dCOG/dVn*dVn/dt
            return Vn/(Vn*Vn+Ve*Ve)*Vedot - Ve/(Vn*Vn+Ve*Ve)*Vndot;
        }
    };

};

// stream output
std::ostream &operator<<(std::ostream &out, const FGStateSpace::Component &c );
std::ostream &operator<<(std::ostream &out, const FGStateSpace::ComponentVector &v );
std::ostream &operator<<(std::ostream &out, const FGStateSpace &ss );
std::ostream &operator<<( std::ostream &out, const std::vector< std::vector<double> > &vec2d );
std::ostream &operator<<( std::ostream &out, const std::vector<double> &vec );

} // JSBSim

#endif

// vim:ts=4:sw=4
