/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGLog.h
 Author:       Bertrand Coconnier
 Date started: 05/03/24

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
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
05/03/24   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGLOG_H
#define FGLOG_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "simgear/misc/sg_path.hxx"
#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;

// The return type of std::setprecision is unspecified by the C++ standard so we
// need some C++ magic to be able to overload the operator<< for std::setprecision
using setprecision_t = decltype(std::setprecision(0));

enum class LogLevel {
  BULK,  // For frequent messages
  DEBUG, // Less frequent debug type messages
  INFO,  // Informatory messages
  WARN,  // Possible impending problem
  ERROR, // Problem that can be recovered
  FATAL  // Fatal problem => an exception will be thrown
};

enum class LogFormat {
  RESET,
  RED,
  BLUE,
  CYAN,
  GREEN,
  DEFAULT,
  BOLD,
  NORMAL,
  UNDERLINE_ON,
  UNDERLINE_OFF
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGLogger
{
public:
  virtual ~FGLogger() {}
  virtual void SetLevel(LogLevel level) { level = level; }
  virtual void FileLocation(const std::string& filename, int line) {}
  void SetMinLevel(LogLevel level) { min_level = level; }
  virtual void Message(const std::string& message) = 0;
  virtual void Format(LogFormat format) {}
  virtual void Flush(void) {}
protected:
  LogLevel level = LogLevel::BULK;
  LogLevel min_level = LogLevel::INFO;
};

class FGLogging
{
public:
  FGLogging(std::shared_ptr<FGLogger> logger, LogLevel level)
    : logger(logger)
  { logger->SetLevel(level); }

  virtual ~FGLogging() { Flush(); }
  FGLogging& operator<<(const char* message) { buffer << message ; return *this; }
  FGLogging& operator<<(const std::string& message) { buffer << message ; return *this; }
  FGLogging& operator<<(unsigned int value) { buffer << value; return *this; }
  FGLogging& operator<<(std::ostream& (*manipulator)(std::ostream&)) { buffer << manipulator; return *this; }
  FGLogging& operator<<(setprecision_t value) { buffer << value; return *this; }
  FGLogging& operator<<(const SGPath& path) { buffer << path; return *this; }
  FGLogging& operator<<(LogFormat format);
  std::string str(void) const { return buffer.str(); }
  void Flush(void);
protected:
  std::shared_ptr<FGLogger> logger;
  std::ostringstream buffer;
};

class FGXMLLogging : public FGLogging
{
public:
  FGXMLLogging(std::shared_ptr<FGLogger> logger, Element* el, LogLevel level);
};

class FGLogConsole : public FGLogger
{
public:
  FGLogConsole() : out(std::cout.rdbuf()) {}

  void SetLevel(LogLevel level) override;
  void FileLocation(const std::string& filename, int line) override
  { out << std::endl << "In file " << filename << ": line" << line << std::endl; }
  void Format(LogFormat format) override;
  void Flush(void) override {
    out.flush();
    out.clear();
  }

  void Message(const std::string& message) override {
    // if (level < min_level) return;
    out << message;
  }

private:
  std::ostream out;
};
} // namespace JSBSim
#endif
