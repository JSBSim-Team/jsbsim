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
 * https://archive.aoe.vt.edu/mason/Mason_f/ConfigAeroTransonics.pdf
 * http://www.aerostudents.com/courses/flight-dynamics/flightDynamicsFullVersion.pdf
 * http://www.aerostudents.com/courses/flight-dynamics/flightDynamicsFullVersion.pdf
 *
 * Formula's for CLalpha wing for different configurations:
 * http://aviation.stackexchange.com/questions/14508/calculating-a-finite-wings-lift-from-its-sectional-airfoil-shape
 *
 * See also:
 * https://web.archive.org/web/20180712182926/http://www.flightlevelengineering.com/downloads/stab.pdf
 */

#include <math.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <types.h>
#include <Aircraft.h>
#include "Controls.h"

#define MIN_ALPHA	(-2*DEG_TO_RAD)
#define MAX_ALPHA	( 20*DEG_TO_RAD)

namespace Aeromatic
{

Controls::Controls(Aeromatic *p) : System(p, true)
{
    _description.push_back("Aircraft control");

    Param *controls = new Param("Control system", 0, _ctype, MAX_CONTROL);
    _inputs_order.push_back("Control system");
    _inputs["Control system"] = controls;

    _control.push_back(new CableControls(p));
    controls->add_option(_control[0]->get_description());

    _control.push_back(new YawDamper(p));
    controls->add_option(_control[1]->get_description());

    _control.push_back(new FlyByWire(p));
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
    float Q = 0.5f*RHO*Vt*Vt;
    float CL = W/Q/Sw;

    if (Vs > 0.5f)
    {
        // *** CLmax based on wing geometry and stall speed ***
        _aircraft->_CLmax[0] = 2.0f*Ws/(RHO*Sw*Vs*Vs);

        if (_aircraft->_Mcrit == 0)
        {
            // *** Critical Mach based on wing geometry and stall speed ***
            float T_C = _aircraft->_wing.thickness/cbarw;

            // Korn  equation
            float CS = cosf(sweep_le);
            float CS2 = CS*CS, CS3 = CS2*CS;
            float Ka = _aircraft->_wing.Ktf;
            float Mdd = Ka/CS - T_C/CS2 - CL/(10.0f*CS3);
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
    float ch = cbarw*sqrtf(Sh/Sw);

    // drag
// https://www.fzt.haw-hamburg.de/pers/Scholz/HOOU/AircraftDesign_13_Drag.pdf
    float M2, M = 0.01f;

    float Cf;
    switch(_aircraft->_atype)
    {
    case LIGHT:
        if (_aircraft->_no_engines == 0) Cf = 0.0030f;
        else if (_aircraft->_no_engines == 1) Cf = 0.0055f;
        else Cf = 0.0045f;
        break;
    case PROP_TRANSPORT:
        Cf = 0.0035f;
        break;
    case JET_TRANSPORT:
        Cf = 0.0030f;
        break;
    case PERFORMANCE:
        Cf = 0.0035f;
        break;
    case FIGHTER:
        Cf = 0.0030f;
        break;
#if 0
    case BIPLANE:
        Cf = 0.0065f;
        break;
#endif
    default:
        Cf = 0.0030f;
        break;
    }

    // Fuselage
    float Df = _aircraft->get_fuselage_diameter();
    float Lf = _aircraft->_length;
    float fr = Lf/Df; // fuselage finess ratio

    float Qf = 1.0f;
    float FFf = 1.0f + 60.0f/powf(fr, 3.0f) + fr/400.0f;
    float Swet_f = PI*Df*Lf*powf(1.0f-2.0f/fr, 0.667f)*(1.0f + 1.0f/(fr*fr));
    float CD0f = Cf*FFf*Qf*Swet_f/Sw;

    // Main Wing
    float Qw = 1.0f;
    float T_Cw = _aircraft->_wing.thickness/cbarw;
    float FFw = 1.0f + 2.7f*T_Cw + 100.0f*powf(T_Cw, 4.0f);
    float Swet_w = 2.0f*(1.0f + 0.25f*T_Cw)*bw*cbarw;
    float CD0w = Cf*FFw*Qw*Swet_w/Sw;
    CD0w *= (1.0f - sinf(sweep));

    // Horizontal tail and vertical tail are aerodynamically clean
    Cf = 0.0025f;

    // Horizontal tail
    float bh = _aircraft->_htail.span;
    float cbarh = _aircraft->_htail.chord_mean;
    float T_Ch = _aircraft->_htail.thickness/cbarh;

    float Qh = 1.03f;
    float FFh = 1.0f + 2.7f*T_Ch + 100.0f*powf(T_Ch, 4.0f);
    float Swet_h = 2.0f*(1.0f + 0.25f*T_Ch)*bh*cbarw;
    float CD0h = Cf*FFh*Qh*Swet_h/Sw;

    // Vertical tail
    float bv = _aircraft->_vtail.span;
    float cbarv = _aircraft->_vtail.chord_mean;
    float T_Cv = _aircraft->_vtail.thickness/cbarv;

    float Qv = 1.03f;
    float FFv = 1.0f + 2.7f*T_Cv + 100.0f*powf(T_Cv, 4.0f);
    float Swet_v = 2.0f*(1.0f + 0.25f*T_Cv)*bv*cbarw;
    float CD0v = Cf*FFv*Qv*Swet_v/Sw;

    // Sum Drag coefficients
    _aircraft->_CD0 = CD0f + CD0w + CD0h + CD0v;


    // lift
    float alpha = 0;
    float Ew = _aircraft->_wing.efficiency;
    float E0 = Ew*(CL/AR);
    float de = 0;
    float CLh = W/Q/Sh;
    float a0w = _aircraft->_wing.incidence*DEG_TO_RAD;
    float iw = ((CL/CLaw[0]) + a0w - alpha);
    float ih = ((CLh/CLah[0]) - (alpha*(1.0f-deda) - E0 + Ee*de));

    float Sv = _aircraft->_vtail.area;
    float lv = _aircraft->_vtail.arm;
    float Vv = Sv*lv/bw/Sw;

    float nv = _aircraft->_vtail.efficiency;
    float Er = _aircraft->_vtail.flap_ratio;    // rudder

    M = 0.0f;
    M2 = M*M;

    float dsdB = -_aircraft->_vtail.de_da;          // ds/dB
    float CYbeta = -nv*Sv/Sw*CLav[0]*(1.0f-dsdB);

    float CL0 = _aircraft->_CL0;
    if (Vs > 0.5f)
    {
        CL0 = CLaw[0]*(iw - a0w);
        if (_aircraft->_wing.shape != DELTA) {
            CL0 += (Sh/Sw)*nh*CLah[0]*(ih - E0);
        }
        _aircraft->_CL0 = CL0;
    }

    float CLalpha = _aircraft->_CLalpha[0];
    _aircraft->_CDalpha.at(0) = CLalpha*(2.0f*CL0)/(PI*AR*Ew);

    float zw = -0.0f; // z-pos. wing: positive down
    float zv = -1.0f; // z-pos. vertical tail:  positive down
    float Clbwf = 1.2f*sqrt(AR)*((zw+2.0f*Df)/(bw*bw));
    float Clbvt = -(zv/bw)*CLah[0];
    for (int i=0; i<4; ++i)
    {
        switch (i)
        {
        case 0:
            if (Vs > 0.5f)
            {
                CL = 10.0f * _aircraft->_CLmax[0];
                Vt = Vs;
            }
            else	// No stall speed was specified
            {
                CL = CL0;
                Vt = sqrtf(W/(0.5f*RHO*CL*Sw));
            }
            break;
        case 1:
            Vt = 1.1f*Vs;
            Q = 0.5f*RHO*Vs*Vs;
            CL = W/Q/Sw;
            break;
        case 2:
            Vt = 1.5f*Vs;
            Q = 0.5f*RHO*Vt*Vt;
            CL = W/Q/Sw;
            break;
        case 3:
            CL = CL0;
            Vt = sqrtf(W/(0.5f*RHO*CL*Sw));
            break;
        }
        _aircraft->_Re.at(i) = (Vt * cbarw)*6372.38970987f; // 1/0.000156927

        alpha = (CL-CL0)/CLalpha;
        _aircraft->_alpha.at(i) = alpha;

#if 0
        float CYp_const = (AR+cosf(sweep))/(AR+4*cosf(sweep))*tanf(sweep);
        _aircraft->_CYp.at(i) = CL*CYp_const;
        _aircraft->_Cnp.at(i) = -CL/8.0f;
#else
        float l0a = 4*Sw*CL/PI/bw;
        float av = l0a*(PI/4.0f)*bv/Sv;
        _aircraft->_CYp.at(i) = 8.0f/(3.0f*PI)*nv*(bv*Sv/(bw*Sw))*av;
        _aircraft->_Cnp.at(i) = -lv*_aircraft->_CYp[i]/bw;
#endif

        float CLmin = CL0 + MIN_ALPHA*CLalpha;
        float CLmax = CL0 + std::max<float>(alpha,MAX_ALPHA)*CLalpha;
        float Cmin, Cmax;

        // From Flight Dynamics by Robert F. Stengel page 99
        M = Vt / 661.5f; M2 = M*M;
        Cmin = (-((1.0f+2.0f*TR)/(6.0f+6.0f*TR))*(dihedral*CLaw[0] + (CLmin*tanf(sweep)/(1.0f-M2*powf(cosf(sweep), 2.0f)))));
        Cmax = (-((1.0f+2.0f*TR)/(6.0f+6.0f*TR))*(dihedral*CLaw[0] + (CLmax*tanf(sweep)/(1.0f-M2*powf(cosf(sweep), 2.0f)))));

        _aircraft->_Clbeta.at(i*2) = Cmin - Clbwf - alpha*Clbvt;
        _aircraft->_Clbeta.at(i*2+1) = Cmax - Clbwf - alpha*Clbvt;
        _aircraft->_Clbeta.at(i*2+2) = Cmin - Clbwf;

        float Clr_const = 2.0f*lv*zv/bw/bw*CYbeta;
        Cmin = (CLmin/4.0f)-Clr_const;
        Cmax = (CLmax/4.0f)-Clr_const;
        _aircraft->_Clr.at(i*2) = Cmin;
        _aircraft->_Clr.at(i*2+1) = Cmax;
        _aircraft->_Clr.at(i*2+2) = (CLmin/2.0f)-Clr_const;

        if (Vs <= 0.5f) break;
    }

    _aircraft->_CLq = 2.0f*nh*Vh*CLah[0];
    _aircraft->_CLadot = _aircraft->_CLq*deda;

    float CLhde = ((CLah[0]/PI)*(acosf(1.0f-2.0f*Ee)+2.0f*sqrtf(Ee*(1.0f-Ee))));
    _aircraft->_CLde = Sh*CLhde/Sw; // *2.0f/PI;

    // pitch
    if (_aircraft->_user_wing_data > 0)
    {
        float dcgx = -(cg_loc[X] - _aircraft->_aero_rp[X])*INCH_TO_FEET;

        // fuselage component: L = fuselage length, D = fuselage max. diameter.
        float dwf = Lf/_aircraft->_aero_rp[X];
        float Kf = 0.033f + 0.538f*dwf + 1.5f*dwf*dwf;
        float Cmaf = -Kf*Df*Df*Lf/Sw/cbarw/CLaw[0];

        _aircraft->_Cmalpha = CLaw[0]*(dcgx/cbarw) - Vh*CLah[0]*(1.0f-deda) + Cmaf;
        _aircraft->_Cmq = -_aircraft->_CLq*(lh/cbarw);
        _aircraft->_Cmadot = -_aircraft->_CLadot*(lh/cbarw);

        float Cmtde = CLah[0]/PI*(1.0f-Ee)*sqrtf(Ee*(1.0f-Ee));
        _aircraft->_Cmde = (Sh*ch/Sw/cbarw*Cmtde - lh*Sh*CLhde/cbarw/Sw);
    }

    // side
    float Cltdr = (CLav[0]/PI)*(acosf(1.0f-2.0f*Er)+2.0f*sqrtf(Er*(1.0f-Er)));
    _aircraft->_CYbeta = CYbeta;
    _aircraft->_CYr = -2.0f*(lv/bw)*(CYbeta);
    _aircraft->_CYdr = (Sv/Sw)*Cltdr;

    // roll
    float TRh = _aircraft->_htail.taper;
    float TRv = _aircraft->_vtail.taper;
    _aircraft->_Clp = -(CLaw[0]/12.0f)*(1.0f+3.0f*TR)/(1.0f+TR)
                      + (CLah[0]/12.0f)*(Sh/Sw)*(1.0f+3.0f*TRh)/(1.0f+TRh)
                      + (CLav[0]/12.0f)*(Sv/Sw)*(1.0f+3.0f*TRv)/(1.0f+TRv);

    // yaw
    float k0 = 0.075f;
    float k1 = 1.0f+T_Cw;	// correction factor for wing thickness
    _aircraft->_Cnbeta = nv*Vv*CLav[0]; // *(1.0f-dsdB);
    _aircraft->_Cnr = -2.0f*nv*Vv*CLav[0]*(lv/bw) - (k0*CL*CL + k1*CD0w);
    _aircraft->_Cndr = -Vv*Cltdr;
}


std::string CableControls::lift()
{
    float CLalpha, CLa_vortex, CLmax, CL0, CLde, CLq, CLadot;
    float alpha_CLmax, alpha0, T_C;
    std::stringstream file;

    T_C = _aircraft->_wing.thickness/_aircraft->_wing.chord_mean;
    CLalpha = _aircraft->_CLalpha[0];
    CLmax = _aircraft->_CLmax[0];
    CL0 = _aircraft->_CL0;
    CLq = _aircraft->_CLq;
    CLadot = _aircraft->_CLadot;
    CLde = _aircraft->_CLde;

    CLa_vortex = 0.0f;
    if (_aircraft->_wing.shape == DELTA) { // account for vortex lift
        CLalpha /= 3.3f;
        CLa_vortex = 0.5f*CLmax;
    }
    alpha0 = CL0/CLalpha;
    alpha_CLmax = CLmax/CLalpha;

    if ((alpha0-alpha_CLmax) >= 0.88f) {
        std::cerr << std::endl;
        std::cerr << "*** ERROR: The alpha value for maximum lift is too high." << std::endl;
        std::cerr << "           This means the specified Stall Speed was too low." << std::endl;
        std::cerr << "           Make sure it is for a clean (no gear and no flaps) configurtion." << std::endl;
        std::cerr << std::endl;
    }

    // Post stall behaviour is loosly based on:
    // http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/20140000500.pdf

    file << "    <!-- Lift above 0.85 and below -0.85 is generalised -->" << std::endl;
    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Lift_alpha\">" << std::endl;
    file << "      <description>Lift due to alpha</description>" << std::endl;
    file << "      <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "              -1.57  0.0000" << std::endl;
    file << "              -1.22 " << std::setw(6) << (-0.6428*(1.0f-T_C)) << std::endl;
    file << "              -1.05 " << std::setw(6) << (-0.8660*(1.0f-T_C)) << std::endl;
    file << "              -0.88 " << std::setw(6) << (-1.0f*(1.0f-T_C)) << std::endl;

    float alpha = alpha0-alpha_CLmax;
    float CL = -(CLmax-(0.6*alpha_CLmax*CLalpha)-CL0);
    file << "              " << std::setprecision(2)
                             << alpha << " "
                             << std::setw(6) << std::setprecision(4)
                             << CL << std::endl;

    alpha = 0.0f;
    CL = CL0;
    file << "               " << std::setprecision(2)
                              << alpha << std::setprecision(4) << "  "
                              << CL << std::endl;

    alpha = alpha_CLmax;
    CL = (CLmax);
    file << "               " << std::setprecision(2)
                              << alpha << std::setprecision(4) << "  "
                              << CL << std::endl;

    alpha = alpha0+alpha_CLmax;
    CL = 0.4f*alpha_CLmax*(CLalpha+CLa_vortex);
    file << "               " << std::setprecision(2)
                              << alpha << std::setprecision(4) << "  "
                              << CL << std::endl;

    file << "               0.88  " << std::setw(6) << (1.0f*(1.0f+T_C)) << std::endl;
    file << "               1.05  " << std::setw(6) << (0.8660*(1.0f+T_C)) << std::endl;
    file << "               1.22  " << std::setw(6) << (0.6428*(1.0f+T_C)) << std::endl;
    file << "               1.57  0.0000" << std::endl;
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
    file << "    <!-- CD0 is based on fuselage, wing, horizontal- en vertical tail -->" << std::endl;
    file << "    <!-- CD for gear (fixed and retractable) is defined below         -->" << std::endl;
    file << "    <function name=\"aero/force/Drag_minimum\">" << std::endl;
    file << "       <description>Minimum drag</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <value> " << (CD0) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;
    file << std::endl;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Drag_alpha\">" << std::endl;
    file << "       <description>Drag due to alpha</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "             -1.57    " << (CDmax) << std::endl;
    file << "             " << std::setprecision(2) << (-alpha) << "    " << std::setprecision(4) << (alpha * CDalpha) << std::endl;
    file << "              0.00    " << 0.0f << std::endl;
    file << "              " << std::setprecision(2) << (alpha) << "    " << std::setprecision(4) << (alpha * CDalpha) << std::endl;
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

    if (_aircraft->_Clbeta.size() > 1) {
        file << "           <property>fcs/gear-no-wow</property>" << std::endl;
    }
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

    if (_aircraft->_Re[1] != 0.0f)
    {
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
            file << std::setw(12) << int(_aircraft->_Re[i]);
        }
        for (int j=0; j<2; ++j)
        {
            file << std::endl << std::setprecision(4) << std::setw(24) << alpha;
            for (int i=0; i<2; i++)
            {
                file << std::setw(12) << _aircraft->_Cna[j+2*i];
            }
            alpha = MAX_ALPHA;
        }
        file << std::endl;
        file << "             </tableData>" << std::endl;
        file << "           </table>" << std::endl;
        file << "       </product>" << std::endl;
        file << "    </function>" << std::endl;
        file << std::endl;
    }
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
    if (C.size() == 1 || _aircraft->_Re[1] == 0.0f) {
        file << "           <value> " << (C[0]) << " </value>" << std::endl;
    }
    else if (C.size() == 4)
    {
        file << "           <table>" << std::endl;
        file << "             <independentVar lookup=\"row\">aero/Re</independentVar>" << std::endl;
        file << "             <tableData>" << std::endl;
        for (int i=0; i<4; ++i) {
            file << std::setw(24) << int(_aircraft->_Re[i]) << std::setw(12) << std::setprecision(4) << C[i] << std::endl;
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
            file << std::setw(12) << int(_aircraft->_Re[i]);
        }
        for (int j=0; j<2; ++j)
        {
            file << std::endl << std::setprecision(4) << std::setw(24) << alpha;
            for (int i=0; i<4; i++)
            {
                file << std::setw(12) << C[j+2*i];
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
    float M, M2, e;

    // Wing dihedral
    float dihedral = wing.dihedral * DEG_TO_RAD;

    // Wing Seeep
    float sweep = wing.sweep * DEG_TO_RAD;
    float sweep_le = wing.sweep_le * DEG_TO_RAD;

    // aspect ratio
    float AR = wing.aspect;

    // taper ratio
    float TR = wing.taper;

    // Required to calculate CLalpha_wing
    float MC, TRC = (1.0f - TR)/(1.0f + TR);
    float PAR = PI*AR;
    float AR2 = AR*AR;
    switch (wing.shape)
    {
    case ELLIPTICAL:
        CLaw[0] = PAR/2.0f;
        CLaw[1] = PAR/2.0f;
        CLaw[2] = PAR/2.0f;
        break;
    case DELTA:
    {
        M = 0.3f; M2 = 0.0f;
        MC = sqrtf(1.0f - M2);

        float xdmax_l = 0.93f; //  chordwise pos. of maximum airfoil thickness

        CLaw[0] = 2.0f + sqrtf(AR2*((1.0/(1.0f - M2)) + powf((tanf(sweep_le) - (4.0*xdmax_l/AR)*TRC), 2.0f)/powf(CLalpha_ic/(2.0f*PI*MC), 2.0f)) + 4.0f);

        CLaw[1] = PAR/2.0f;

        M = 2.0f; M2 = M*M;
        MC = sqrtf(M2 - 1.0f);
        CLaw[2] = (4.0f/MC)*(1.0f - TR/(2.0f*AR*MC));
        break;
    }
    case VARIABLE_SWEEP:
    case STRAIGHT:
    default:
        M = 0.0f; M2 = 0.0f;
        CLaw[0] = (PAR*powf(cosf(dihedral), 2.0f)) / (1.0f + sqrtf(1.0f + (AR2/4.0f)*(powf(tanf(sweep), 2.0f) + 1.0f - M2)));

        CLaw[1] = PAR/2.0f;

        M = 2.0f; M2 = M*M;
        MC = sqrtf(M2 - 1.0f);
        CLaw[2] = (4.0f/MC)*(1.0f - TR/(2.0f*AR*MC));
        break;
    }

    if (wing.efficiency == 0)
    {
        if (wing.shape != ELLIPTICAL)
        {
#if 0
/*
 * Raymer, D.
 * Aircraft Design: A Conceptual Approach, 1999
 */
            if (fabsf(sweep_le) <  0.05f) {
                e = 1.78f*(1.0f - 0.045f*powf(AR, 0.68f)) - 0.64f;
            } else {
                e = 4.61f*(1.0f - 0.045f*powf(AR, 0.86f))*powf(cosf(sweep_le), 0.15f) - 3.1f;
            }

#elif 0
/*
 * Pamadi, Bandu N.
 * Performance, Stability, Dynamics, & Control, 2004
 */
            float k = (AR*TR) / cosf(sweep_le);
            float R = 0.0004f*k*k*k - 0.008f*k*k + 0.05f*k + 0.86f;
            e = (1.1f* CLaw[0]) / (R* CLaw[0] + ((1.0f-R)*PAR));
#else
/*
 * Scholz, D. and Niță, M.
 * Comparison of different methods of estimating the Oswald factor, 2012
 * http://www.fzt.haw-hamburg.de/pers/Scholz/OPerA/OPerA_PUB_DLRK_12-09-10.pdf
 */
            float e_theo, fY, TR2, TRopt, kf, kd;
            float bw = _aircraft->_wing.span;

            TRopt = 0.45f*expf(-0.0375f*sweep);
            TR -= TRopt;
            TR2 = TR*TR;
            fY = 0.0524f*TR2*TR2-0.15f*TR2*TR+0.1659f*TR2-0.0706f*TR+0.0119f;
            e_theo = 1.0f / (1.0f + fY*AR);

            kf = 1.0f - 2.0f*powf(_aircraft->get_fuselage_diameter()/bw, 2.0f);
            kd = powf(1.0f/cosf(dihedral), 2.0f);
            e = e_theo * kf * kd;
#endif
        }
        else {	// wing.shape == ELLIPTICAL
            e = 1.0f;
        }

        wing.efficiency = e;
    }
}

} /* namespace Aeromatic */
