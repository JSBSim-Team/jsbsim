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

#ifndef _MSC_VER
    char FGJSBBase::highint[5]  = {27, '[', '1', 'm', '\0'      };
    char FGJSBBase::halfint[5]  = {27, '[', '2', 'm', '\0'      };
    char FGJSBBase::normint[6]  = {27, '[', '2', '2', 'm', '\0' };
    char FGJSBBase::reset[5]    = {27, '[', '0', 'm', '\0'      };
    char FGJSBBase::underon[5]  = {27, '[', '4', 'm', '\0'      };
    char FGJSBBase::underoff[6] = {27, '[', '2', '4', 'm', '\0' };
    char FGJSBBase::fgblue[6]   = {27, '[', '3', '4', 'm', '\0' };
    char FGJSBBase::fgcyan[6]   = {27, '[', '3', '6', 'm', '\0' };
    char FGJSBBase::fgred[6]    = {27, '[', '3', '1', 'm', '\0' };
    char FGJSBBase::fggreen[6]  = {27, '[', '3', '2', 'm', '\0' };
    char FGJSBBase::fgdef[6]    = {27, '[', '3', '9', 'm', '\0' };
#else
    char FGJSBBase::highint[5]  = {'\0' };
    char FGJSBBase::halfint[5]  = {'\0' };
    char FGJSBBase::normint[6]  = {'\0' };
    char FGJSBBase::reset[5]    = {'\0' };
    char FGJSBBase::underon[5]  = {'\0' };
    char FGJSBBase::underoff[6] = {'\0' };
    char FGJSBBase::fgblue[6]   = {'\0' };
    char FGJSBBase::fgcyan[6]   = {'\0' };
    char FGJSBBase::fgred[6]    = {'\0' };
    char FGJSBBase::fggreen[6]  = {'\0' };
    char FGJSBBase::fgdef[6]    = {'\0' };
#endif

const string FGJSBBase::needed_cfg_version = "2.0";
const string FGJSBBase::JSBSim_version = JSBSIM_VERSION " " __DATE__ " " __TIME__ ;

queue <FGJSBBase::Message> FGJSBBase::Messages;
FGJSBBase::Message FGJSBBase::localMsg;
unsigned int FGJSBBase::messageId = 0;

int FGJSBBase::gaussian_random_number_phase = 0;

short FGJSBBase::debug_lvl  = 1;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const Message& msg)
{
  Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text)
{
  Message msg;
  msg.text = text;
  msg.messageId = messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eText;
  Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text, bool bVal)
{
  Message msg;
  msg.text = text;
  msg.messageId = messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eBool;
  msg.bVal = bVal;
  Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text, int iVal)
{
  Message msg;
  msg.text = text;
  msg.messageId = messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eInteger;
  msg.iVal = iVal;
  Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::PutMessage(const string& text, double dVal)
{
  Message msg;
  msg.text = text;
  msg.messageId = messageId++;
  msg.subsystem = "FDM";
  msg.type = Message::eDouble;
  msg.dVal = dVal;
  Messages.push(msg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::ProcessMessage(void)
{
  if (Messages.empty()) return;
  localMsg = Messages.front();

  while (SomeMessages()) {
      switch (localMsg.type) {
      case JSBSim::FGJSBBase::Message::eText:
        cout << localMsg.messageId << ": " << localMsg.text << endl;
        break;
      case JSBSim::FGJSBBase::Message::eBool:
        cout << localMsg.messageId << ": " << localMsg.text << " " << localMsg.bVal << endl;
        break;
      case JSBSim::FGJSBBase::Message::eInteger:
        cout << localMsg.messageId << ": " << localMsg.text << " " << localMsg.iVal << endl;
        break;
      case JSBSim::FGJSBBase::Message::eDouble:
        cout << localMsg.messageId << ": " << localMsg.text << " " << localMsg.dVal << endl;
        break;
      default:
        cerr << "Unrecognized message type." << endl;
        break;
      }
      Messages.pop();
      if (SomeMessages()) localMsg = Messages.front();
      else break;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGJSBBase::Message* FGJSBBase::ProcessNextMessage(void)
{
  if (Messages.empty()) return NULL;
  localMsg = Messages.front();

  Messages.pop();
  return &localMsg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::disableHighLighting(void)
{
  highint[0]='\0';
  halfint[0]='\0';
  normint[0]='\0';
  reset[0]='\0';
  underon[0]='\0';
  underoff[0]='\0';
  fgblue[0]='\0';
  fgcyan[0]='\0';
  fgred[0]='\0';
  fggreen[0]='\0';
  fgdef[0]='\0';
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
  static double V1, V2, S;
  double X;

  if (gaussian_random_number_phase == 0) {
    V1 = V2 = S = X = 0.0;

    do {
      double U1 = (double)rand() / RAND_MAX;
      double U2 = (double)rand() / RAND_MAX;

      V1 = 2 * U1 - 1;
      V2 = 2 * U2 - 1;
      S = V1 * V1 + V2 * V2;
    } while(S >= 1 || S == 0);

    X = V1 * sqrt(-2 * log(S) / S);
  } else
    X = V2 * sqrt(-2 * log(S) / S);

  gaussian_random_number_phase = 1 - gaussian_random_number_phase;

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

