// Controls.cpp -- Implements the Aircraft Control types.
//
// Based on Aeromatic2 PHP code by David P. Culp
// Started June 2003
//
// C++-ified and modulized by Erik Hofman, started October 2015.
//
// Copyright (C) 2003, David P. Culp <davidculp2@comcast.net>
// Copyright (C) 2015 Erik Hofman <erik@ehofman.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

/**
 * References:
 *
 * http://www.princeton.edu/~stengel/MAE331Lectures.html
 * http://www.dept.aoe.vt.edu/~mason/Mason_f/ConfigAeroTransonics.pdf
 * http://aerostudents.com/files/flightDynamics/lateralStabilityDerivatives.pdf
 * http://aerostudents.com/files/flightDynamics/longitudinalStabilityDerivatives.pdf
 * http://aviation.stackexchange.com/questions/14508/calculating-a-finite-wings-lift-from-its-sectional-airfoil-shape
 *
 * See also:
 * http://www.flightlevelengineering.com/downloads/stab.pdf
 */

#include <math.h>

#include <sstream>
#include <iomanip>

#include <types.h>
#include <Aircraft.h>
#include "Controls.h"

#define MIN_ALPHA	(-2*DEG_TO_RAD)
#define MAX_ALPHA	( 20*DEG_TO_RAD)

