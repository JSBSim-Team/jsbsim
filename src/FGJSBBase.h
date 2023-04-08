/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGJSBBase.h
 Author:       Jon S. Berndt
 Date started: 07/01/01

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
#include <queue>
#include <string>
#include <cmath>
#include <stdexcept>
#include <random>
#include <chrono>

#include "JSBSim_API.h"
#include "input_output/string_utilities.h"

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class JSBSIM_API BaseException : public std::runtime_error {
  public:
    BaseException(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * @brief Random number generator.
 * This class encapsulates the C++11 random number generation classes for
 * uniform and gaussian (aka normal) distributions as well as their seed.
 * This class guarantees that whenever its seed is reset so are its uniform
 * and normal random number generators.
 */

class JSBSIM_API RandomNumberGenerator {
  public:
    /// Default constructor using a seed based on the system clock.
    RandomNumberGenerator(void) : uniform_random(-1.0, 1.0), normal_random(0.0, 1.0)
    {
      auto seed_value = std::chrono::system_clock::now().time_since_epoch().count();
      generator.seed(static_cast<unsigned int>(seed_value));
    }
    /// Constructor allowing to specify a seed.
    RandomNumberGenerator(unsigned int seed)
      : generator(seed), uniform_random(-1.0, 1.0), normal_random(0.0, 1.0) {}
    /// Specify a new seed and reinitialize the random generation process.
    void seed(unsigned int value) {
      generator.seed(value);
      uniform_random.reset();
      normal_random.reset();
    }
    /** Get a random number which probability of occurrence is uniformly
     * distributed over the segment [-1;1( */
    double GetUniformRandomNumber(void) { return uniform_random(generator); }
    /** Get a random number which probability of occurrence is following Gauss
     * normal distribution with a mean of 0.0 and a standard deviation of 1.0 */
    double GetNormalRandomNumber(void) { return normal_random(generator); }
  private:
    std::default_random_engine generator;
    std::uniform_real_distribution<double> uniform_random;
    std::normal_distribution<double> normal_random;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** JSBSim Base class.
*   This class provides universal constants, utility functions, messaging
*   functions, and enumerated constants to JSBSim.
    @author Jon S. Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGJSBBase {
public:
  /// Constructor for FGJSBBase.
  FGJSBBase() {};

  /// Destructor for FGJSBBase.
  virtual ~FGJSBBase() {};

  /// First order, (low pass / lag) filter
  class Filter {
    double prev_in;
    double prev_out;
    double ca;
    double cb;
  public:
    Filter(void) {}
    Filter(double coeff, double dt) {
      prev_in = prev_out = 0.0;
      double denom = 2.0 + coeff*dt;
      ca = coeff*dt/denom;
      cb = (2.0 - coeff*dt)/denom;
    }
    double execute(double in) {
      double out = (in + prev_in)*ca + prev_out*cb;
      prev_in = in;
      prev_out = out;
      return out;
    }
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

  /** Returns the version number of JSBSim.
  *   @return The version number of JSBSim. */
  static const std::string& GetVersion(void) {return JSBSim_version;}

  /// Disables highlighting in the console output.
  void disableHighLighting(void);

  static short debug_lvl;

  /** Converts from degrees Kelvin to degrees Fahrenheit.
  *   @param kelvin The temperature in degrees Kelvin.
  *   @return The temperature in Fahrenheit. */
  static constexpr double KelvinToFahrenheit (double kelvin) {
    return 1.8*kelvin - 459.4;
  }

  /** Converts from degrees Celsius to degrees Rankine.
  *   @param celsius The temperature in degrees Celsius.
  *   @return The temperature in Rankine. */
  static constexpr double CelsiusToRankine (double celsius) {
    return celsius * 1.8 + 491.67;
  }

  /** Converts from degrees Rankine to degrees Celsius.
  *   @param rankine The temperature in degrees Rankine.
  *   @return The temperature in Celsius. */
  static constexpr double RankineToCelsius (double rankine) {
    return (rankine - 491.67)/1.8;
  }

  /** Converts from degrees Kelvin to degrees Rankine.
  *   @param kelvin The temperature in degrees Kelvin.
  *   @return The temperature in Rankine. */
  static constexpr double KelvinToRankine (double kelvin) {
    return kelvin * 1.8;
  }

  /** Converts from degrees Rankine to degrees Kelvin.
  *   @param rankine The temperature in degrees Rankine.
  *   @return The temperature in Kelvin. */
  static constexpr double RankineToKelvin (double rankine) {
    return rankine/1.8;
  }

  /** Converts from degrees Fahrenheit to degrees Celsius.
  *   @param fahrenheit The temperature in degrees Fahrenheit.
  *   @return The temperature in Celsius. */
  static constexpr double FahrenheitToCelsius (double fahrenheit) {
    return (fahrenheit - 32.0)/1.8;
  }

  /** Converts from degrees Celsius to degrees Fahrenheit.
  *   @param celsius The temperature in degrees Celsius.
  *   @return The temperature in Fahrenheit. */
  static constexpr double CelsiusToFahrenheit (double celsius) {
    return celsius * 1.8 + 32.0;
  }

  /** Converts from degrees Celsius to degrees Kelvin
  *   @param celsius The temperature in degrees Celsius.
  *   @return The temperature in Kelvin. */
  static constexpr double CelsiusToKelvin (double celsius) {
    return celsius + 273.15;
  }

  /** Converts from degrees Kelvin to degrees Celsius
  *   @param celsius The temperature in degrees Kelvin.
  *   @return The temperature in Celsius. */
  static constexpr double KelvinToCelsius (double kelvin) {
    return kelvin - 273.15;
  }

  /** Converts from feet to meters
  *   @param measure The length in feet.
  *   @return The length in meters. */
  static constexpr double FeetToMeters (double measure) {
    return measure*0.3048;
  }

  /** Finite precision comparison.
      @param a first value to compare
      @param b second value to compare
      @return if the two values can be considered equal up to roundoff */
  static bool EqualToRoundoff(double a, double b) {
    double eps = 2.0*DBL_EPSILON;
    return std::fabs(a - b) <= eps * std::max<double>(std::fabs(a), std::fabs(b));
  }

  /** Finite precision comparison.
      @param a first value to compare
      @param b second value to compare
      @return if the two values can be considered equal up to roundoff */
  static bool EqualToRoundoff(float a, float b) {
    float eps = 2.0*FLT_EPSILON;
    return std::fabs(a - b) <= eps * std::max<double>(std::fabs(a), std::fabs(b));
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

  /** Constrain a value between a minimum and a maximum value.
  */
  static constexpr double Constrain(double min, double value, double max) {
    return value<min?(min):(value>max?(max):(value));
  }

  static constexpr double sign(double num) {return num>=0.0?1.0:-1.0;}

protected:
  static constexpr double radtodeg = 180. / M_PI;
  static constexpr double degtorad = M_PI / 180.;
  static constexpr double hptoftlbssec = 550.0;
  static constexpr double psftoinhg = 0.014138;
  static constexpr double psftopa = 47.88;
  static constexpr double fttom = 0.3048;
  static constexpr double ktstofps = 1852./(3600*fttom);
  static constexpr double fpstokts = 1.0 / ktstofps;
  static constexpr double inchtoft = 1.0/12.0;
  static constexpr double m3toft3 = 1.0/(fttom*fttom*fttom);
  static constexpr double in3tom3 = inchtoft*inchtoft*inchtoft/m3toft3;
  static constexpr double inhgtopa = 3386.38;
  /** Note that definition of lbtoslug by the inverse of slugtolb and not to a
      different constant you can also get from some tables will make
      lbtoslug*slugtolb == 1 up to the magnitude of roundoff. So converting from
      slug to lb and back will yield to the original value you started with up
      to the magnitude of roundoff.
      Taken from units gnu commandline tool */
  static constexpr double slugtolb = 32.174049;
  static constexpr double lbtoslug = 1.0/slugtolb;
  static constexpr double kgtolb = 2.20462;
  static constexpr double kgtoslug = 0.06852168;
  static const std::string needed_cfg_version;
  static const std::string JSBSim_version;

  static std::string CreateIndexedPropertyName(const std::string& Property, int index);

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
