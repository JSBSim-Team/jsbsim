
/*******************************************************************************

 Header:       FGFilter.h
 Author:       
 Date started: 

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

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGFILTER_H
#define FGFILTER_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFCSComponent.h"
#include "../FGConfigFile.h"

/*******************************************************************************
DEFINES
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFilter  : public FGFCSComponent         
{
  float dt;
  float ca;
  float cb;
  float cc;
  float cd;
  float PreviousInput1;
  float PreviousInput2;
  float PreviousOutput1;
  float PreviousOutput2;
  FGConfigFile* AC_cfg;

public:
  FGFilter(FGFCS* fcs, FGConfigFile* AC_cfg);
  ~FGFilter ( ) { }       //Destructor

  float getdt() const {return dt;}
  float getca() const {return ca;}
  float getcb() const {return cb;}
  float getcc() const {return cc;}
  float getcd() const {return cd;}
  float getPreviousInput1() const {return PreviousInput1;}
  float getPreviousInput2() const {return PreviousInput2;}
  float getPreviousOutput1() const {return PreviousOutput1;}
  float getPreviousOutput2() const {return PreviousOutput2;}
  void setdt (float adt) {dt = adt;}
  void setca (float aca) {ca = aca;}
  void setcb (float acb) {cb = acb;}
  void setcc (float acc) {cc = acc;}
  void setcd (float acd) {cd = acd;}
  void setPreviousInput1 (float aPreviousInput1) {PreviousInput1 = aPreviousInput1;}
  void setPreviousInput2 (float aPreviousInput2) {PreviousInput2 = aPreviousInput2;}
  void setPreviousOutput1 (float aPreviousOutput1) {PreviousOutput1 = aPreviousOutput1;}
  void setPreviousOutput2 (float aPreviousOutput2) {PreviousOutput2 = aPreviousOutput2;}

  bool Run (void);
};

#endif