namespace Aeromatic
{

Controls::Controls(Aeromatic *p) :
    System(p, true),
    _ctype(0)
{
    _description.push_back("Aircraft control");

    Param *controls = new Param("Control system", 0, _ctype);
    _inputs.push_back(controls);

    _control[0] = new CableControls(p);
    controls->add_option(_control[0]->get_description());

    _control[1] = new YawDamper(p);
    controls->add_option(_control[1]->get_description());

    _control[2] = new FlyByWire(p);
//  controls->add_option(_control[2]->get_description());
}

Controls::~Controls()
{
    for (unsigned i=0; i<3; ++i) {
        delete _control[i];
    }
}

std::string Controls::comment()
{
    std::stringstream file;

    file << "    control type:   " << _control[_ctype]->get_description() << std::endl;

    return file.str();
}

void CableControls::set(const float* cg_loc)
{
    // *** CLalpha_wing based on wing geometry ***
    std::vector<float>& CLaw = _aircraft->_CLaw;
    std::vector<float>& CLah = _aircraft->_CLah;
    std::vector<float>& CLav = _aircraft->_CLav;
    _get_CLaw(CLaw, _aircraft->_wing);
    _get_CLaw(CLah, _aircraft->_htail);
    _get_CLaw(CLav, _aircraft->_vtail);

    float Sw = _aircraft->_wing.area;
    float W = _aircraft->_empty_weight + _aircraft->_payload;
    float Ws = _aircraft->_stall_weight;
    float Vs = _aircraft->_stall_speed * KNOTS_TO_FPS;
    float dihedral = _aircraft->_wing.dihedral * DEG_TO_RAD;
    float sweep_le = _aircraft->_wing.sweep_le * DEG_TO_RAD;
    float sweep = _aircraft->_wing.sweep * DEG_TO_RAD;
    float cbarw = _aircraft->_wing.chord_mean;
    float AR = _aircraft->_wing.aspect;
    float TR = _aircraft->_wing.taper;

    float Vt = (Vs > 1.0f) ? (2.8f*Vs) : 202.0f;	// approx. 120kts.
    float rho = 0.0023769f;
    float Q = 0.5f*rho*Vt*Vt;
    float CL = W/Q/Sw;

    if (Vs)
    {
        // *** CLmax based on wing geometry and stall speed ***
        _aircraft->_CLmax[0] = 2*Ws/(rho*Sw*Vs*Vs);

        if (_aircraft->_Mcrit == 0)
        {
            // *** Critical Mach based on wing geometry and stall speed ***
            float TC = _aircraft->_wing.thickness/cbarw;

            // Korn  equation
            float CS = cosf(sweep_le);
            float CS2 = CS*CS, CS3 = CS2*CS;
            float Ka = _aircraft->_wing.Ktf;
            float Mdd = Ka/CS - TC/CS2 - CL/(10.0f*CS3);
            _aircraft->_Mcrit = Mdd - 0.1077217345f;
        }
    }

    // *** Pitch, Roll and Yaw moments ***
    // Approximations based on code by Mark Peters for MPX-5.
    // https://engineering.purdue.edu/~andrisan/Courses/AAE451%20Fall2000/mpx5

    float bw = _aircraft->_wing.span;
    float deda = _aircraft->_wing.de_da;
    float Sh = _aircraft->_htail.area;

    _aircraft->_CLalpha.at(0) = CLaw[0]+CLah[0]*Sh/Sw*(1.0f-deda);
    _aircraft->_CLalpha.at(1) = CLaw[1];
    _aircraft->_CLalpha.at(2) = CLaw[2];

    // *** Pitch moment ***
    float lh = _aircraft->_htail.arm;
    float Vh = lh*Sh/cbarw/Sw;
  
    float nh = _aircraft->_htail.efficiency;
    float Ee = _aircraft->_htail.flap_ratio;	// elevator
    float dcgx = -(cg_loc[X] - _aircraft->_aero_rp[X])*INCH_TO_FEET;
    float ch = cbarw*sqrtf(Sh/Sw);

    // drag
    float Sv = _aircraft->_vtail.area;

    // Sfus: Fuselage wetted area
    float TC = _aircraft->_wing.thickness/cbarw;
    float k0 = 0.075f;
    float k1 = 0.2f*TC; // 1.256f; // correction factor for wing thickness
//  float k2 = 1.12f;           // fuselage fineness ratio correction factor
    float Cf = 0.006f;          // skin Friction Coefficient
    float fwings = Cf*(Sw+Sh+Sv)*2.04f*k1;
    float CD0w = fwings/Sw;
//  float ffus   = Cf*Sfus*k2;
//  float CD0f = ffus/Sw;
//  _aircraft->_CD0 = CD0f + CD0w;
    _aircraft->_CD0 *= (1.0f - sinf(sweep));

    // lift
    float alpha = 0;
    float Ew = _aircraft->_wing.efficiency;
    float E0 = Ew*(CL/AR);
    float de = 0;
    float CLh = W/Q/Sh;
    float a0w = _aircraft->_wing.incidence*DEG_TO_RAD;
    float iw = ((CL/CLaw[0]) + a0w - alpha);
    float ih = ((CLh/CLah[0]) - (alpha*(1.0f-deda) - E0 + Ee*de));

    float lv = _aircraft->_vtail.arm;
    float Vv = Sv*lv/bw/Sw;

    float nv = _aircraft->_vtail.efficiency;
    float Er = _aircraft->_vtail.flap_ratio;    // rudder

    float M = 0.0f;
    float M2 = M*M;

    float dsdB = 0.0f;          // ds/dB
    float CYbeta = -nv*(Sv/Sw)*CLav[0]*(1.0f+dsdB);
    float CYp_const = -(AR+cosf(sweep))/(AR+4*cosf(sweep))*tanf(sweep);

    float CL0 = _aircraft->_CL0;
    float CLalpha = _aircraft->_CLalpha[0];

    float zw = -0.0f; // z-pos. wing: positive down
    float zv = -1.0f; // z-pos. vertical tail:  positive down
    float fus_diameter = AR/bw; // 0.1f*bw;
    float Clbwf = 1.2f*sqrt(AR)*((zw+2.0f*fus_diameter)/(bw*bw));
    float Clbvt = -(zv/bw)*CLah[0];

    CL0 = CLaw[0]*(iw - a0w)+(Sh/Sw)*nh*CLah[0]*(ih - E0);
    _aircraft->_CDalpha.at(0) = CLalpha*(2.0f*CL0)/(PI*AR*Ew);

    _aircraft->_CL0 = CL0;
    for (int i=0; i<4; ++i)
    {
        switch (i)
        {
        case 0:
            CL = 10.0f * _aircraft->_CLmax[0];
            Vt = Vs;
            break;
        case 1:
            Vt = 1.1*Vs;
            Q = 0.5f*rho*Vs*Vs;
            CL = W/Q/Sw;
            break;
        case 2:
            Vt = 1.5f*Vs;
            Q = 0.5f*rho*Vt*Vt;
            CL = W/Q/Sw;
            break;
        case 3:
            CL = CL0;
            Vt = sqrtf(W/(0.5f*rho*CL*Sw));
            break;
        }
        _aircraft->_Re.at(i) = (Vt * cbarw)*6372.38970987f; // 1/0.000156927

        alpha = (CL-CL0)/CLalpha;
        _aircraft->_alpha.at(i) = alpha;

        float CLmin = CL0 + MIN_ALPHA*CLalpha;
        float CLmax = CL0 + std::max<float>(alpha,MAX_ALPHA)*CLalpha;

        _aircraft->_CYp.at(i) = -CL*CYp_const;
        _aircraft->_Cnp.at(i) = -CL/8.0f;

        float Cmin, Cmax;

        // From Flight Dynamics by Robert F. Stengel page 99
        M = Vt / 661.5f; M2 = M*M;
        Cmin = (-((1.0f+2.0f*TR)/(6.0f+6.0f*TR))*(dihedral*CLaw[0] + (CLmin*tanf(sweep)/(1.0f-M2*powf(cosf(sweep), 2.0f)))));
        Cmax = (-((1.0f+2.0f*TR)/(6.0f+6.0f*TR))*(dihedral*CLaw[0] + (CLmax*tanf(sweep)/(1.0f-M2*powf(cosf(sweep), 2.0f)))));

      _aircraft->_Clbeta.at(i*2) = Cmin - Clbwf - alpha*Clbvt;
      _aircraft->_Clbeta.at(i*2+1) = Cmax - Clbwf - alpha*Clbvt;

        float Clr_const = 2.0f*lv*lv/bw/bw*(CYbeta*alpha);
        Cmin = (CLmin/4.0f)-Clr_const;
        Cmax = (CLmax/4.0f)-Clr_const;
        _aircraft->_Clr.at(i*2) = Cmin;
        _aircraft->_Clr.at(i*2+1) = Cmax;
    }

    _aircraft->_CLq = 2.0f*nh*Vh*CLah[0];
    _aircraft->_CLadot = _aircraft->_CLq*deda;

    float CLhde = ((CLah[0]/PI)*(acosf(1.0f-2.0f*Ee)+2.0f*sqrtf(Ee-Ee*Ee)));
    _aircraft->_CLde = (CLhde*Sh/Sw)*2.0f/PI;

    // pitch
    if (_aircraft->_user_wing_data > 0)
    {
        _aircraft->_Cmalpha =  CLaw[0]*(dcgx/cbarw) - Vh*CLah[0]*(1.0f-deda);
        _aircraft->_Cmq = -_aircraft->_CLq*(lh/cbarw);
        _aircraft->_Cmadot = -_aircraft->_CLadot*(lh/cbarw);

        float Cmtde = CLah[0]/PI*(1.0f-Ee)*sqrtf(Ee-Ee*Ee);
        _aircraft->_Cmde = (Sh*ch/Sw/cbarw*Cmtde - lh*Sh*CLhde/cbarw/Sw);
    }

    // side
    float Cltdr = (CLav[0]/PI)*(acosf(1.0f-2.0f*Er)+2.0f*sqrtf(Er-Er*Er));
    _aircraft->_CYbeta = CYbeta;
    _aircraft->_CYr = -2.0f*(lv/bw)*(CYbeta);
    _aircraft->_CYdr = (Sv/Sw)*Cltdr;

    // roll
    float TRh = _aircraft->_htail.taper;
//  float TRv = _aircraft->_vtail.taper;
    _aircraft->_Clp = -(CLaw[0]/12.0f)*(1.0f+3.0f*TR)/(1.0f+TR)
                      + (CLah[0]/12.0f)*(Sh/Sw)*(1.0f+3.0f*TRh)/(1.0f+TRh)
                      + (CLav[0]/12.0f)*(Sv/Sw)*(1.0f+3.0f*TRh)/(1.0f+TRh);

    // yaw
    _aircraft->_Cnbeta = nv*Vv*CLav[0]*(1.0-dsdB);
    _aircraft->_Cnr = -(k0*CL*CL + k1*CD0w) - 2.0f*nv*Vv*CLav[0]*(lv/bw);
    _aircraft->_Cndr = -Vv*Cltdr;

#if 0
printf("Cma: %f, Cmadot: %f, Cmq: %f, Cmde: %f\n",  _aircraft->_Cmalpha, _aircraft->_Cmadot, _aircraft->_Cmq, _aircraft->_Cmde);
printf("CYbeta: %f, CYr: %f, CYp: %f, CYdr: %f\n", _aircraft->_CYbeta, _aircraft->_CYr, _aircraft->_CYp, _aircraft->_CYdr);
printf("Cnbeta: %f, Cnr: %f, Cnp: %f, Cndr: %f\n", _aircraft->_Cnbeta, _aircraft->_Cnr, _aircraft->_Cnp, _aircraft->_Cndr);
printf("Clbeta: %f, Clr: %f, Clp: %f\n", _aircraft->_Clbeta, _aircraft->_Clr, _aircraft->_Clp);
#endif
}


std::string CableControls::lift()
{
    float CLalpha, CLmax, CL0, CLde, CLq, CLadot, alpha;
    std::stringstream file;

    CLalpha = _aircraft->_CLalpha[0];
    CLmax = _aircraft->_CLmax[0];
    CL0 = _aircraft->_CL0;
    CLq = _aircraft->_CLq;
    CLadot = _aircraft->_CLadot;
    CLde = _aircraft->_CLde;

    alpha = (CLmax-CL0)/CLalpha;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Lift_alpha\">" << std::endl;
    file << "      <description>Lift due to alpha</description>" << std::endl;
    file << "      <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "              -0.20 " << std::setw(5) << (-0.2*CLalpha + CL0) << std::endl;
    file << "               0.00  " << std::setw(6) << CL0 << std::endl;
    file << "               " << std::setprecision(2) << (alpha) << std::setprecision(4) << "  " << (CLmax) << std::endl;
    file << "               0.60  " << std::setw(6) << (CLmax-(0.6*alpha*CLalpha)) << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "      </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Lift_pitch_rate\">" << std::endl;
    file << "        <description>Lift due to pitch rate</description>" << std::endl;
    file << "        <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <property>velocities/q-aero-rad_sec</property>" << std::endl;
    file << "          <property>aero/ci2vel</property>" << std::endl;
    file << "          <value> " << (CLq) << " </value>" << std::endl;
    file << "        </product>" << std::endl;
    file << "      </function>" << std::endl;
    file << std::endl;
    file << "      <function name=\"aero/force/Lift_alpha_rate\">" << std::endl;
    file << "        <description>Lift due to alpha rate</description>" << std::endl;
    file << "        <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>aero/alphadot-rad_sec</property>" << std::endl;
    file << "           <property>aero/ci2vel</property>" << std::endl;
    file << "           <value> " << (CLadot) << " </value>" << std::endl;
    file << "        </product>" << std::endl;
    file << "      </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Lift_elevator\">" << std::endl;
    file << "       <description>Lift due to Elevator Deflection</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/elevator-pos-rad</property>" << std::endl;
    file << "           <value> " << (CLde) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string CableControls::drag()
{
    float CD0, CDi, CDmax, CDalpha, Mcrit, CDbeta, CDde;
    std::stringstream file;

    CD0 = _aircraft->_CD0;
    CDi = _aircraft->_Kdi;
    Mcrit = _aircraft->_Mcrit;
    CDalpha = _aircraft->_CDalpha[0];
    CDbeta = _aircraft->_CDbeta;
    CDde = _aircraft->_CDde;

    float AR = _aircraft->_wing.aspect;
//  float sweep = _aircraft->_wing.sweep * DEG_TO_RAD;
    float Ew = _aircraft->_wing.efficiency;

    float CL0 = _aircraft->_CL0;
    float CLmax = _aircraft->_CLmax[0];
    float CLalpha = _aircraft->_CLalpha[0];
    float alpha = (CLmax-CL0)/CLalpha;

    CDi = 1.0f/(PI * fabs(Ew) * AR);

    float Sw = _aircraft->_wing.area;
    float Sh = _aircraft->_htail.area;
    CDmax = 1.28f * 1.1f*(Sw+(Sh/Sw))/Sw;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Drag_basic\">" << std::endl;
    file << "       <description>Drag at zero lift</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "             -1.57    " << (CDmax) << std::endl;
    file << "             " << std::setprecision(2) << (-alpha) << "    " << std::setprecision(4) << (CD0 + alpha * CDalpha) << std::endl;
    file << "              0.00    " << (CD0) << std::endl;
    file << "              " << std::setprecision(2) << (alpha) << "    " << std::setprecision(4) << (CD0 + alpha * CDalpha) << std::endl;
    file << "              1.57    " << (CDmax) << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Drag_induced\">" << std::endl;
    file << "       <description>Induced drag</description>" << std::endl;
    file << "         <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>aero/cl-squared</property>" << std::endl;
    file << "           <value> " << (CDi) << " </value>" << std::endl;
    file << "         </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Drag_mach\">" << std::endl;
    file << "       <description>Drag due to mach</description>" << std::endl;
    file << "        <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "                0.00    0.0000" << std::endl;
    file << "                " << std::setprecision(2) << (Mcrit) << std::setprecision(4) << "    0.0000" << std::endl;
    file << "                1.10    0.0230" << std::endl;
    file << "                1.80    0.0150" << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "        </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Drag_beta\">" << std::endl;
    file << "       <description>Drag due to sideslip</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/beta-rad</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "              -1.57    1.2300" << std::endl;
    file << "              -0.26    " << (0.25*CDbeta) << std::endl;
    file << "               0.00    0.0000" << std::endl;
    file << "               0.26    " << (0.25*CDbeta) << std::endl;
    file << "               1.57    1.2300" << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Drag_elevator\">" << std::endl;
    file << "       <description>Drag due to Elevator Deflection</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <abs><property>fcs/elevator-pos-rad</property></abs>" << std::endl;
    file << "           <value> " << (CDde) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string CableControls::side()
{
    std::stringstream file;
    float CYbeta, CYr, CYdr;

    CYbeta = _aircraft->_CYbeta;
//  CYp = _aircraft->_CYp[0];
    CYr = _aircraft->_CYr;
    CYdr = _aircraft->_CYdr;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Side_beta\">" << std::endl;
    file << "       <description>Side force due to beta</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>aero/beta-rad</property>" << std::endl;
    file << "           <value> " << (CYbeta) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << "    <function name=\"aero/force/Side_roll_rate\">" << std::endl;
    file << "       <description>Side force due to roll rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/p-aero-rad_sec</property>" << std::endl;

    file << _print_vector(_aircraft->_CYp);

    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Side_yaw_rate\">" << std::endl;
    file << "       <description>Side force due to yaw rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/r-aero-rad_sec</property>" << std::endl;
    file << "           <value> " << (CYr) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Side_rudder\">" << std::endl;
    file << "       <description>Side force due to rudder</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/rudder-pos-rad</property>" << std::endl;
    file << "           <value> " << (CYdr) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string CableControls::roll()
{
    float Clp, Clda, Cldr;
    std::stringstream file;

//  Clbeta = _aircraft->_Clbeta[0];
    Clp = _aircraft->_Clp;
//  Clr = _aircraft->_Clr[0];
    Clda = _aircraft->_Clda;
    Cldr = _aircraft->_Cldr;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/moment/Roll_beta\">" << std::endl;
    file << "       <description>Roll moment due to beta</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/beta-rad</property>" << std::endl;

    file << _print_vector(_aircraft->_Clbeta);

    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Roll_damp\">" << std::endl;
    file << "       <description>Roll moment due to roll rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/p-aero-rad_sec</property>" << std::endl;
    file << "           <value> " << (Clp) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Roll_yaw\">" << std::endl;
    file << "       <description>Roll moment due to yaw rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/r-aero-rad_sec</property>" << std::endl;

    file << _print_vector(_aircraft->_Clr);

    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Roll_aileron\">" << std::endl;
    file << "       <description>Roll moment due to aileron</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <property>metrics/bw-ft</property>" << std::endl;
    file << "          <property>fcs/left-aileron-pos-rad</property>" << std::endl;
    file << "           <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "              0.0    " << (Clda) << std::endl;
    file << "              2.0    " << (0.25*Clda) << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Roll_rudder\">" << std::endl;
    file << "       <description>Roll moment due to rudder</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>fcs/rudder-pos-rad</property>" << std::endl;
    file << "           <value> " << (Cldr) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string CableControls::pitch()
{
    float Cmalpha, Cmq, Cmadot, Cmde;
    std::stringstream file;

    Cmalpha = _aircraft->_Cmalpha;
    Cmq = _aircraft->_Cmq;
    Cmadot = _aircraft->_Cmadot;
    Cmde = _aircraft->_Cmde;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/moment/Pitch_alpha\">" << std::endl;
    file << "       <description>Pitch moment due to alpha</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/cbarw-ft</property>" << std::endl;
    file << "           <property>aero/alpha-rad</property>" << std::endl;
    file << "           <value> " << (Cmalpha) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Pitch_elevator\">" << std::endl;
    file << "       <description>Pitch moment due to elevator</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <property>metrics/cbarw-ft</property>" << std::endl;
    file << "          <property>fcs/elevator-pos-rad</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "              0.0    " << (Cmde) << std::endl;
    file << "              2.0    " << (0.25*Cmde) << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl; 
    file << std::endl;
    file << "    <function name=\"aero/moment/Pitch_damp\">" << std::endl;
    file << "       <description>Pitch moment due to pitch rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/cbarw-ft</property>" << std::endl;
    file << "           <property>aero/ci2vel</property>" << std::endl;
    file << "           <property>velocities/q-aero-rad_sec</property>" << std::endl;
    file << "           <value> " << (Cmq) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Pitch_alphadot\">" << std::endl;
    file << "       <description>Pitch moment due to alpha rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/cbarw-ft</property>" << std::endl;
    file << "           <property>aero/ci2vel</property>" << std::endl;
    file << "           <property>aero/alphadot-rad_sec</property>" << std::endl;
    file << "           <value> " << (Cmadot) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string CableControls::yaw()
{
    float Cnbeta, Cndr, Cnda, Cnr;
    std::stringstream file;

    Cnbeta = _aircraft->_Cnbeta;
    Cndr = _aircraft->_Cndr;
    Cnda = _aircraft->_Cnda;
//  Cnp = _aircraft->_Cnp[0];
    Cnr = _aircraft->_Cnr;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <!-- Stall initiator -->" << std::endl;
    file << "    <function name=\"aero/moment/Yaw_alpha\">" << std::endl;
    file << "       <description>Yaw moment due to alpha</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/alpha-rad</property>" << std::endl;
    file << "           <property>fcs/gear-no-wow</property>" << std::endl;

    file << "           <table>" << std::endl;
        file << "             <independentVar lookup=\"row\">aero/beta-rad</independentVar>" << std::endl;
        file << "             <independentVar lookup=\"column\">aero/Re</independentVar>" << std::endl;
        file << "             <tableData>" << std::endl;
        float alpha = -MAX_ALPHA;
        file << std::setw(24) << "";
        for (int i=0; i<2; i++) {
            file << std::setw(9) << int(_aircraft->_Re[i]);
        }
        for (int j=0; j<2; ++j)
        {
            file << std::endl << std::setprecision(4) << std::setw(24) << alpha;
            for (int i=0; i<2; i++)
            {
                file << std::setw(9) << _aircraft->_Cna[j+2*i];
            }
            alpha = MAX_ALPHA;
        }
        file << std::endl;
        file << "             </tableData>" << std::endl;
        file << "           </table>" << std::endl;

    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Yaw_beta\">" << std::endl;
    file << "       <description>Yaw moment due to beta</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/beta-rad</property>" << std::endl;
    file << "           <value> " << (Cnbeta) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Yaw_roll_rate\">" << std::endl;
    file << "       <description>Yaw moment due to roll rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/p-rad_sec</property>" << std::endl;

    file << _print_vector(_aircraft->_Cnp);

    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Yaw_damp\">" << std::endl;
    file << "       <description>Yaw moment due to yaw rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/r-aero-rad_sec</property>" << std::endl;
    file << "           <value> " << (Cnr) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Yaw_rudder\">" << std::endl;
    file << "       <description>Yaw moment due to rudder</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>fcs/rudder-pos-rad</property>" << std::endl;
    file << "           <value> " << (Cndr) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/moment/Yaw_aileron\">" << std::endl;
    file << "       <description>Adverse yaw</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>fcs/left-aileron-pos-rad</property>" << std::endl;
    file << "           <value> " << (Cnda) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}


std::string CableControls::system()
{
    std::stringstream file;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "  <channel name=\"Pitch\">" << std::endl;
    file << "   <summer name=\"Pitch Trim Sum\">" << std::endl;
    file << "      <input>fcs/elevator-cmd-norm</input>" << std::endl;
    file << "      <input>fcs/pitch-trim-cmd-norm</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Elevator Control\">" << std::endl;
    file << "      <input>fcs/pitch-trim-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/elevator-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Elevator Normalization\">" << std::endl;
    file << "      <input>fcs/elevator-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/elevator-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"Roll\">" << std::endl;
    file << "   <summer name=\"Roll Trim Sum\">" << std::endl;
    file << "      <input>fcs/aileron-cmd-norm</input>" << std::endl;
    file << "      <input>fcs/roll-trim-cmd-norm</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Left Aileron Control\">" << std::endl;
    file << "      <input>fcs/roll-trim-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/left-aileron-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Right Aileron Control\">" << std::endl;
    file << "      <input>-fcs/roll-trim-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/right-aileron-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Left Aileron Normalization\">" << std::endl;
    file << "      <input>fcs/left-aileron-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/left-aileron-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Right Aileron Normalization\">" << std::endl;
    file << "      <input>fcs/right-aileron-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/right-aileron-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"Yaw\">" << std::endl;
    file << "   <summer name=\"Rudder Command Sum\">" << std::endl;
    file << "      <input>fcs/rudder-cmd-norm</input>" << std::endl;
    file << "      <input>fcs/yaw-trim-cmd-norm</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Rudder Control\">" << std::endl;
    file << "      <input>fcs/rudder-command-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/rudder-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Rudder Normalization\">" << std::endl;
    file << "      <input>fcs/rudder-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/rudder-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string YawDamper::system()
{
    std::stringstream file;

    file << "  <property value=\"1\">fcs/yaw-damper-enable</property>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"Pitch\">" << std::endl;
    file << "   <summer name=\"Pitch Trim Sum\">" << std::endl;
    file << "      <input>fcs/elevator-cmd-norm</input>" << std::endl;
    file << "      <input>fcs/pitch-trim-cmd-norm</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Elevator Control\">" << std::endl;
    file << "      <input>fcs/pitch-trim-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/elevator-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Elevator Normalization\">" << std::endl;
    file << "      <input>fcs/elevator-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/elevator-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"Roll\">" << std::endl;
    file << "   <summer name=\"Roll Trim Sum\">" << std::endl;
    file << "      <input>fcs/aileron-cmd-norm</input>" << std::endl;
    file << "      <input>fcs/roll-trim-cmd-norm</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Left Aileron Control\">" << std::endl;
    file << "      <input>fcs/roll-trim-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/left-aileron-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Right Aileron Control\">" << std::endl;
    file << "      <input>fcs/roll-trim-sum</input>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/right-aileron-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Left Aileron Normalization\">" << std::endl;
    file << "      <input>fcs/left-aileron-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/left-aileron-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Right Aileron Normalization\">" << std::endl;
    file << "      <input>fcs/right-aileron-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/right-aileron-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"Yaw\">" << std::endl;
    file << "   <summer name=\"Rudder Command Sum\">" << std::endl;
    file << "      <input>fcs/rudder-cmd-norm</input>" << std::endl;
    file << "      <input>fcs/yaw-trim-cmd-norm</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <scheduled_gain name=\"Yaw Damper Rate\">" << std::endl;
    file << "      <input>velocities/r-aero-rad_sec</input>" << std::endl;
    file << "      <table>" << std::endl;
    file << "        <independentVar lookup=\"row\">velocities/ve-kts</independentVar>" << std::endl;
    file << "         <tableData>" << std::endl;
    file << "            30     0.00" << std::endl;
    file << "            60     2.00" << std::endl;
    file << "         </tableData>" << std::endl;
    file << "      </table>" << std::endl;
    file << "      <gain>fcs/yaw-damper-enable</gain>" << std::endl;
    file << "   </scheduled_gain>" << std::endl;
    file << std::endl;
    file << "   <summer name=\"Rudder Sum\">" << std::endl;
    file << "      <input>fcs/rudder-command-sum</input>" << std::endl;
    file << "      <input>fcs/yaw-damper-rate</input>" << std::endl;
    file << "      <clipto>" << std::endl;
    file << "        <min> -1.1 </min>" << std::endl;
    file << "        <max>  1.1 </max>" << std::endl;
    file << "      </clipto>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Rudder Control\">" << std::endl;
    file << "      <input>fcs/rudder-sum</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -1.1 </min>" << std::endl;
    file << "        <max>  1.1 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/rudder-pos-rad</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Rudder Normalization\">" << std::endl;
    file << "      <input>fcs/rudder-pos-rad</input>" << std::endl;
    file << "      <domain>" << std::endl;
    file << "        <min> -0.35 </min>" << std::endl;
    file << "        <max>  0.35 </max>" << std::endl;
    file << "      </domain>" << std::endl;
    file << "      <range>" << std::endl;
    file << "        <min> -1 </min>" << std::endl;
    file << "        <max>  1 </max>" << std::endl;
    file << "      </range>" << std::endl;
    file << "      <output>fcs/rudder-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;
    file << std::endl;

    return file.str();
}

std::string FlyByWire::system()
{
    std::stringstream file;
    return file.str();
}

// ---------------------------------------------------------------------------

char const* System::_supported = "Does the aircraft include this system?";

std::string CableControls::_print_vector(std::vector<float>& C)
{
    std::stringstream file;

    file << std::fixed << std::showpoint;
    if (C.size() == 1) {
        file << "           <value> " << (C[0]) << " </value>" << std::endl;
    }
    else if (C.size() == 4)
    {
        file << "           <table>" << std::endl;
        file << "             <independentVar lookup=\"row\">aero/Re</independentVar>" << std::endl;
        file << "             <tableData>" << std::endl;
        for (int i=0; i<4; ++i) {
            file << std::setw(24) << int(_aircraft->_Re[i]) << std::setw(9) << std::setprecision(4) << C[i] << std::endl;
        }
        file << "             </tableData>" << std::endl;
        file << "           </table>" << std::endl;
    }
    else
    {
        file << "           <table>" << std::endl;
        file << "             <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
        file << "             <independentVar lookup=\"column\">aero/Re</independentVar>" << std::endl;
        file << "             <tableData>" << std::endl;
//      float Clra = (C[1] - C[0])/_aircraft->_alpha[1];
        float alpha = MIN_ALPHA;
        file << std::setw(24) << "";
        for (int i=0; i<4; i++) {
            file << std::setw(9) << int(_aircraft->_Re[i]);
        }
        for (int j=0; j<2; ++j)
        {
            file << std::endl << std::setprecision(4) << std::setw(24) << alpha;
            for (int i=0; i<4; i++)
            {
                file << std::setw(9) << C[j+2*i];
            }
            alpha = MAX_ALPHA;
        }
        file << std::endl;
        file << "             </tableData>" << std::endl;
        file << "           </table>" << std::endl;
    }

    return file.str();
}

void CableControls::_get_CLaw(std::vector<float>& CLaw, Aeromatic::_lift_device_t &wing)
{
    // lift coefficient gradient over angle of attack in incompressible flow
    float CLalpha_ic = 1.0f;
    float M, M2, k, R, e;

    // Wing dihedral
    float dihedral = wing.dihedral * DEG_TO_RAD;

    // Wing Seeep
    float sweep = wing.sweep * DEG_TO_RAD;
    float sweep_le = wing.sweep_le * DEG_TO_RAD;

    // aspect ratio
    float AR = wing.aspect;

    // taper ratio
    float TR = wing.taper;

    // max thickness
    float MT = 0.25f * wing.chord_mean;

    // Required to calculate CLalpha_wing
    float TRC = (1.0f - TR)/(1.0f + TR);
    float PAR = PI*AR;
    float AR2 = AR*AR;
    switch (wing.shape)
    {
    case ELLIPTICAL:
        CLaw[0] = PAR/2.0f;
        CLaw[1] = PAR/2.0f;
        CLaw[2] = PAR/2.0f;
        e = 1.0f;
        break;
    case DELTA:
        M = 0.0f; M2 = 0.0f;
        CLaw[0] = (2.0f*PAR) / (2.0f + sqrtf(AR2 * ((1.0f - M2 + powf((tanf(sweep_le) - 0.25f*AR*MT*TRC), 2.0f)) / powf((CLalpha_ic * sqrtf(1.0f - M2) / (2.0f*PI)), 2.0f)) + 4.0f));

        CLaw[1] = PAR/2.0f;

        M = 2.0f; M2 = M*M;
        CLaw[2] = 4.0f / (sqrtf(M2 - 1.0f)*(1.0f-TR/(2.0f*AR*sqrtf(M2 - 1.0f))));

        // Pamadi approximation for Oswald Efficiency Factor e
        k = (AR*TR) / cosf(sweep_le);
        R = 0.0004f*k*k*k - 0.008f*k*k + 0.05f*k + 0.86f;
        e = (1.1f* CLaw[0]) / (R* CLaw[0] + ((1.0f-R)*PAR));
        break;
    case VARIABLE_SWEEP:
    case STRAIGHT:
    default:
        M = 0.0f; M2 = 0.0f;
        CLaw[0] = (PAR*powf(cosf(dihedral), 2.0f)) / (1.0f + sqrtf(1.0f + 0.25f*AR2*(1.0f - M2)*(powf(tanf(sweep), 2.0f) + 1.0f)));

        CLaw[1] = PAR/2.0f;

        M = 2.0f; M2 = M*M;
        CLaw[2] = 4.0f / (sqrtf(M2 - 1.0f)*(1.0f-TR/(2.0f*AR*sqrtf(M2 - 1.0f))));

        // Pamadi approximation for Oswald Efficiency Factor e
        k = (AR*TR) / cosf(sweep_le);
        R = 0.0004f*k*k*k - 0.008f*k*k + 0.05f*k + 0.86f;
        e = (1.1f* CLaw[0]) / (R* CLaw[0] + ((1.0f-R)*PAR));
        break;
    }

    if (wing.efficiency == 0) {
        wing.efficiency = e;
    }
}

} /* namespace Aeromatic */

