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
#  include <queue.h>
#  include <string.h>
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

#define ID_JSBBASE "$Id: FGJSBBase.h,v 1.10 2001/11/11 23:06:26 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** JSBSim Base class.
    @author Jon S. Berndt
    @version $Id: FGJSBBase.h,v 1.10 2001/11/11 23:06:26 jberndt Exp $
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

  ///@name JSBSim Enums.
  //@{
  /// Moments
  enum {eL     = 1, eM,     eN    };
  /// Rates
  enum {eP     = 1, eQ,     eR    };
  /// Velocities
  enum {eU     = 1, eV,     eW    };
  /// Positions
  enum {eX     = 1, eY,     eZ    };
  /// Euler angles
  enum {ePhi   = 1, eTht,   ePsi  };
  /// Stability axis forces
  enum {eDrag  = 1, eSide,  eLift };
  /// Local frame orientation
  enum {eRoll  = 1, ePitch, eYaw  };
  /// Local frame position
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
  static struct Message  localMsg;
  
  static queue <struct Message*> Messages;

  virtual void Debug(void) {};

  static short debug_lvl;
  static int frame;
  static unsigned int messageId;
  
  static const double radtodeg;
  static const double degtorad;
  static const double hptoftlbssec;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

