/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGJSBBase.h
 Author:       Jon S. Berndt
 Date started: 07/01/01

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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

#ifdef FGFS
#  include <simgear/compiler.h>
#  include <math.h>
#  include <queue>
#  include STL_STRING
  SG_USING_STD(queue);
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#    include <queue.h>
#    include <string.h>
#  else
#    include <cmath>
#    include <queue>
#    include <string>
#  endif
#endif

#ifndef M_PI 
#  include <simgear/constants.h>
#  define M_PI SG_PI
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_JSBBASE "$Id: FGJSBBase.h,v 1.12 2001/11/12 09:56:12 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

enum eParam {
  FG_UNDEF = 0,
  FG_TIME,
  FG_QBAR,
  FG_WINGAREA,
  FG_WINGSPAN,
  FG_CBAR,
  FG_ALPHA,
  FG_ALPHADOT,
  FG_BETA,
  FG_ABETA,
  FG_BETADOT,
  FG_PHI,
  FG_THT,
  FG_PSI,
  FG_PITCHRATE,
  FG_ROLLRATE,
  FG_YAWRATE,
  FG_CL_SQRD,
  FG_MACH,
  FG_ALTITUDE,
  FG_BI2VEL,
  FG_CI2VEL,
  FG_ELEVATOR_POS,
  FG_AILERON_POS,
  FG_RUDDER_POS,
  FG_SPDBRAKE_POS,
  FG_SPOILERS_POS,
  FG_FLAPS_POS,
  FG_ELEVATOR_CMD,
  FG_AILERON_CMD,
  FG_RUDDER_CMD,
  FG_SPDBRAKE_CMD,
  FG_SPOILERS_CMD,
  FG_FLAPS_CMD,
  FG_THROTTLE_CMD,
  FG_THROTTLE_POS,
  FG_MIXTURE_CMD,
  FG_MIXTURE_POS,
  FG_MAGNETO_CMD,
  FG_STARTER_CMD,
  FG_ACTIVE_ENGINE,
  FG_HOVERB,
  FG_PITCH_TRIM_CMD,
  FG_YAW_TRIM_CMD,
  FG_ROLL_TRIM_CMD,
  FG_LEFT_BRAKE_CMD,
  FG_CENTER_BRAKE_CMD,
  FG_RIGHT_BRAKE_CMD,
  FG_SET_LOGGING,
  FG_ALPHAH,
  FG_ALPHAW,
  FG_LBARH,     //normalized horizontal tail arm
  FG_LBARV,     //normalized vertical tail arm
  FG_HTAILAREA,
  FG_VTAILAREA,
  FG_VBARH,    //horizontal tail volume 
  FG_VBARV     //vertical tail volume 
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** JSBSim Base class.
    @author Jon S. Berndt
    @version $Id: FGJSBBase.h,v 1.12 2001/11/12 09:56:12 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGJSBBase {
public:
  /// Constructor for FGJSBBase.
  FGJSBBase();

  /// Destructor for FGJSBBase.
  virtual ~FGJSBBase() {};

  /// JSBSim Message structure
  struct Message {
    unsigned int fdmId;
    unsigned int messageId;
    string text;
    string subsystem;
    enum mType {eText, eInteger, eDouble, eBool} type;
    bool bVal;
    int  iVal;
    double dVal;
  };

  ///@name JSBSim Enums.
  //@{
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
  //@}
  
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
  struct Message* PutMessage(struct Message* msg);
  /** Creates a message with the given text and places it on the queue.
      @param text message text
      @return pointer to a Message structure */
  struct Message* PutMessage(string text);
  /** Creates a message with the given text and boolean value and places it on the queue.
      @param text message text
      @param bVal boolean value associated with the message
      @return pointer to a Message structure */
  struct Message* PutMessage(string text, bool bVal);
  /** Creates a message with the given text and integer value and places it on the queue.
      @param text message text
      @param iVal integer value associated with the message
      @return pointer to a Message structure */
  struct Message* PutMessage(string text, int iVal);
  /** Creates a message with the given text and double value and places it on the queue.
      @param text message text
      @param dVal double value associated with the message
      @return pointer to a Message structure */
  struct Message* PutMessage(string text, double dVal);
  /** Reads the message on the queue (but does not delete it).
      @return pointer to a Message structure (or NULL if no mesage) */
  struct Message* ReadMessage(void);
  /** Reads the message on the queue and removes it from the queue.
      @return pointer to a Message structure (or NULL if no mesage) */
  struct Message* ProcessMessage(void);
  //@}

protected:
  static struct Message localMsg;
  
  static queue <struct Message*> Messages;

  virtual void Debug(void) {};

  static short debug_lvl;
  static int frame;
  static unsigned int messageId;
  
  static const double radtodeg;
  static const double degtorad;
  static const double hptoftlbssec;
  static const double fpstokts;
  static const double ktstofps;
  static const double inchtoft;
  static const double Reng;         // Specific Gas Constant,ft^2/(sec^2*R)
  static const double SHRatio;
  static const string needed_cfg_version;
  static const string JSBSim_version;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

