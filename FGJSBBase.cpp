/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGJSBBase.cpp
 Author:       Jon S. Berndt
 Date started: 07/01/01
 Purpose:      Encapsulates the JSBBase object

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
07/01/01  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"

static const char *IdSrc = "$Id: FGJSBBase.cpp,v 1.7 2001/11/23 20:06:17 jberndt Exp $";
static const char *IdHdr = ID_JSBBASE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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

const double FGJSBBase::radtodeg = 57.29578;
const double FGJSBBase::degtorad = 1.745329E-2;
const double FGJSBBase::hptoftlbssec = 550.0;
const double FGJSBBase::fpstokts = 0.592484;
const double FGJSBBase::ktstofps = 1.68781;
const double FGJSBBase::inchtoft = 0.08333333;
const double FGJSBBase::Reng = 1716.0;
const double FGJSBBase::SHRatio = 1.40;
const string FGJSBBase::needed_cfg_version = "1.55";
const string FGJSBBase::JSBSim_version = "0.9.1";

queue <struct FGJSBBase::Message*> FGJSBBase::Messages;
struct FGJSBBase::Message FGJSBBase::localMsg;
unsigned int FGJSBBase::messageId = 0; 
unsigned int FGJSBBase::frame = 0;

short FGJSBBase::debug_lvl  = 0;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGJSBBase::FGJSBBase()
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::PutMessage(struct Message* msg)
{
  Messages.push(msg);
  return msg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::PutMessage(string text)
{
  struct Message *msg = new Message();
  msg->text = text;
  msg->messageId = messageId++;
  msg->subsystem = "FDM";
  msg->type = Message::eText;
  Messages.push(msg);
  return msg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::PutMessage(string text, bool bVal)
{
  struct Message *msg = new Message();
  msg->text = text;
  msg->messageId = messageId++;
  msg->subsystem = "FDM";
  msg->type = Message::eBool;
  msg->bVal = bVal;
  Messages.push(msg);
  return msg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::PutMessage(string text, int iVal)
{
  struct Message *msg = new Message();
  msg->text = text;
  msg->messageId = messageId++;
  msg->subsystem = "FDM";
  msg->type = Message::eInteger;
  msg->bVal = iVal;
  Messages.push(msg);
  return msg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::PutMessage(string text, double dVal)
{
  struct Message *msg = new Message();
  msg->text = text;
  msg->messageId = messageId++;
  msg->subsystem = "FDM";
  msg->type = Message::eDouble;
  msg->bVal = dVal;
  Messages.push(msg);
  return msg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::ReadMessage(void)
{
  if (!Messages.empty()) return Messages.front();
  else                   return NULL;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

struct FGJSBBase::Message* FGJSBBase::ProcessMessage(void)
{
  if (!Messages.empty())
    localMsg = *(Messages.front());
  else 
    return NULL;
  Messages.pop();
  return &localMsg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

