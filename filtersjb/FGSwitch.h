/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSwitch.h
 Author:       Jon S. Berndt
 Date started: 12/23/2002

 ------------- Copyright (C)  -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSWITCH_H
#define FGSWITCH_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include "../FGConfigFile.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_SWITCH "$Id: FGSwitch.h,v 1.13 2002/12/27 13:04:51 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a filter for the flight control system.
<COMPONENT NAME="switch1" TYPE="SWITCH">
  <TEST LOGIC="{AND|OR}>
    {property} {conditional} {PROPERTY|INT|DOUBLE} {property|value}
    {property} {conditional} {PROPERTY|INT|DOUBLE} {property|value}
    ...
  </TEST>
  <TEST LOGIC="{AND|OR}>
    {property} {conditional} {PROPERTY|INT|DOUBLE} {property|value}
    ...
  </TEST>
  ...
  <VALUE TYPE={BOOLEAN|MULTIPLE}>
    <!-- if component value chosen based on the value of the
         components set of tests, the output is chosen as follows -->
    TRUE  {property|value}
    FALSE {property|value}
    <!-- if component value is MULTIPLE, the output value of the
         component is chosen for the first top-level test that evaluates
         to true -->
    TEST 0 {PROPERTY|INT|DOUBLE} {property|value}
    TEST 1 {PROPERTY|INT|DOUBLE} {property|value}
    ...
  </VALUE>
</COMPONENT>
    */
   
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSwitch  : public FGFCSComponent
{
public:
  FGSwitch(FGFCS* fcs, FGConfigFile* AC_cfg);
  ~FGSwitch();

  bool Run(void);

private:
  FGFCS* fcs;
  FGConfigFile* AC_cfg;

  enum eLogic {eAnd=0, eOr};
  enum eComparison {eEQ=0, eNE, eGT, eGE, eLT, eLE};

  struct condition {
    FGPropertyManager *TestParam1, *TestParam2;
    double TestValue;
    eComparison Comparison;
    double compare_val;
  };

  map <const string, eComparison> mComparison;

  struct test {
    vector <condition> conditions;
    eLogic Logic;
  };

  vector <test> tests;

  void Debug(int from);
};

#endif
