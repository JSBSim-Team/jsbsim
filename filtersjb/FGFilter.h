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
  float C1;
  float C2;
  float C3;
  float C4;
  float C5;
  float C6;
  float PreviousInput1;
  float PreviousInput2;
  float PreviousOutput1;
  float PreviousOutput2;
  FGConfigFile* AC_cfg;

protected:
  enum {eLag, eRectLag, eLeadLag, eOrder2, eWashout, eIntegrator, eUnknown} FilterType; 

public:
  FGFilter(FGFCS* fcs, FGConfigFile* AC_cfg);
  ~FGFilter ( ) { }       //Destructor

  bool Run (void);
};

#endif
