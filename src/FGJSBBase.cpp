/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGJSBBase.cpp
 Author:       Jon S. Berndt
 Date started: 07/01/01
 Purpose:      Encapsulates the JSBBase object

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

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
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace JSBSim {

static const char *IdSrc = "$Id: FGJSBBase.cpp,v 1.35 2012/03/25 11:05:36 bcoconni Exp $";
static const char *IdHdr = ID_JSBBASE;

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

const double FGJSBBase::radtodeg = 57.295779513082320876798154814105;
const double FGJSBBase::degtorad = 0.017453292519943295769236907684886;
const double FGJSBBase::hptoftlbssec = 550.0;
const double FGJSBBase::psftoinhg = 0.014138;
const double FGJSBBase::psftopa = 47.88;
const double FGJSBBase::fpstokts = 0.592484;
const double FGJSBBase::ktstofps = 1.68781;
const double FGJSBBase::inchtoft = 0.08333333;
const double FGJSBBase::in3tom3 = 1.638706E-5;
const double FGJSBBase::m3toft3 = 1.0/(fttom*fttom*fttom);
const double FGJSBBase::inhgtopa = 3386.38;
const double FGJSBBase::fttom = 0.3048;
double FGJSBBase::Reng = 1716.56;   // Gas constant for Air (ft-lb/slug-R)
double FGJSBBase::Rstar = 1545.348; // Universal gas constant
double FGJSBBase::Mair = 28.9645;   //
const double FGJSBBase::SHRatio = 1.40;

// Note that definition of lbtoslug by the inverse of slugtolb and not
// to a different constant you can also get from some tables will make
// lbtoslug*slugtolb == 1 up to the magnitude of roundoff. So converting from
// slug to lb and back will yield to the original value you started with up
// to the magnitude of roundoff.
// Taken from units gnu commandline tool
const double FGJSBBase::slugtolb = 32.174049;
const double FGJSBBase::lbtoslug = 1.0/slugtolb;
const double FGJSBBase::kgtolb = 2.20462;
const double FGJSBBase::kgtoslug = 0.06852168;

const string FGJSBBase::needed_cfg_version = "2.0";
const string FGJSBBase::JSBSim_version = "1.0 "__DATE__" "__TIME__;

std::queue <FGJSBBase::Message> FGJSBBase::Messages;
FGJSBBase::Message FGJSBBase::localMsg;
unsigned int FGJSBBase::messageId = 0;

short FGJSBBase::debug_lvl  = 1;

using std::cerr;
using std::cout;
using std::endl;

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

int FGJSBBase::SomeMessages(void)
{
  return !Messages.empty();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGJSBBase::ProcessMessage(void)
{
  if (Messages.empty()) return;
  localMsg = Messages.front();

  while (Messages.size() > 0) {
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
      if (Messages.size() > 0) localMsg = Messages.front();
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
  std::ostringstream buf;
  buf << Property << '[' << index << ']';
  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::GaussianRandomNumber(void)
{
  static double V1, V2, S;
  static int phase = 0;
  double X;

  if (phase == 0) {
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

  phase = 1 - phase;

  return X;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::VcalibratedFromMach(double mach, double p, double psl, double rhosl)
{
  double pt,A;

  if (mach < 0) mach=0;
  if (mach < 1)    //calculate total pressure assuming isentropic flow
    pt=p*pow((1 + 0.2*mach*mach),3.5);
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

    pt = p*166.92158*pow(mach,7.0)/pow(7*mach*mach-1,2.5);
  }

  A = pow(((pt-p)/psl+1),0.28571);
  return sqrt(7*psl/rhosl*(A-1));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGJSBBase::MachFromVcalibrated(double vcas, double p, double psl, double rhosl)
{
  double pt = p + psl*(pow(1+vcas*vcas*rhosl/(7.0*psl),3.5)-1);

  if (pt/p < 1.89293)
    return sqrt(5.0*(pow(pt/p, 0.2857143) -1)); // Mach < 1
  else {
    // Mach >= 1
    double mach = sqrt(0.77666*pt/p); // Initial guess is based on a quadratic approximation of the Rayleigh formula
    double delta = 1.;
    double target = pt/(166.92158*p);
    int iter = 0;

    // Find the root with Newton-Raphson. Since the differential is never zero,
    // the function is monotonic and has only one root with a multiplicity of one.
    // Convergence is certain.
    while (delta > 1E-5 && iter < 10) {
      double m2 = mach*mach; // Mach^2
      double m6 = m2*m2*m2;  // Mach^6
      delta = mach*m6/pow(7.0*m2-1.0,2.5) - target;
      double diff = 7.0*m6*(2.0*m2-1)/pow(7.0*m2-1.0,3.5); // Never zero when Mach >= 1
      mach -= delta/diff;
      iter++;
    }

    return mach;
  }
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim

