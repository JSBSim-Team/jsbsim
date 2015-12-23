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
 * https://www.princeton.edu/~stengel/MAE331Lecture4.pdf
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
    float *CLaw = _aircraft->_CLaw;
    float *CLah = _aircraft->_CLah;
    float *CLav = _aircraft->_CLav;
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

    // *** Pitch moment ***
    float Sh = _aircraft->_htail.area;
    float lh = _aircraft->_htail.arm;
    float Vh = lh*Sh/cbarw/Sw;
  
    float nh = _aircraft->_htail.efficiency;
    float Ee = _aircraft->_htail.flap_ratio;	// elevator
    float cgx = -cg_loc[Z]*INCH_TO_FEET;
    float ch = cbarw*sqrtf(Sh/Sw);

    float Cmtde = CLah[0]/PI*(1.0f-Ee)*sqrtf(Ee-Ee*Ee);
    float Cltde = ((CLah[0]/PI)*(acosf(1.0f-2.0f*Ee)+2.0f*sqrtf(Ee-Ee*Ee)));

    // lift
    _aircraft->_CLalpha[0] = CLaw[0]+CLah[0]*Sh/Sw*(1.0f-deda);
    _aircraft->_CLalpha[1] = CLaw[1];
    _aircraft->_CLalpha[2] = CLaw[2];
    _aircraft->_CLadot = 2.0f*nh*CLah[0]*Vh*deda;
    _aircraft->_CLq = _aircraft->_CLadot/deda;
    _aircraft->_CLde = (Cltde*Sh/Sw)*2.0f/PI;

    // drag
    float Ew = _aircraft->_wing.efficiency;
    _aircraft->_CDalpha = (2.0f*CL*_aircraft->_CLalpha[0])/(PI*AR*Ew);

    // pitch
    if (_aircraft->_user_wing_data > 0)
    {
        _aircraft->_Cmalpha =  CLaw[0]*(cgx/cbarw) - Vh*CLah[0]*(1.0f-deda);
        _aircraft->_Cmadot = -_aircraft->_CLq*lh/cbarw*deda;
        _aircraft->_Cmq = _aircraft->_Cmadot/deda;
        _aircraft->_Cmde = (Sh*ch/Sw/cbarw*Cmtde - lh*Sh*Cltde/cbarw/Sw);
    }

    float Sv = _aircraft->_vtail.area;
    float lv = _aircraft->_vtail.arm;
    float Vv = Sv*lv/bw/Sw;

    float nv = _aircraft->_vtail.efficiency;
    float Er = _aircraft->_vtail.flap_ratio;	// rudder

    float dsdB = 0.0f;		// ds/dB

    float CYbeta = -nv*Sv/Sw*CLav[0]*(1.0f+dsdB);
    float Cltdr = (CLav[0]/PI)*(acosf(1.0f-2.0f*Er)+2.0f*sqrtf(Er-Er*Er));
    float CYp_const = (AR+cosf(sweep))/(AR+4*cosf(sweep))*tanf(sweep);
    float Clr_const = 2.0f*lv*lv/bw/bw*CYbeta;

    // side
    _aircraft->_CYbeta = CYbeta;
    _aircraft->_CYr = -2.0f*(lv/bw)*(CYbeta);
    _aircraft->_CYp = -CL*CYp_const;
    _aircraft->_CYdr = (Sv/Sw)*Cltdr;

    // roll
    float M2 = 0.0f;
    _aircraft->_Clbeta = -((1.0f+2.0f*TR)/(6.0f+6.0f*TR))*(dihedral*CLaw[0] + (CL*tanf(sweep)/(1.0f-M2*powf(cosf(sweep), 2.0f))));
    _aircraft->_Clp = -(CLaw[0]/12.0f)*(1.0f+3.0f*TR)/(1.0f+TR);
    _aircraft->_Clr = ((CL/4.0f)-Clr_const);

    // yaw
    _aircraft->_Cnbeta = nv*Vv*CLav[0]*(1+dsdB);
    _aircraft->_Cnr = -2.0f*nv*Vv*(lv/bw)*CLav[0];
    _aircraft->_Cndr = -Vv*Cltdr;
    _aircraft->_Cnp = -CL/8.0f;

