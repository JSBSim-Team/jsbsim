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
#include <memory>
#include <sstream>
#include <string>

#include "simgear/misc/sg_path.hxx"
#include "FGJSBBase.h"
#include "math/FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;

// The return type of these functions is unspecified by the C++ standard so we
// need some C++ magic to be able to overload the operator<< for these functions.
using setprecision_t = decltype(std::setprecision(0));
// For MSVC set_precision_t and setw_t are the same type
#ifndef _MSC_VER
using setw_t = decltype(std::setw(0));
#endif

enum class LogLevel {
  BULK,  // For frequent messages
  DEBUG, // Less frequent debug type messages
  INFO,  // Informatory messages
  WARN,  // Possible impending problem
  ERROR, // Problem that can be recovered
  FATAL, // Fatal problem => an exception will be thrown
  STDOUT // Standard output - unconditionally displayed.
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

/**
 * Logging backend interface.
 *
 * JSBSim routes each log record to an FGLogger instance instead of writing
 * directly to stdout/stderr. Applications can keep the default `FGLogConsole`
 * backend, or provide their own subclass and register it through
 * `SetLogger(FGLogger_ptr)`.
 *
 * A single log record follows this lifecycle:
 * 1. `SetLevel()` starts a new log record and gives the severity.
 * 2. `FileLocation()` may be called (typically for XML-related messages),
 *    immediately after `SetLevel()` and before message content.
 * 3. `Message()` receives the textual payload. It can be invoked multiple times
 *    for one logical record because JSBSim may build a message in fragments.
 * 4. `Format()` communicates formatting intent (colors, emphasis, reset) for
 *    the subsequent output. Implementations may ignore it if formatting is not
 *    applicable.
 * 5. `Flush()` ends the current log record and is the right place to finalize
 *    and emit the record (for example: prefixing, buffering, forwarding to a
 *    UI, or forcing output synchronization).
 *
 * Implementations are expected to keep enough internal state between these
 * callbacks to assemble one coherent log record.
 *
 * \see SetLogger
 * \see GetLogger
 */
class JSBSIM_API FGLogger
{
public:
  /// Virtual destructor for polymorphic use.
  virtual ~FGLogger() {}
  /// Starts a new log record and provides its severity level.
  virtual void SetLevel(LogLevel level) { log_level = level;}
  /// Optionally provides source filename and line for contextual diagnostics.
  virtual void FileLocation(const std::string& filename, int line) {}
  /// Appends message text. May be called multiple times per log record.
  virtual void Message(const std::string& message) = 0;
  /// Applies a formatting hint to subsequent output.
  virtual void Format(LogFormat format) {}
  /// Ends the current log record and commits any buffered output.
  virtual void Flush(void) {}
protected:
  LogLevel log_level = LogLevel::BULK;
};

using FGLogger_ptr = std::shared_ptr<FGLogger>;

/**
 * Sets the active logger for the current thread.
 *
 * The logger is stored in thread-local storage: all JSBSim instances running
 * in this thread share this same `FGLogger` instance, while other threads keep
 * their own independent logger instance.
 *
 * \see GetLogger
 */
JSBSIM_API void SetLogger(FGLogger_ptr logger);

/**
 * Returns the active logger for the current thread.
 *
 * \see SetLogger
 */
JSBSIM_API FGLogger_ptr GetLogger(void);

class JSBSIM_API FGLogging
{
public:
  FGLogging(LogLevel level);

  virtual ~FGLogging() { Flush(); }
  FGLogging& operator<<(const char* message) { buffer << message ; return *this; }
  FGLogging& operator<<(const std::string& message) { buffer << message ; return *this; }
  // Operator for ints and anonymous enums
  FGLogging& operator<<(int value) { buffer << value; return *this; }
  // Operator for other numerical types
  template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    FGLogging& operator<<(T value) { buffer << value; return *this; }
  FGLogging& operator<<(std::ostream& (*manipulator)(std::ostream&)) { buffer << manipulator; return *this; }
  FGLogging& operator<<(std::ios_base& (*manipulator)(std::ios_base&)) { buffer << manipulator; return *this; }
  FGLogging& operator<<(setprecision_t value) { buffer << value; return *this; }
  // Avoid duplicate definition for MSVC for which set_precision_t and setw_t
  // are the same type
#ifndef _MSC_VER
  FGLogging& operator<<(setw_t value) { buffer << value; return *this; }
#endif
  FGLogging& operator<<(const SGPath& path) { buffer << path; return *this; }
  FGLogging& operator<<(const FGColumnVector3& vec) { buffer << vec; return *this; }
  FGLogging& operator<<(LogFormat format);
  std::streambuf* rdbuf() { return buffer.rdbuf(); }
  void Flush(void);
protected:
  FGLogging(FGLogger_ptr l) : logger(l) {}

  FGLogger_ptr logger;
  std::ostringstream buffer;
};

class JSBSIM_API FGXMLLogging : public FGLogging
{
public:
  FGXMLLogging(Element* el, LogLevel level);
};

class JSBSIM_API FGLogConsole : public FGLogger
{
public:
  void SetMinLevel(LogLevel level) { min_level = level; }
  void FileLocation(const std::string& filename, int line) override
  { buffer.append("\nIn file " + filename + ": line " + std::to_string(line) + "\n"); }
  void Message(const std::string& message) override { buffer.append(message); }
  void Format(LogFormat format) override;
  void Flush(void) override;
  ~FGLogConsole() override { Flush(); }

protected:
  std::string buffer;
  LogLevel min_level = LogLevel::BULK;
};

class JSBSIM_API LogException : public BaseException, public FGLogging
{
public:
  LogException();
  LogException(LogException& other);
  const char* what() const noexcept override;
};

class JSBSIM_API XMLLogException : public LogException
{
public:
  // Construct an XMLLogException using the current thread-local logger
  // and the supplied Element to add file/line location information.
  XMLLogException(Element* el);
  /// This constructor can promote a LogException to an XMLLogException
  /// by adding the file location information to the exception.
  /// This is useful to add some context to an exception that was thrown in a
  /// context where the file location of the error was not known.
  /// @param exception The LogException to promote.
  /// @param el The Element containing the file location of the error.
  XMLLogException(LogException& exception, Element* el);
};
} // namespace JSBSim
#endif
