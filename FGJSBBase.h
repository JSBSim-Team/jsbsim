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

#define ID_JSBBASE "$Id: FGJSBBase.h,v 1.8 2001/11/10 15:09:35 jberndt Exp $"

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
    @version $Id: FGJSBBase.h,v 1.8 2001/11/10 15:09:35 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGJSBBase {
public:
  /// Constructor for FGJSBBase.
  FGJSBBase() {};

  /// Destructor for FGJSBBase.
  virtual ~FGJSBBase() {};

  ///@name JSBSim Enums.
  //@{
  enum {eL     = 1, eM,     eN    };
  enum {eP     = 1, eQ,     eR    };
  enum {eU     = 1, eV,     eW    };
  enum {eX     = 1, eY,     eZ    };
  enum {ePhi   = 1, eTht,   ePsi  };
  enum {eDrag  = 1, eSide,  eLift };
  enum {eRoll  = 1, ePitch, eYaw  };
  enum {eNorth = 1, eEast,  eDown };
  //@}
  
  ///@name JSBSim console output highlighting terms.
  //@{
  static char highint[5];
  static char halfint[5];
  static char normint[6];
  static char reset[5];
  static char underon[5];
  static char underoff[6];
  static char fgblue[6];
  static char fgcyan[6];
  static char fgred[6];
  static char fggreen[6];
  static char fgdef[6];
  //@}

  ///@name JSBSim Messaging functions
  //@{
  struct Message* PutMessage(struct Message* msg);
  struct Message* PutMessage(string text);
  struct Message* PutMessage(string text, bool bVal);
  struct Message* PutMessage(string text, int iVal);
  struct Message* PutMessage(string text, double dVal);
  struct Message* ReadMessage(void);
  struct Message* ProcessMessage(void);
  //@}

protected:
  static struct Message  localMsg;
  
  static queue <struct Message*> Messages;

  virtual void Debug(void) {};

  static short debug_lvl;
  static int frame;
  static unsigned int messageId;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

