/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputType.cpp
 Author:       Bertrand Coconnier
 Date started: 05/03/24
 Purpose:      Manage output of sim parameters to file or stdout

  ------------- Copyright (C) 2024 Bertrand Coconnier -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
05/03/24   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGLog.h"
#include "input_output/FGXMLElement.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void FGLogging::Flush(void)
{
  logger->Message(buffer.str());
  buffer.str("");
  logger->Format(LogFormat::RESET);
  logger->Flush();
  buffer.clear();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLogging& FGLogging::operator<<(LogFormat format) {
  logger->Message(buffer.str());
  buffer.str("");
  logger->Format(format);
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGXMLLogging::FGXMLLogging(std::shared_ptr<FGLogger> logger, Element* el, LogLevel level)
  : FGLogging(logger, level)
{
  logger->FileLocation(el->GetFileName(), el->GetLineNumber());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLogConsole::SetLevel(LogLevel level) {
  FGLogger::SetLevel(level);
  switch (level)
  {
  case LogLevel::BULK:
  case LogLevel::DEBUG:
  case LogLevel::INFO:
    out.tie(&std::cout);
    break;
  default:
    out.tie(&std::cerr);
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLogConsole::Format(LogFormat format) {
  switch (format)
  {
  case LogFormat::RED:
    out << FGJSBBase::fgred;
    break;
  case LogFormat::BLUE:
    out << FGJSBBase::fgblue;
    break;
  case LogFormat::BOLD:
    out << FGJSBBase::highint;
    break;
  case LogFormat::NORMAL:
    out << FGJSBBase::normint;
    break;
  case LogFormat::UNDERLINE_ON:
    out << FGJSBBase::underon;
    break;
  case LogFormat::UNDERLINE_OFF:
    out << FGJSBBase::underoff;
    break;
  case LogFormat::DEFAULT:
    out << FGJSBBase::fgdef;
    break;
  case LogFormat::RESET:
  default:
    out << FGJSBBase::reset;
    break;
  }
}
}
