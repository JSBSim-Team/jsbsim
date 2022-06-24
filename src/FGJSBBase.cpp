/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGJSBBase.cpp
 Author:       Jon S. Berndt
 Date started: 07/01/01
 Purpose:      Encapsulates the JSBBase object

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
07/01/01  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define BASE

#include "FGJSBBase.h"
#include "models/FGAtmosphere.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
const string FGJSBBase::needed_cfg_version = "2.0";
const string FGJSBBase::JSBSim_version = JSBSIM_VERSION " " __DATE__ " " __TIME__ ;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const Message& msg)
{
  _gdata.Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text)
{
  Message msg;
  msg.text = text;
  msg.messageId = _gdata.messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eText;
  _gdata.Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text, bool bVal)
{
  Message msg;
  msg.text = text;
  msg.messageId = _gdata.messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eBool;
  msg.bVal = bVal;
  _gdata.Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text, int iVal)
{
  Message msg;
  msg.text = text;
  msg.messageId = _gdata.messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eInteger;
  msg.iVal = iVal;
  _gdata.Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text, double dVal)
{
  Message msg;
  msg.text = text;
  msg.messageId = _gdata.messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eDouble;
  msg.dVal = dVal;
  _gdata.Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::ProcessMessage(void)
{
  if (_gdata.Messages.empty()) return;
  _gdata.localMsg = _gdata.Messages.front();

  while (SomeMessages()) {
      switch (_gdata.localMsg.type) {
      case JSBSim::Message::eText:
        cout << _gdata.localMsg.messageId << ": " << _gdata.localMsg.text << endl;
        break;
      case JSBSim::Message::eBool:
        cout << _gdata.localMsg.messageId << ": " << _gdata.localMsg.text << " " << _gdata.localMsg.bVal << endl;
        break;
      case JSBSim::Message::eInteger:
        cout << _gdata.localMsg.messageId << ": " << _gdata.localMsg.text << " " << _gdata.localMsg.iVal << endl;
        break;
      case JSBSim::Message::eDouble:
        cout << _gdata.localMsg.messageId << ": " << _gdata.localMsg.text << " " << _gdata.localMsg.dVal << endl;
        break;
      default:
        cerr << "Unrecognized message type." << endl;
        break;
      }
     _gdata.Messages.pop();
      if (SomeMessages()) _gdata.localMsg = _gdata.Messages.front();
      else break;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Message* FGJSBBase::ProcessNextMessage(void)
{
  if (_gdata.Messages.empty()) return NULL;
  _gdata.localMsg = _gdata.Messages.front();

  _gdata.Messages.pop();
  return &_gdata.localMsg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::disableHighLighting(void)
{
  _gdata.highint[0]='\0';
  _gdata.halfint[0]='\0';
  _gdata.normint[0]='\0';
  _gdata.reset[0]='\0';
  _gdata.underon[0]='\0';
  _gdata.underoff[0]='\0';
  _gdata.fgblue[0]='\0';
  _gdata.fgcyan[0]='\0';
  _gdata.fgred[0]='\0';
  _gdata.fggreen[0]='\0';
  _gdata.fgdef[0]='\0';
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGJSBBase::CreateIndexedPropertyName(const string& Property, int index)
{
  ostringstream buf;
  buf << Property << '[' << index << ']';
  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::GaussianRandomNumber(void)
{
  double X;

  if (_gdata.gaussian_random_number_phase == 0) {
    _gdata.V1 = _gdata.V2 = _gdata.S = X = 0.0;

    do {
      double U1 = (double)rand() / RAND_MAX;
      double U2 = (double)rand() / RAND_MAX;

      _gdata.V1 = 2 * U1 - 1;
      _gdata.V2 = 2 * U2 - 1;
      _gdata.S = _gdata.V1 * _gdata.V1 + _gdata.V2 * _gdata.V2;
    } while(_gdata.S >= 1 || _gdata.S == 0);

    X = _gdata.V1 * sqrt(-2 * log(_gdata.S) / _gdata.S);
  } else
    X = _gdata.V2 * sqrt(-2 * log(_gdata.S) / _gdata.S);

  _gdata.gaussian_random_number_phase = 1 - _gdata.gaussian_random_number_phase;

  return X;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::PitotTotalPressure(double mach, double p)
{
  if (mach < 0) return p;
  if (mach < 1)    //calculate total pressure assuming isentropic flow
    return p*pow((1 + 0.2*mach*mach),3.5);
  else {
    // shock in front of pitot tube, we'll assume its normal and use
    // the Rayleigh Pitot Tube Formula, i.e. the ratio of total
    // pressure behind the shock to the static pressure in front of
    // the normal shock assumption should not be a bad one -- most supersonic
    // aircraft place the pitot probe out front so that it is the forward
    // most point on the aircraft.  The real shock would, of course, take
    // on something like the shape of a rounded-off cone but, here again,
    // the assumption should be good since the opening of the pitot probe
    // is very small and, therefore, the effects of the shock curvature
    // should be small as well. AFAIK, this approach is fairly well accepted
    // within the aerospace community

    // The denominator below is zero for Mach ~ 0.38, for which
    // we'll never be here, so we're safe

    return p*166.92158009316827*pow(mach,7.0)/pow(7*mach*mach-1,2.5);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Based on the formulas in the US Air Force Aircraft Performance Flight Testing 
// Manual (AFFTC-TIH-99-01). In particular sections 4.6 to 4.8.

double FGJSBBase::MachFromImpactPressure(double qc, double p)
{
  double A = qc / p + 1;
  double M = sqrt(5.0*(pow(A, 1. / 3.5) - 1));  // Equation (4.12)

  if (M > 1.0)
    for (unsigned int i = 0; i<10; i++)
      M = 0.8812848543473311*sqrt(A*pow(1 - 1.0 / (7.0*M*M), 2.5));  // Equation (4.17)

  return M;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::VcalibratedFromMach(double mach, double p)
{
  double asl = FGAtmosphere::StdDaySLsoundspeed;
  double psl = FGAtmosphere::StdDaySLpressure;
  double qc = PitotTotalPressure(mach, p) - p;

  return asl * MachFromImpactPressure(qc, psl);  
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::MachFromVcalibrated(double vcas, double p)
{
  double asl = FGAtmosphere::StdDaySLsoundspeed;
  double psl = FGAtmosphere::StdDaySLpressure;
  double qc = PitotTotalPressure(vcas / asl, psl) - psl;

  return MachFromImpactPressure(qc, p);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim

