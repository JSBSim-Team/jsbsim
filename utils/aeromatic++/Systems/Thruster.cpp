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
    std::vector<Param*>::iterator it;
    for(it = _inputs.begin(); it != _inputs.end(); ++it) {
        delete *it;
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


Nozzle::Nozzle(Propulsion *p) : Thruster(p),
    _diameter(3.25f)
{
    strCopy(_thruster_name, "my_nozzle");

    _inputs.push_back(new Param("Nozzle name", "The name is used for the configuration file name", _thruster_name));
    _inputs.push_back(new Param("Nozzle diameter", "Nozzle diameter influences the nozzle area and exit pressure", _diameter, _propulsion->_aircraft->_metric, LENGTH));
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
Propeller::Propeller(Propulsion *p) : Thruster(p),
    _fixed_pitch(true),
    _diameter(8.0f),
    _max_rpm(2100.0f),
    _max_chord(0.0f),
    _pitch(45.0f),
    _ixx(0.0f),
    _pitch_levels(0)
{
    strCopy(_thruster_name, "my_propeller");

    _inputs.push_back(new Param("Thruster name", "The name is used for the configuration file name", _thruster_name));
    _inputs.push_back(new Param("Propeller diameter", "Propeller diameter is critical for a good thrust estimation", _diameter, _propulsion->_aircraft->_metric, LENGTH));
    _inputs.push_back(new Param("Is the propeller fixed pitch?", "Fixed pitch propellers do not have any mechanics to alter the pitch angle", _fixed_pitch));
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
 */
void  Propeller::bladeElement()
{
    const float Y = 167.0f;	// Specific Weight of aluminum
    const float rho = 1.225f;
    const float Cf = 0.006f;	// skin Friction Coefficient
    const float k1 = 0.2f;	// correction factor for airfoil thickness

    float RPM = _engine_rpm;
    float D = _diameter;
    float B = _blades;
    float R = D/2.0f;

    if (_max_chord == 0) {
        _max_chord = 0.76f*sqrtf(R/B);
    }
    float max_thickness = 0.3f*_max_chord;
    float max_camber = 0.035f;

    float xt = R;
    float xs = R/NUM_ELEMENTS;
    float hub = atanf(_pitch/(2.0f*PI*xs*FEET_TO_INCH))*RAD_TO_DEG;
    float tip = atanf(_pitch/(2.0f*PI*xt*FEET_TO_INCH))*RAD_TO_DEG;

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
                float chord=_max_chord*(0.55f+0.7f*(pow(r,0.25f)-pow(r,5.0f)));
                float TC = max_thickness*(1.0f-0.99f*powf(r,0.05f))/chord;
                float CC = max_camber*(1.0f-0.99f*powf(x,0.05f))/chord;
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
                    DtDr = solidity*PI*rho*V0*V0/(sphi*sphi)*rad*CY;
                    DqDr = solidity*PI*rho*V0*V0/(sphi*sphi)*rad*rad*CX;

                    tem1 = DtDr/(4.0f*PI*rad*rho*V*V*(1.0f+a));
                    tem2 = DqDr/(4.0f*PI*rad*rad*rad*rho*V*(1.0f+a)*omega);
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

                float V = PI*chord*(chord*TC)*rstep;
                float m = B*V*Y*LB_TO_SLUGS;
                _ixx += m*rad*rad;
            }

            float CT = thrust/(rho*n2*D4);
            float CQ = torque/(rho*n2*D5);
            float CP = 2.0f*PI*CQ;

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

    float max_rps = _max_rpm / 60.0f;
    float rps2 = max_rps * max_rps;
    float rps3 = rps2 * max_rps;
    float d4 = _diameter * _diameter * _diameter * _diameter;
    float d5 = d4 * _diameter;
    float rho = 0.002378f;

    // power and thrust coefficients at design point
    // for fixed pitch design point is beta=22, J=0.2
    // for variable pitch design point is beta=15, j=0
    _Cp0 = _propulsion->_power * 550.0f / rho / rps3 / d5;
    if (_fixed_pitch == false)
    {
        _Ct0 = _Cp0 * 2.33f;
        _static_thrust = _Ct0 * rho * rps2 * d4;
    } else {
        float rpss = powf(_propulsion->_power * 550.0f / 1.025f / _Cp0 / rho / d5, 0.3333f);
        _Ct0 = _Cp0 * 1.4f;
        _static_thrust = 1.09f * _Ct0 * rho * rpss * rpss * d4;
    }

    // estimate the number of blades
printf("_Cp0: %f\n", _Cp0);
    if (_Cp0 < 0.035f) {
      _blades = 2;
    } else if (_Cp0 > 0.280f) {
        _blades = 8;
    } else if (_Cp0 > 0.140f) {
      _blades = 6;
    } else if (_Cp0 > 0.070f) {
      _blades = 4;
    } else {
      _blades = 3;
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

    float pfact = -Knp*lh*Sh/cbarw/Sw;

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
    std::stringstream file;

    file << "<!-- Generated by Aero-Matic v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "    See: http://wiki.flightgear.org/JSBSim_Thrusters#FGPropeller" << std::endl;
    file << std::endl;
    file << "    Inputs:" << std::endl;
    file << "           horsepower: " << _propulsion->_power << std::endl;
    file << "       max engine rpm: " << _engine_rpm << std::endl;
    file << "   prop diameter (ft): " << _diameter << std::endl;
    file << "      prop chord (ft): " << _max_chord << std::endl;
    file << "                pitch: " << (_fixed_pitch ? "fixed" : "variable") << " at " << _pitch << " inch" << std::endl;
    file << std::endl;
    file << "    Outputs:" << std::endl;
    file << "         max prop rpm: " << _max_rpm << std::endl;
    file << "           gear ratio: " << _gear_ratio << std::endl;
    file << "                  Cp0: " << _Cp0 << std::endl;
    file << "                  Ct0: " << _Ct0 << std::endl;
    file << "  static thrust (lbs): " << _static_thrust << std::endl;
    file << "-->" << std::endl;
    file << std::endl;

    file << "<propeller version=\"1.01\" name=\"prop\">" << std::endl;
    file << "  <ixx> " << _ixx * _blades << " </ixx>" << std::endl;
    file << "  <diameter unit=\"IN\"> " << (_diameter * FEET_TO_INCH) << " </diameter>" << std::endl;
    file << "  <numblades> " << _blades << " </numblades>" << std::endl;
    file << "  <gearratio> " << _gear_ratio << " </gearratio>" << std::endl;
    file << "  <cp_factor> 1.00 </cp_factor>" << std::endl;
    file << "  <ct_factor> " << _blades << " </ct_factor> <!-- set to match the number of blades -->" << std::endl;

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
    0.2500,
0.3800,
0.5100,
0.6400,
0.7700,
0.9000,
1.0000,
1.1000,
1.2000,
1.3000,
1.4000,
1.4600,
1.3500,
1.2800,
1.0000,
0.8000,
0.7900,
0.7800,
0.7700,
0.7600,
0.7500,
0.7835,
0.8085,
0.8320,
0.8540,
0.8744,
0.8933,
0.9106,
0.9264,
0.9406,
0.9532,
0.9643,
0.9739,
0.9819,
0.9884,
0.9935,
0.9971,
0.9992,
1.0000,
0.9994,
0.9974,
0.9942,
0.9897,
0.9839,
0.9770,
0.9689,
0.9597,
0.9495,
0.9382,
0.9260,
0.9128,
0.8987,
0.8838,
0.8680,
0.8515,
0.8343,
0.8163,
0.7977,
0.7785,
0.7587,
0.7384,
0.7176,
0.6962,
0.6745,
0.6523,
0.6297,
0.6068,
0.5835,
0.5599,
0.5361,
0.5119,
0.4876,
0.4630,
0.4382,
0.4133,
0.3881,
0.3628,
0.3374,
0.3119,
0.2863,
0.2605,
0.2347,
0.2088,
0.1828,
0.1568,
0.1307,
0.1046,
0.0785,
0.0524,
0.0262,
0.0000,
-0.0262,
-0.0524,
-0.0785,
-0.1046,
-0.1307,
-0.1568,
-0.1828,
-0.2088,
-0.2347,
-0.2605,
-0.2863,
-0.3119,
-0.3374,
-0.3628,
-0.3881,
-0.4133,
-0.4382,
-0.4630,
-0.4876,
-0.5119,
-0.5361,
-0.5599,
-0.5835,
-0.6068,
-0.6297,
-0.6523,
-0.6745,
-0.6962,
-0.7176,
-0.7384,
-0.7587,
-0.7785,
-0.7977,
-0.8163,
-0.8343,
-0.8515,
-0.8680,
-0.8838,
-0.8987,
-0.9128,
-0.9260,
-0.9382,
-0.9495,
-0.9597,
-0.9689,
-0.9770,
-0.9839,
-0.9897,
-0.9942,
-0.9974,
-0.9994,
-1.0000,
-0.9992,
-0.9971,
-0.9935,
-0.9884,
-0.9819,
-0.9739,
-0.9643,
-0.9532,
-0.9406,
-0.9264,
-0.9106,
-0.8933,
-0.8744,
-0.8540,
-0.8320,
-0.8085,
-0.7835,
-0.7571,
-0.7292,
-0.6999,
-0.6693,
-0.6373,
-0.6041,
-0.5696,
-0.7460,
-0.8526,
-1.0003,
-0.9790,
-0.9185,
-0.8588,
-0.7999,
-0.7415,
-0.6838,
-0.5965,
-0.5095,
-0.4229,
-0.3364

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
0.0000,
0.0021,
0.0042,
0.0063,
0.0084,
0.0105,
0.0125,
0.0146,
0.0167,
0.0188,
0.0209,
0.0283,
0.0356,
0.0430,
0.0503,
0.0577,
0.1371,
0.2164,
0.2366,
0.2569,
0.2771,
0.2973,
0.3176,
0.3378,
0.3737,
0.4097,
0.4456,
0.4815,
0.5175,
0.5534,
0.5893,
0.6252,
0.6612,
0.6971,
0.7292,
0.7614,
0.7935,
0.8257,
0.8578,
0.8900,
0.9221,
0.9542,
0.9864,
1.0185,
1.0507,
1.0828,
1.1166,
1.1504,
1.1843,
1.2181,
1.2519,
1.2857,
1.3195,
1.3534,
1.3872,
1.4210,
1.4368,
1.4527,
1.4685,
1.4843,
1.5002,
1.5160,
1.5318,
1.5477,
1.5635,
1.5793,
1.5952,
1.6110,
1.6268,
1.6427,
1.6585,
1.6727,
1.6870,
1.7012,
1.7155,
1.7297,
1.7440,
1.7582,
1.7725,
1.7867,
1.8010,
1.8047,
1.8083,
1.8120,
1.8157,
1.8193,
1.8230,
1.8267,
1.8304,
1.8340,
1.8377,
1.8297,
1.8218,
1.8138,
1.8059,
1.7979,
1.7899,
1.7820,
1.7740,
1.7661,
1.7581,
1.7459,
1.7337,
1.7215,
1.7093,
1.6971,
1.6850,
1.6728,
1.6606,
1.6484,
1.6362,
1.6229,
1.6097,
1.5964,
1.5832,
1.5699,
1.5567,
1.5434,
1.5302,
1.5169,
1.5037,
1.4793,
1.4550,
1.4306,
1.4063,
1.3819,
1.3575,
1.3332,
1.3088,
1.2845,
1.2601,
1.2283,
1.1966,
1.1648,
1.1331,
1.1013,
1.0695,
1.0378,
1.0060,
0.9743,
0.9425,
0.9086,
0.8748,
0.8409,
0.8070,
0.7731,
0.7393,
0.7054,
0.6715,
0.6377,
0.6038,
0.5747,
0.5456,
0.5164,
0.4873,
0.4582,
0.4291,
0.4000,
0.3708,
0.3417,
0.3126,
0.2946,
0.2766,
0.2586,
0.2406,
0.2225,
0.2045,
0.1865,
0.1685,
0.1505,
0.1325,
0.1192,
0.1060,
0.0927,
0.0795,
0.0662,
0.0530,
0.0397,
0.0265,
0.0132
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

