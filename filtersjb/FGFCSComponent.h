
/*******************************************************************************

 Header:       FGFCSComponent.h
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

#ifndef FGFCSCOMPONENT_H
#define FGFCSCOMPONENT_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

/*******************************************************************************
DEFINES
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFCSComponent
{
  int Type;
  int ID;
  int QueueOrder;
  float Input, Output;

public:
  FGFCSComponent(void) {}

  bool Run (void) {return true;}
  float GetOutput (void) {return 0.0;}
  void SetInput (float in=0.0) {}
   ~ FGFCSComponent ( ) { }       //Destructor
};

#endif