#if 0
    // Sfus: Fuselage wetted area
    float TC = _aircraft->_wing.thickness/cbarw;
    float k1 = 0.2f*TC; // 1.256f; // correction factor for wing thickness
    float k2 = 1.12f;		// fuselage fineness ratio correction factor
    float Cf = 0.006f;		// skin Friction Coefficient
    float fwings = Cf*(Sw+Sh+Sv)*2.04f*k1;
    float ffus   = Cf*Sfus*k2;
    _aircraft->_CD0 = (fwings+ffus)/Sw;
#endif

#if 0
printf("CLa: %f, CLmax: %f, CLde: %f\n", _aircraft->_CLalpha[0], _aircraft->_CLmax[0], _aircraft->_CLde);
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
    file << "        <description>Lift_due_to_pitch_rate</description>" << std::endl;
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
    file << "        <description>Lift_due_to_alpha_rate</description>" << std::endl;
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
    float CD0, CDmax, CDalpha, K, Mcrit, CDbeta, CDde;
    std::stringstream file;

    CD0 = _aircraft->_CD0;
    K = _aircraft->_Kdi;
    Mcrit = _aircraft->_Mcrit;
    CDalpha = _aircraft->_CDalpha;
    CDbeta = _aircraft->_CDbeta;
    CDde = _aircraft->_CDde;

    float AR = _aircraft->_wing.aspect;
    float sweep = _aircraft->_wing.sweep * DEG_TO_RAD;
    float Ew = _aircraft->_wing.efficiency;

    float CL0 = _aircraft->_CL0;
    float CLmax = _aircraft->_CLmax[0];
    float CLalpha = _aircraft->_CLalpha[0];
    float alpha = (CLmax-CL0)/CLalpha;

    CD0 = (1.0f - sinf(sweep)) * CD0;
    K = 1.0f/(PI * fabs(Ew) * AR);

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
    file << "           <value> " << (K) << " </value>" << std::endl;
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
    float CYbeta, CYp, CYr, CYdr;

    CYbeta = _aircraft->_CYbeta;
    CYp = _aircraft->_CYp;
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
    file << std::endl;
    file << "    <function name=\"aero/force/Side_roll_rate\">" << std::endl;
    file << "       <description>Side_force_due_to_roll_rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/p-aero-rad_sec</property>" << std::endl;
    file << "           <value> " << (CYp) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;
    file << "    <function name=\"aero/force/Side_yaw_rate\">" << std::endl;
    file << "       <description>Side_force_due_to_yaw_rate</description>" << std::endl;
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
    file << "       <description>Side_force_due_to_rudder</description>" << std::endl;
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
    float Clbeta, Clp, Clr, Clda, Cldr;
    std::stringstream file;

    Clbeta = _aircraft->_Clbeta;
    Clp = _aircraft->_Clp;
    Clr = _aircraft->_Clr;
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
    file << "           <value> " << (Clbeta) << " </value>" << std::endl;
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
    file << "           <value> " << (Clr) << " </value>" << std::endl;
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
    float Cnbeta, Cndr, Cnda, Cnp, Cnr;
    std::stringstream file;

    Cnbeta = _aircraft->_Cnbeta;
    Cndr = _aircraft->_Cndr;
    Cnda = _aircraft->_Cnda;
    Cnp = _aircraft->_Cnp;
    Cnr = _aircraft->_Cnr;

    file << std::setprecision(4) << std::fixed << std::showpoint;
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
    file << "    <function name=\"aero/moment/Yaw_rol_rate\">" << std::endl;
    file << "       <description>Yaw_moment_due_to_roll_rate</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/bi2vel</property>" << std::endl;
    file << "           <property>velocities/p-rad_sec</property>" << std::endl;
    file << "           <value> " << (Cnp) << " </value>" << std::endl;
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

void CableControls::_get_CLaw(float CLaw[3], Aeromatic::_lift_device_t &wing)
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

