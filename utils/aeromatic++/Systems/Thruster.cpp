// Thruster.cpp -- Implements the Propulsion Thruster types.
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

#include <math.h>
#include <sstream>
#include <iomanip>

#include <Aircraft.h>
#include "Propulsion.h"
#include "Thruster.h"

namespace Aeromatic
{

#define FACT			2.667f

Thruster::Thruster(Propulsion *p) :
    _propulsion(p)
{
}

Thruster::~Thruster()
{
    for (auto it : _inputs) {
        delete it.second;
    }
}


Direct::Direct(Propulsion *p) : Thruster(p)
{
    strCopy(_thruster_name, "direct");
}

std::string Direct::thruster()
{
    std::stringstream file;

    file << "<!--" << std::endl;
    file << "    See: http://wiki.flightgear.org/JSBSim_Thrusters#FGDirect" << std::endl;
    file << std::endl;
    file << "    Thrust is computed directly by the engine" << std::endl;
    file << "-->" << std::endl;
    file << std::endl;
    file << "<direct name=\"Direct\">" << std::endl;
    file << "</direct>" << std::endl;

    return file.str();
}


Nozzle::Nozzle(Propulsion *p) : Thruster(p)
{
    bool& convert = _propulsion->_aircraft->_metric;

    strCopy(_thruster_name, "my_nozzle");

    _inputs_order.push_back("nozzleName");
    _inputs["nozzleName"] = new Param("Nozzle name", "The name is used for the configuration file name", _thruster_name);
    _inputs_order.push_back("nozzleDiameter");
    _inputs["nozzleDiameter"] = new Param("Nozzle diameter", "Nozzle diameter influences the nozzle area and exit pressure", _diameter, convert, LENGTH);
}

std::string Nozzle::thruster()
{
    std::stringstream file;

    float area = _diameter * _diameter * PI/4;
    float pe = area / _propulsion->_power;

    file << "<!--" << std::endl;
    file << "    See:  http://wiki.flightgear.org/JSBSim_Thrusters#FGNozzle" << std::endl;
    file << std::endl;
    file << "    pe      = Nozzle exit pressure, psf." << std::endl;
    file << "    area    = Nozzle exit area, sqft." << std::endl;
    file << "  -->" << std::endl;
    file << std::endl;
    file << "<nozzle name=\"" << _thruster_name << "\">" << std::endl;
    file << "  <pe unit=\"PSF\"> " << pe << " </pe>" << std::endl;
    file << "  <area unit=\"FT2\"> " << area << " </area>" << std::endl;
    file << "</nozzle>" << std::endl;

    return file.str();
}


#define NUM_PROP_PITCHES	6
#define NUM_ELEMENTS		12
Propeller::Propeller(Propulsion *p) : Thruster(p)
{
    bool& convert = _propulsion->_aircraft->_metric;

    strCopy(_thruster_name, "my_propeller");

    _inputs_order.push_back("thrusterName");
    _inputs["thrusterName"] = new Param("Thruster name", "The name is used for the configuration file name", _thruster_name);
    _inputs_order.push_back("propellerDiameter");
    _inputs["propellerDiameter"] = new Param("Propeller diameter", "Propeller diameter is critical for a good thrust estimation", _diameter, convert, LENGTH);
    _inputs_order.push_back("propellerFixedPitch");
    _inputs["propellerFixedPitch"] = new Param("Is the propeller fixed pitch?", "Fixed pitch propellers do not have any mechanics to alter the pitch angle", _fixed_pitch);
}

/**
 * Option for using blade element theory:
 * http://www-mdp.eng.cam.ac.uk/web/library/enginfo/aerothermal_dvd_only/aero/propeller/prop1.html
 *
 * A simple propeller design with linear properties:
 * http://www-mdp.eng.cam.ac.uk/web/library/enginfo/aerothermal_dvd_only/aero/propeller/propel.txt
 * However with the inclusion of your own propeller geometry and section data
 * a more accurate analysis of the specific propeller design can be obtained.
 *
 * http://www.icas.org/ICAS_ARCHIVE/ICAS2010/PAPERS/434.PDF
 *
 * http://www.engineeringtoolbox.com/density-solids-d_1265.html
 * http://www.engineeringtoolbox.com/wood-density-d_40.html
 * Mahogany:	 41
 * Walnut:	 35
 * Oak:		 46
 * Aluminum:	167
 *
 * The code is based on:
 *   PROPELLER BLADE ELEMENT MOMENTUM THEORY WITH VORTEX WAKE DEFLECTION
 *   M. K. Rwigema, 2010
 *   School of Mechanical, Industrial and Aeronautical Engineering
 *   http://www.icas.org/ICAS_ARCHIVE/ICAS2010/PAPERS/434.PDF
 */
void  Propeller::bladeElement()
{
    const float Cf = 0.006f;	  // skin Friction Coefficient
    const float k1 = 0.2f;	  // correction factor for airfoil thickness

    float Y = _specific_weight;
    float density = _density_factor;
    float RPM = _max_rpm;
    float D = _diameter;
    float B = _blades;
    float R = 0.5f*D;

    if (_max_chord == 0) {
        _max_chord = 0.76f*sqrtf(R/B);
    }
    float max_thickness = 0.3f*_max_chord;
    float max_camber = 0.035f;

    float xt = R;
    float xs = R/NUM_ELEMENTS;
    float hub = 0.5f*PI*atanf(_pitch/(2.0f*PI*xs*FEET_TO_INCH))*RAD_TO_DEG;
    float tip = 0.5f*PI*atanf(_pitch/(2.0f*PI*xt*FEET_TO_INCH))*RAD_TO_DEG;

    float n = RPM/60.0f;
    float n2 = n*n;
    float D4 = D*D*D*D;
    float D5 = D4*D;

    float omega = 2.0f*PI*n;
    float coef1 = (tip-hub)/(xt-xs);
    float coef2 = hub - coef1*xs;
    float rstep = (xt-xs)/NUM_ELEMENTS;

    float pitch = _fixed_pitch ? 0.0f : -15.0f;
    do
    {
        float step = 0.05f;
        for (float J=1e-9; J<2.4f; J += step)
        {
            _ixx = 0.0f;

            if (J > 1.36f) step = 0.1f;

            float V = J*n*D;
            float thrust = 0.0f;
            float torque = 0.0f;
            for (unsigned i=0; i<NUM_ELEMENTS; ++i)
            {
                float rad = xs + i*rstep;
                float theta = coef1*rad + coef2+pitch;
                float th = theta*DEG_TO_RAD;
                float a = 0.1f;
                float b = 0.01f;
                int finished = 0;
                int sum = 1;

                float r = rad/xt;
                float x = 1.0f - r;

                float crd = 0.055f + powf(x,0.1f) - powf(x,10.0f);
                float toc = 0.03f + 1.374f*powf(x,4.0f);
                float tw = 0.25f + 0.84f*powf(x,1.15f);
                float chord =_max_chord*crd;
                float TC = max_thickness*toc/chord;
                float CC = max_camber*tw;

                float AR = rstep/chord;
                float PAR = PI*AR;
                float eff = 0.71 + (i*0.23f/NUM_ELEMENTS);
                float CL0 = 4.0f*PI*CC;
                float CLa = PAR/(1.0f + sqrtf(1.0f + 0.25f*AR*AR));
                float CDi = 1.0f/(eff*B*PAR);
                float CD0 = TC*k1*Cf + 0.3333f*CDi*CL0*CL0;
                float CDa = CLa*CDi;

                float DtDr, DqDr, tem1, tem2, anew, bnew;
                do
                {
                    float V0 = V*(1.0f+a);
                    float V2 = omega*rad*(1.0f-b);
                    float phi = atan2f(V0,V2);
                    float sphi = sinf(phi);
                    float cphi = cosf(phi);
                    float alpha = th-phi;

                    float CL = std::min<float>(std::max<float>(CL0 + alpha*CLa, -1.42f), 1.42f);
                    float CD = CD0 + alpha*CDa*CL + CDi*CL*CL;
                    float CY = CL*cphi - CD*sphi;
                    float CX = CD*cphi + CL*sphi;

                    // Blade element momentum theory
                    float solidity = B*chord/(PI*R);
                    DtDr = solidity*PI*RHO*V0*V0/(sphi*sphi)*rad*CY;
                    DqDr = solidity*PI*RHO*V0*V0/(sphi*sphi)*rad*rad*CX;

                    tem1 = DtDr/(4.0f*PI*rad*RHO*V*V*(1.0f+a));
                    tem2 = DqDr/(4.0f*PI*rad*rad*rad*RHO*V*(1.0f+a)*omega);
                    anew = 0.5f*(a+tem1);
                    bnew = 0.5f*(b+tem2);
                    if (fabsf(anew-a) < 1.0e-5f && fabsf(bnew-b) < 1.0e-5f) {
                        finished=1;
                    }
                    a = anew;
                    b = bnew;
                    if (++sum > 500) {
                        finished = 1;
                    }
                    break;
                }
                while (finished == 0);

                thrust += DtDr*rstep;
                torque += DqDr*rstep;

                float V = PI*chord*(chord*TC)*rstep*density;
                float m = V*Y*LB_TO_SLUGS;
                _ixx += m*rad*rad;
            }

            float CT = thrust/(RHO*n2*D4);
            float CQ = torque/(RHO*n2*D5);
            float CP = PI*CQ;

            _performance_t entry(J, CT, CP);
            _performance.push_back(entry);
        }

        _pitch_levels++;
        if (_fixed_pitch) break;
        pitch += 15.0f;
    }
    while (_pitch_levels < NUM_PROP_PITCHES);

    // hub
    float r = 0.1f*R;
    float V = PI*r*r*max_thickness;
    float m = V*Y*LB_TO_SLUGS;
    _ixx += m*r*r;
}

void Propeller::set_thruster(float mrpm)
{
    // find rpm which gives a tip mach of 0.88 (static at sea level)
    _engine_rpm = mrpm;
    _max_rpm = 18763.0f / _diameter;
    _gear_ratio = _MAX(_engine_rpm / _max_rpm, 1.0f);

    float n = _max_rpm/60.0f;
    float n2 = n*n;
    float D = _diameter;
    float D4 = D*D*D*D;
    float D5 = D4*D;

    // power and thrust coefficients at design point
    // for fixed pitch design point is beta=22, J=0.2
    // for variable pitch design point is beta=15, j=0
    _Cp0 = _propulsion->_power * 550.0f / RHO / n2 / D5;
    if (_fixed_pitch == false)
    {
        _Ct0 = _Cp0 * 2.33f;
        _static_thrust = _Ct0 * RHO * n2 * D4;
    } else {
        _Ct0 = _Cp0 * 2.33f; // 1.4f;
        _static_thrust = _Ct0 * RHO * n2 * D4;
    }

    // estimate the number of blades
    if (_static_thrust < 100000.0f)
    {
        _blades = 2;
        if (_static_thrust <  50000.0f)
        {
            _density_factor = 1.0f;
            _specific_weight = 116.0f;	// wood
        }
        else
        {
            _density_factor = 0.2f;
            _specific_weight = 172.0f;	// aluminum
        }
    }
    else if (_static_thrust < 175000.0f)
    {
        _blades = 3;
        _density_factor = 0.2f;
        _specific_weight = 172.0f;	// aluminum
    }
    else if (_static_thrust < 200000.0f)
    {
        _blades = 4;			// aluminum
        _density_factor = 0.2f;
        _specific_weight = 172.0f;
    }
    else if (_static_thrust < 400000.0f)
    {
        _blades = 6;
        _density_factor = 0.2f;
        _specific_weight = 172.0f;	// aluminum
    } else {
        _blades = 8;
        _density_factor = 0.1f;
        _specific_weight = 100.0f;	// carbon fiber
    }

    // Thruster effects on coefficients
    Aeromatic* aircraft = _propulsion->_aircraft;
    float Swp = 0.96f*_diameter/aircraft->_wing.span;

    _dCLT0 = aircraft->_CL0*Swp;
    _dCLTmax = aircraft->_CLmax[0]*Swp;
    _dCLTalpha = aircraft->_CLaw[0]*Swp;

    _prop_span_left = 0;
    _prop_span_right = 0;
    int left = 0, right = 0;
    for (unsigned i = 0; i < aircraft->_no_engines; ++i)
    {
        if (_propulsion->_mount_point[i] == LEFT_WING)
        {
            left++;
            _prop_span_left += _propulsion->_thruster_loc[i][Y];
        }
        else if (_propulsion->_mount_point[i] == RIGHT_WING)
        {
            right++;
            _prop_span_right += _propulsion->_thruster_loc[i][Y];
        }
    }
    if (aircraft->_no_engines > 1)
    {
       _prop_span_left /= left;
       _prop_span_right /= right;
    }

    bladeElement();

    _max_thrust = _fixed_pitch ? _performance[0].CT
                          : _performance[_performance.size()/_pitch_levels].CT;
    _max_thrust *= RHO * n2 * D4;
    _max_torque = -RHO * _ixx*(2.0f*PI*_max_rpm);
}

std::string Propeller::lift()
{
    std::stringstream file;

    Aeromatic* aircraft = _propulsion->_aircraft;
    float dCL0 = _dCLT0;
    float dCLmax = _dCLTmax;
    float dCLalpha = _dCLTalpha;

    float alpha = (dCLmax-dCL0)/dCLalpha;

    file << std::setprecision(3) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Lift_propwash\">" << std::endl;
    file << "      <description>Delta lift due to propeller induced velocity</description>" << std::endl;
    file << "      <product>" << std::endl;
    if (aircraft->_no_engines > 1) {
        file << "         <property>systems/propulsion/thrust-coefficient</property>" << std::endl;
    } else {
        file << "         <property>propulsion/engine[0]/thrust-coefficient</property>" << std::endl;
    }
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
    file << "            <independentVar lookup=\"column\">fcs/flap-pos-deg</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "                     0.0     60.0" << std::endl;
    file << "              " << std::setprecision(2) << std::setw(5) << _MIN(-dCL0/alpha, -0.01f) << "  0.000   0.000" << std::endl;
    file << "               0.00  " << std::setprecision(3) << std::setw(5) << (dCL0) << std::setw(8) << (FACT * dCL0) << std::endl;
    file << "               " << std::setprecision(2) << (alpha) << std::setprecision(3) << std::setw(7) << (dCLmax) << std::setw(8) << (FACT * dCLmax) << std::endl;
    file << "               " << std::setprecision(2) << (2.0f*alpha) << "  0.000   0.000" << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "      </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string Propeller::pitch()
{
    std::stringstream file;

    Aeromatic* aircraft = _propulsion->_aircraft;
    float Sw = aircraft->_wing.area;
    float lh = aircraft->_htail.arm;
    float Sh = aircraft->_htail.area;
    float cbarw = aircraft->_wing.chord_mean;

    float Knp = aircraft->_no_engines;
    if (Knp > 3.0f) Knp = 2.0f;
    Knp /= aircraft->_no_engines;

    float pfact = Knp*lh*Sh/cbarw/Sw;
    if (aircraft->_cg_loc[X] > aircraft->_aero_rp[X]) pfact *= -1.0f;

    float Cm0 = _dCLT0*pfact;
    float Cmmax = _dCLTmax*pfact;
    float Cmalpha = _dCLTalpha*pfact;

    float alpha = (Cmmax-Cm0)/Cmalpha;

    file << std::setprecision(3) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/moment/Pitch_propwash\">" << std::endl;
    file << "      <description>Pitch moment due to propeller induced velocity</description>" << std::endl;
    file << "      <product>" << std::endl;
    if (aircraft->_no_engines > 1) {
        file << "         <property>systems/propulsion/thrust-coefficient</property>" << std::endl;
    } else {
        file << "          <property>propulsion/engine[0]/thrust-coefficient</property>" << std::endl;
    }
    file << "          <property>aero/qbar-psf</property>" << std::endl;
    file << "          <property>metrics/Sw-sqft</property>" << std::endl;
    file << "          <property>metrics/bw-ft</property>" << std::endl;
    file << "          <table>" << std::endl;
    file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
    file << "            <independentVar lookup=\"column\">fcs/flap-pos-deg</independentVar>" << std::endl;
    file << "            <tableData>" << std::endl;
    file << "                     0.0     60.0" << std::endl;
    file << "              " << std::setprecision(2) << std::setw(5) << _MIN(Cm0*alpha, -0.01f) << "  0.000   0.000" << std::endl;
    file << "               0.00 " << std::setprecision(3) << std::setw(6) << (Cm0) << std::setw(8) << (FACT * Cm0) << std::endl;
    file << "               " << std::setprecision(2) << (alpha) << std::setprecision(3) << std::setw(7) << (Cmmax) << std::setw(8) << (FACT * Cmmax) << std::endl;
    file << "               " << std::setprecision(2) << (1.3f*alpha) << "  0.000   0.000" << std::endl;
    file << "            </tableData>" << std::endl;
    file << "          </table>" << std::endl;
    file << "      </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string Propeller::roll()
{
    std::stringstream file;

    Aeromatic* aircraft = _propulsion->_aircraft;
    float y = _prop_span_left - _diameter/2.0f;
    float k = y/(aircraft->_wing.span/2.0f);
//  float TR = aircraft->_wing.taper;

    // http://www.princeton.edu/~stengel/MAE331Lecture5.pdf
    float dClT = (_dCLTalpha/2.0f)*((1.0f-k*k)/3.0);

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/moment/Roll_differential_propwash\">" << std::endl;
    file << "       <description>Roll moment due to differential propwash</description>" << std::endl;
    file << "       <product>" << std::endl;
    if (aircraft->_no_engines > 1) {
        file << "           <property>systems/propulsion/thrust-coefficient-left-right</property>" << std::endl;
    } else {
        file << "           <property>propulsion/engine[0]/thrust-coefficient</property>" << std::endl;
    }
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/bw-ft</property>" << std::endl;
    file << "           <property>aero/alpha-rad</property>" << std::endl;
    file << "           <value> " << (dClT) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string Propeller::thruster()
{
//  PistonEngine *engine = (PistonEngine*)_engine;
    bool& convert = _propulsion->_aircraft->_metric;
    std::stringstream file;

    file << "<!-- Generated by Aero-Matic v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "    See: http://wiki.flightgear.org/JSBSim_Thrusters#FGPropeller" << std::endl;
    file << std::endl;
    file << "    Inputs:" << std::endl;
    file << "           horsepower: " << _propulsion->_power << std::endl;
    file << "       max engine rpm: " << _engine_rpm << std::endl;
    file << "        prop diameter: " << _diameter << " " << _inputs["propellerDiameter"]->get_unit(false, LENGTH, convert) << std::endl;
    file << "           prop chord: " << _max_chord << " " << _inputs["propellerDiameter"]->get_unit(false, LENGTH, convert) << std::endl;
    file << "                pitch: " << (_fixed_pitch ? "fixed" : "variable") << " at " << _pitch << " degrees" << std::endl;
    file << std::endl;
    file << "    Outputs:" << std::endl;
    file << "         max prop rpm: " << _max_rpm << std::endl;
    file << "           gear ratio: " << _gear_ratio << std::endl;
    file << "                  Cp0: " << _Cp0 << std::endl;
    file << "                  Ct0: " << _Ct0 << std::endl;
    file << "  static thrust (lbs): " << std::fixed << std::setprecision(1) << _static_thrust << std::endl;
    file << "    max. thrust (lbs): " << std::fixed <<  std::setprecision(1) << _max_thrust << std::endl;
    file << "-->" << std::endl;
    file << std::endl;

    file << "<propeller version=\"1.01\" name=\"prop\">" << std::endl;
    file << "  <ixx> " << _ixx * _blades << " </ixx>" << std::endl;
    file << "  <diameter unit=\"" << Param::get_unit(true, LENGTH, convert) << "\"> " << Param::get(_diameter, LENGTH, convert) << " </diameter>" << std::endl;
    file << "  <numblades> " << _blades << " </numblades>" << std::endl;
    file << "  <gearratio> " << _gear_ratio << " </gearratio>" << std::endl;
//  file << "  <cp_factor> 1.00 </cp_factor>" << std::endl;
//  file << "  <ct_factor> " << _blades << " </ct_factor> <!-- set to match the number of blades -->" << std::endl;

    if(_fixed_pitch == false)
    {
        file << "  <minpitch> 12 </minpitch>" << std::endl;
        file << "  <maxpitch> 45 </maxpitch>" << std::endl;
        file << "  <minrpm> " << (_max_rpm * 0.85f) << " </minrpm>" << std::endl;
        file << "  <maxrpm> " << _max_rpm << " </maxrpm>" << std::endl;
    }
    file << std::endl;

    file << std::fixed;
    if(_fixed_pitch)
    {
        file << "  <table name=\"C_THRUST\" type=\"internal\">" << std::endl;
        file << "     <tableData>" << std::endl;
        for (unsigned i=0; i<_performance.size(); ++i)
        {
            file << std::setw(10) << std::setprecision(2) << _performance[i].J;
            file << std::setw(10) << std::setprecision(4) << _performance[i].CT;
            file << std::endl;
        }
        file << "     </tableData>" << std::endl;
        file << "  </table>" << std::endl;
        file << std::endl;
    }
    else	// variable pitch
    {
        file << " <!-- thrust coefficient as a function of advance ratio and blade angle -->" << std::endl;
        file << "  <table name=\"C_THRUST\" type=\"internal\">" << std::endl;
        file << "      <tableData>" << std::endl;
        unsigned size = _performance.size()/_pitch_levels;
        file << std::setw(16) << "";
        for (unsigned p=0; p<NUM_PROP_PITCHES; ++p) {
            file << std::setw(10) << -15+int(p*15);
        }

        file << std::endl;
        for (unsigned i=0; i<size; ++i)
        {
            file << std::setw(16) << std::setprecision(2) << _performance[i].J;
            file << std::setprecision(4);
            for (unsigned p=0; p<NUM_PROP_PITCHES; ++p) {
                file << std::setw(10) << _performance[(p*size)+i].CT;
            }
            file << std::endl;
        }
        file << "      </tableData>" << std::endl;
        file << "  </table>" << std::endl;
    }

    file << "" << std::endl;
    if(_fixed_pitch)
    {
        file << "  <table name=\"C_POWER\" type=\"internal\">" << std::endl;
        file << "     <tableData>" << std::endl;
        for (unsigned i=0; i<_performance.size(); ++i)
        {
            file << std::setw(10) << std::setprecision(2) << _performance[i].J;
            file << std::setw(10) << std::setprecision(4) << _performance[i].CP;
            file << std::endl;
        }
        file << "     </tableData>" << std::endl;
        file << "  </table>" << std::endl;
    }
    else
    {		// variable pitch
        file << " <!-- power coefficient as a function of advance ratio and blade angle -->" << std::endl;
        file << "  <table name=\"C_POWER\" type=\"internal\">" << std::endl;
        file << "     <tableData>" << std::endl;
        unsigned size = _performance.size()/_pitch_levels;
        file << std::setw(16) << "";
        for (unsigned p=0; p<NUM_PROP_PITCHES; ++p) {
            file << std::setw(10) << -15+int(p*15);
        }

        file << std::endl;
        for (unsigned i=0; i<size; ++i)
        {
            file << std::setw(16) << std::setprecision(2) << _performance[i].J;
            file << std::setprecision(4);
            for (unsigned p=0; p<NUM_PROP_PITCHES; ++p) {
                file << std::setw(10) << _performance[(p*size)+i].CP;
            }
            file << std::endl;
        }
        file << "     </tableData>" << std::endl;
        file << "  </table>" << std::endl;
    }

    file << std::endl;
    file << "<!-- thrust effects of helical tip Mach -->" << std::endl;
    file << "<table name=\"CT_MACH\" type=\"internal\">" << std::endl;
    file << "  <tableData>" << std::endl;
    file << "    0.85   1.0" << std::endl;
    file << "    1.05   0.8" << std::endl;
    file << "  </tableData>" << std::endl;
    file << "</table>" << std::endl;

    file << "" << std::endl;
    file << "<!-- power-required effects of helical tip Mach -->" << std::endl;
    file << "<table name=\"CP_MACH\" type=\"internal\">" << std::endl;
    file << "  <tableData>" << std::endl;
    file << "    0.85   1.0" << std::endl;
    file << "    1.05   1.8" << std::endl;
    file << "    2.00   1.4" << std::endl;
    file << "  </tableData>" << std::endl;
    file << "</table>" << std::endl;

    file << "\n</propeller>" << std::endl;

    return file.str();
}

std::string Propeller::json()
{
    std::stringstream file;

    file.precision(1);
    file.flags(std::ios::left);
    file << std::fixed << std::showpoint;

    std::string param  = "    \"FT_max\"";
    file << std::setw(14) << param << ": " << _max_thrust << "," << std::endl;

    param  = "    \"MT_max\"";
    file << std::setw(14) << param << ": " << _max_torque << "," << std::endl;

    param  = "    \"rpm_max\"";
    file << std::setw(14) << param << ": " << _max_rpm;

    return file.str();
}

// ---------------------------------------------------------------------------

float const Propeller::_CL_t[180] = {
#if 0
    0.0, 0.11, 0.22, 0.33, 0.44, 0.55, 0.66, 0.7483, 0.8442, 0.9260, 0.9937,
    1.0363, 1.0508, 1.0302, 0.9801, 0.9119, 0.8401, 0.77, 0.7305, 0.7041,
    0.699, 0.7097, 0.7298, 0.7593, 0.7961, 0.8353, 0.8838, 0.9473, 0.855,
    0.98, 1.035, 1.05, 1.02, 0.955, 0.875, 0.76, 0.63, 0.5, 0.365, 0.23, 0.09,
    -0.05, -0.185, -0.32, -0.45, -0.575, -0.67, -0.76, -0.85, -0.93, -0.98,
    -0.9, -0.77, -0.67, -0.635, -0.68, -0.85, -0.66, 0.0
#else
0.2500f,
0.3800f,
0.5100f,
0.6400f,
0.7700f,
0.9000f,
1.0000f,
1.1000f,
1.2000f,
1.3000f,
1.4000f,
1.4600f,
1.3500f,
1.2800f,
1.0000f,
0.8000f,
0.7900f,
0.7800f,
0.7700f,
0.7600f,
0.7500f,
0.7835f,
0.8085f,
0.8320f,
0.8540f,
0.8744f,
0.8933f,
0.9106f,
0.9264f,
0.9406f,
0.9532f,
0.9643f,
0.9739f,
0.9819f,
0.9884f,
0.9935f,
0.9971f,
0.9992f,
1.0000f,
0.9994f,
0.9974f,
0.9942f,
0.9897f,
0.9839f,
0.9770f,
0.9689f,
0.9597f,
0.9495f,
0.9382f,
0.9260f,
0.9128f,
0.8987f,
0.8838f,
0.8680f,
0.8515f,
0.8343f,
0.8163f,
0.7977f,
0.7785f,
0.7587f,
0.7384f,
0.7176f,
0.6962f,
0.6745f,
0.6523f,
0.6297f,
0.6068f,
0.5835f,
0.5599f,
0.5361f,
0.5119f,
0.4876f,
0.4630f,
0.4382f,
0.4133f,
0.3881f,
0.3628f,
0.3374f,
0.3119f,
0.2863f,
0.2605f,
0.2347f,
0.2088f,
0.1828f,
0.1568f,
0.1307f,
0.1046f,
0.0785f,
0.0524f,
0.0262f,
0.0000f,
-0.0262f,
-0.0524f,
-0.0785f,
-0.1046f,
-0.1307f,
-0.1568f,
-0.1828f,
-0.2088f,
-0.2347f,
-0.2605f,
-0.2863f,
-0.3119f,
-0.3374f,
-0.3628f,
-0.3881f,
-0.4133f,
-0.4382f,
-0.4630f,
-0.4876f,
-0.5119f,
-0.5361f,
-0.5599f,
-0.5835f,
-0.6068f,
-0.6297f,
-0.6523f,
-0.6745f,
-0.6962f,
-0.7176f,
-0.7384f,
-0.7587f,
-0.7785f,
-0.7977f,
-0.8163f,
-0.8343f,
-0.8515f,
-0.8680f,
-0.8838f,
-0.8987f,
-0.9128f,
-0.9260f,
-0.9382f,
-0.9495f,
-0.9597f,
-0.9689f,
-0.9770f,
-0.9839f,
-0.9897f,
-0.9942f,
-0.9974f,
-0.9994f,
-1.0000f,
-0.9992f,
-0.9971f,
-0.9935f,
-0.9884f,
-0.9819f,
-0.9739f,
-0.9643f,
-0.9532f,
-0.9406f,
-0.9264f,
-0.9106f,
-0.8933f,
-0.8744f,
-0.8540f,
-0.8320f,
-0.8085f,
-0.7835f,
-0.7571f,
-0.7292f,
-0.6999f,
-0.6693f,
-0.6373f,
-0.6041f,
-0.5696f,
-0.7460f,
-0.8526f,
-1.0003f,
-0.9790f,
-0.9185f,
-0.8588f,
-0.7999f,
-0.7415f,
-0.6838f,
-0.5965f,
-0.5095f,
-0.4229f,
-0.3364f

#endif
};

float const Propeller::_CD_t[180] = {
#if 0
    0.0077, 0.0078, 0.008, 0.0083, 0.0089, 0.0098, 0.0108, 0.0122, 0.0135,
    0.0149, 0.0164, 0.0182, 0.02, 0.0221, 0.0244, 0.0269, 0.0297, 0.134,
    0.238, 0.26, 0.282, 0.305, 0.329, 0.354, 0.379, 0.405, 0.432, 0.46,0.57,
    0.745, 0.92, 1.075, 1.215, 1.345, 1.47, 1.575, 1.665, 1.735, 1.78, 1.8,
    1.8, 1.78, 1.75, 1.7, 1.635, 1.555, 1.465, 1.35, 1.225, 1.085, 0.925,
    0.755, 0.575, 0.42, 0.32, 0.23, 0.14, 0.055, 0.25
#else
0.0000f,
0.0021f,
0.0042f,
0.0063f,
0.0084f,
0.0105f,
0.0125f,
0.0146f,
0.0167f,
0.0188f,
0.0209f,
0.0283f,
0.0356f,
0.0430f,
0.0503f,
0.0577f,
0.1371f,
0.2164f,
0.2366f,
0.2569f,
0.2771f,
0.2973f,
0.3176f,
0.3378f,
0.3737f,
0.4097f,
0.4456f,
0.4815f,
0.5175f,
0.5534f,
0.5893f,
0.6252f,
0.6612f,
0.6971f,
0.7292f,
0.7614f,
0.7935f,
0.8257f,
0.8578f,
0.8900f,
0.9221f,
0.9542f,
0.9864f,
1.0185f,
1.0507f,
1.0828f,
1.1166f,
1.1504f,
1.1843f,
1.2181f,
1.2519f,
1.2857f,
1.3195f,
1.3534f,
1.3872f,
1.4210f,
1.4368f,
1.4527f,
1.4685f,
1.4843f,
1.5002f,
1.5160f,
1.5318f,
1.5477f,
1.5635f,
1.5793f,
1.5952f,
1.6110f,
1.6268f,
1.6427f,
1.6585f,
1.6727f,
1.6870f,
1.7012f,
1.7155f,
1.7297f,
1.7440f,
1.7582f,
1.7725f,
1.7867f,
1.8010f,
1.8047f,
1.8083f,
1.8120f,
1.8157f,
1.8193f,
1.8230f,
1.8267f,
1.8304f,
1.8340f,
1.8377f,
1.8297f,
1.8218f,
1.8138f,
1.8059f,
1.7979f,
1.7899f,
1.7820f,
1.7740f,
1.7661f,
1.7581f,
1.7459f,
1.7337f,
1.7215f,
1.7093f,
1.6971f,
1.6850f,
1.6728f,
1.6606f,
1.6484f,
1.6362f,
1.6229f,
1.6097f,
1.5964f,
1.5832f,
1.5699f,
1.5567f,
1.5434f,
1.5302f,
1.5169f,
1.5037f,
1.4793f,
1.4550f,
1.4306f,
1.4063f,
1.3819f,
1.3575f,
1.3332f,
1.3088f,
1.2845f,
1.2601f,
1.2283f,
1.1966f,
1.1648f,
1.1331f,
1.1013f,
1.0695f,
1.0378f,
1.0060f,
0.9743f,
0.9425f,
0.9086f,
0.8748f,
0.8409f,
0.8070f,
0.7731f,
0.7393f,
0.7054f,
0.6715f,
0.6377f,
0.6038f,
0.5747f,
0.5456f,
0.5164f,
0.4873f,
0.4582f,
0.4291f,
0.4000f,
0.3708f,
0.3417f,
0.3126f,
0.2946f,
0.2766f,
0.2586f,
0.2406f,
0.2225f,
0.2045f,
0.1865f,
0.1685f,
0.1505f,
0.1325f,
0.1192f,
0.1060f,
0.0927f,
0.0795f,
0.0662f,
0.0530f,
0.0397f,
0.0265f,
0.0132f
#endif
};

#if 0
float const Propeller::_thrust_t[23][9] =
{
    {-0.488f, 0.275f, 1.000f, 1.225f, 1.350f, 1.425f, 1.313f, 1.125f, 0.0f },
    {-0.725f, 0.000f, 1.000f, 1.225f, 1.350f, 1.438f, 1.344f, 1.125f, 0.0f },
    {-0.813f,-0.250f, 0.863f, 1.200f, 1.331f, 1.438f, 1.344f, 1.125f, 0.0f },
    {-0.813f,-0.581f, 0.650f, 1.188f, 1.306f, 1.425f, 1.344f, 1.125f, 0.0f },
    {-0.813f,-0.813f, 0.344f, 1.069f, 1.250f, 1.388f, 1.325f, 1.125f, 0.0f },
    {-0.813f,-0.813f, 0.019f, 0.800f, 1.213f, 1.338f, 1.325f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.325f, 0.488f, 1.163f, 1.269f, 1.313f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.669f, 0.150f, 0.956f, 1.225f, 1.313f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.219f, 0.688f, 1.206f, 1.288f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.556f, 0.375f, 1.163f, 1.263f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f, 0.063f, 1.000f, 1.225f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.250f, 0.781f, 1.220f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.563f, 0.563f, 1.200f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f, 0.300f, 0.980f, 1.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f, 0.038f, 0.620f, 1.000f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.225f, 0.406f, 0.813f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.488f, 0.213f, 0.625f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.750f, 0.019f, 0.438f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.175f, 0.250f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.369f, 0.063f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.563f,-0.125f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.756f,-0.313f, 0.0f },
    {-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f,-0.813f, 0.0f }
};

float const Propeller::_power_t[23][9] =
{
    { 0.167f, 0.333f, 1.167f, 2.650f, 4.570f, 6.500f, 7.500f, 8.300f, 8.300f },
    { 0.667f, 0.167f, 1.000f, 2.470f, 4.370f, 6.500f, 7.530f, 8.300f, 8.300f },
    { 0.950f, 0.267f, 0.967f, 2.300f, 4.180f, 6.500f, 7.530f, 8.300f, 8.300f },
    { 1.280f, 0.583f, 0.833f, 2.120f, 3.970f, 6.500f, 7.530f, 8.300f, 8.300f },
    { 1.570f, 0.883f, 0.550f, 1.970f, 3.720f, 6.370f, 7.500f, 8.300f, 8.300f },
    { 1.850f, 1.183f, 0.167f, 1.670f, 3.500f, 6.080f, 7.500f, 8.300f, 8.300f },
    { 2.130f, 1.470f, 0.167f, 1.170f, 3.300f, 5.770f, 7.470f, 8.300f, 8.300f },
    { 2.420f, 1.175f,-0.550f, 0.450f, 2.920f, 5.530f, 7.420f, 8.300f, 8.300f },
    { 2.700f, 2.030f,-0.830f,-0.333f, 2.250f, 5.450f, 7.330f, 8.300f, 8.300f },
    { 2.980f, 2.320f,-0.970f,-1.000f, 1.420f, 5.300f, 7.170f, 8.000f, 8.300f },
    { 3.270f, 2.600f,-1.000f,-1.670f, 0.417f, 4.700f, 6.950f, 7.830f, 8.300f },
    { 3.550f, 2.880f,-1.280f,-2.330f,-0.500f, 4.000f, 6.620f, 7.670f, 8.300f },
    { 3.830f, 3.170f,-1.570f,-3.000f,-1.500f, 3.250f, 6.420f, 7.330f, 8.300f },
    { 4.120f, 3.450f,-1.850f,-3.670f,-2.500f, 2.320f, 6.230f, 7.170f, 8.300f },
    { 4.400f, 3.730f,-2.130f,-4.330f,-3.170f, 0.970f, 6.080f, 6.920f, 8.300f },
    { 4.680f, 4.020f,-2.420f,-5.000f,-3.800f,-0.330f, 5.950f, 6.830f, 8.300f },
    { 4.970f, 4.300f,-2.700f,-5.670f,-4.500f,-1.500f, 5.750f, 6.830f, 8.300f },
    { 5.250f, 4.580f,-2.980f,-6.330f,-5.170f,-2.670f, 5.380f, 6.670f, 8.300f },
    { 5.530f, 4.870f,-3.270f,-7.000f,-5.830f,-3.830f, 4.170f, 6.500f, 8.300f },
    { 5.820f, 5.150f,-3.550f,-7.670f,-6.500f,-5.000f, 2.930f, 6.330f, 8.300f },
    { 6.100f, 5.430f,-3.830f,-8.300f,-7.170f,-6.170f, 1.630f, 6.130f, 8.300f },
    { 6.380f, 5.720f,-4.120f,-8.300f,-8.300f,-7.330f, 0.330f, 5.670f, 8.300f },
    { 8.300f, 8.300f,-8.300f,-8.300f,-8.300f,-8.300f,-8.300f,-5.000f, 8.300f }
};
#endif

} /* namespace Aeromatic */

