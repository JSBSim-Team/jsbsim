/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGJSBBase.h
 Author:       Jon S. Berndt
 Date started: 07/01/01

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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
07/01/01  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGJSBBASE_H
#define FGJSBBASE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <float.h>

#ifdef FGFS
#  include <simgear/compiler.h>
#  include <math.h>
#  include <queue>
#  include STL_STRING

SG_USING_STD(string);

#  ifndef M_PI
#    include <simgear/constants.h>
#    define M_PI SG_PI
#  endif

#else  // JSBSim section

#  include <queue>
#  include <string>
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif

using std::string;

#  if defined(_MSC_VER) && _MSC_VER <= 1200
#    ifndef max
#      define max(a,b)            (((a) > (b)) ? (a) : (b))
#    endif

#    ifndef min
#      define min(a,b)            (((a) < (b)) ? (a) : (b))
#    endif
#  else

using std::fabs;

#  endif

#  ifndef M_PI
#    define M_PI 3.14159265358979323846
#  endif

#endif

#if !defined(WIN32) || defined(__GNUC__) || (defined(_MSC_VER) && (_MSC_VER >= 1300))
using std::max;
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_JSBBASE "$Id: FGJSBBase.h,v 1.11 2007/02/28 03:15:44 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** JSBSim Base class.
*   This class provides universal constants, utility functions, messaging
*   functions, and enumerated constants to JSBSim.
    @author Jon S. Berndt
    @version $Id: FGJSBBase.h,v 1.11 2007/02/28 03:15:44 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGJSBBase {
public:
  /// Constructor for FGJSBBase.
  FGJSBBase() {};

  /// Destructor for FGJSBBase.
  ~FGJSBBase() {};

  /// JSBSim Message structure
  class Message {
  public:
    unsigned int fdmId;
    unsigned int messageId;
    string text;
    string subsystem;
    enum mType {eText, eInteger, eDouble, eBool} type;
    bool bVal;
    int  iVal;
    double dVal;
  };

  ///@name JSBSim console output highlighting terms.
  //@{
  /// highlights text
  static char highint[5];
  /// low intensity text
  static char halfint[5];
  /// normal intensity text
  static char normint[6];
  /// resets text properties
  static char reset[5];
  /// underlines text
  static char underon[5];
  /// underline off
  static char underoff[6];
  /// blue text
  static char fgblue[6];
  /// cyan text
  static char fgcyan[6];
  /// red text
  static char fgred[6];
  /// green text
  static char fggreen[6];
  /// default text
  static char fgdef[6];
  //@}

  ///@name JSBSim Messaging functions
  //@{
  /** Places a Message structure on the Message queue.
      @param msg pointer to a Message structure
      @return pointer to a Message structure */
void PutMessage(const Message& msg);
  /** Creates a message with the given text and places it on the queue.
      @param text message text
      @return pointer to a Message structure */
  void PutMessage(const string& text);
  /** Creates a message with the given text and boolean value and places it on the queue.
      @param text message text
      @param bVal boolean value associated with the message
      @return pointer to a Message structure */
void PutMessage(const string& text, bool bVal);
  /** Creates a message with the given text and integer value and places it on the queue.
      @param text message text
      @param iVal integer value associated with the message
      @return pointer to a Message structure */
void PutMessage(const string& text, int iVal);
  /** Creates a message with the given text and double value and places it on the queue.
      @param text message text
      @param dVal double value associated with the message
      @return pointer to a Message structure */
void PutMessage(const string& text, double dVal);
  /** Reads the message on the queue (but does not delete it).
      @return 1 if some messages */
  int SomeMessages(void);
  /** Reads the message on the queue and removes it from the queue.
      @return pointer to a Message structure (or NULL if no mesage) */
  Message* ProcessMessage(void);
  //@}

  /** Returns the version number of JSBSim.
  *   @return The version number of JSBSim. */
  string GetVersion(void) {return JSBSim_version;}

  /// Disables highlighting in the console output.
  void disableHighLighting(void);

  static short debug_lvl;

  /** Converts from degrees Kelvin to degrees Fahrenheit.
  *   @param kelvin The temperature in degrees Kelvin.
  *   @return The temperature in Fahrenheit. */
  static double KelvinToFahrenheit (double kelvin) {
    return 1.8*kelvin - 459.4;
  }

  /** Converts from degrees Rankine to degrees Celsius.
  *   @param rankine The temperature in degrees Rankine.
  *   @return The temperature in Celsius. */
  static double RankineToCelsius (double rankine) {
    return (rankine - 491.67)/1.8;
  }

  /** Converts from degrees Rankine to degrees Kelvin.
  *   @param rankine The temperature in degrees Rankine.
  *   @return The temperature in Kelvin. */
  static double RankineToKelvin (double rankine) {
    return rankine/1.8;
  }

  /** Converts from degrees Fahrenheit to degrees Celsius.
  *   @param fahrenheit The temperature in degrees Fahrenheit.
  *   @return The temperature in Celsius. */
  static double FahrenheitToCelsius (double fahrenheit) {
    return (fahrenheit - 32.0)/1.8;
  }

  /** Converts from degrees Celsius to degrees Fahrenheit.
  *   @param celsius The temperature in degrees Celsius.
  *   @return The temperature in Fahrenheit. */
  static double CelsiusToFahrenheit (double celsius) {
    return celsius * 1.8 + 32.0;
  }

  /** Finite precision comparison.
      @param a first value to compare
      @param b second value to compare
      @return if the two values can be considered equal up to roundoff */
  static bool EqualToRoundoff(double a, double b) {
    double eps = 2.0*DBL_EPSILON;
    return fabs(a - b) <= eps*max(fabs(a), fabs(b));
  }

  /** Finite precision comparison.
      @param a first value to compare
      @param b second value to compare
      @return if the two values can be considered equal up to roundoff */
  static bool EqualToRoundoff(float a, float b) {
    float eps = 2.0*FLT_EPSILON;
    return fabs(a - b) <= eps*max(fabs(a), fabs(b));
  }

  /** Finite precision comparison.
      @param a first value to compare
      @param b second value to compare
      @return if the two values can be considered equal up to roundoff */
  static bool EqualToRoundoff(float a, double b) {
    return EqualToRoundoff(a, (float)b);
  }

  /** Finite precision comparison.
      @param a first value to compare
      @param b second value to compare
      @return if the two values can be considered equal up to roundoff */
  static bool EqualToRoundoff(double a, float b) {
    return EqualToRoundoff((float)a, b);
  }

protected:
  static Message localMsg;

  static std::queue <Message> Messages;

  void Debug(int from) {};

  static unsigned int messageId;

  static const double radtodeg;
  static const double degtorad;
  static const double hptoftlbssec;
  static const double psftoinhg;
  static const double psftopa;
  static const double fpstokts;
  static const double ktstofps;
  static const double inchtoft;
  static const double in3tom3;
  static double Reng;         // Specific Gas Constant,ft^2/(sec^2*R)
  static const double SHRatio;
  static const double lbtoslug;
  static const double slugtolb;
  static const string needed_cfg_version;
  static const string JSBSim_version;

public:
/// Moments L, M, N
enum {eL     = 1, eM,     eN    };
/// Rates P, Q, R
enum {eP     = 1, eQ,     eR    };
/// Velocities U, V, W
enum {eU     = 1, eV,     eW    };
/// Positions X, Y, Z
enum {eX     = 1, eY,     eZ    };
/// Euler angles Phi, Theta, Psi
enum {ePhi   = 1, eTht,   ePsi  };
/// Stability axis forces, Drag, Side force, Lift
enum {eDrag  = 1, eSide,  eLift };
/// Local frame orientation Roll, Pitch, Yaw
enum {eRoll  = 1, ePitch, eYaw  };
/// Local frame position North, East, Down
enum {eNorth = 1, eEast,  eDown };
/// Locations Radius, Latitude, Longitude
enum {eLat = 1, eLong, eRad     };
/// Conversion specifiers
enum {inNone = 0, inDegrees, inRadians, inMeters, inFeet };

};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

