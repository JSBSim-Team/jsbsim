/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLog.cpp
 Author:       Bertrand Coconnier
 Date started: 05/03/24
 Purpose:      Manage the logging of messages

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
This is the place where the logging of messages is managed. The messages can be
sent to the console, to a file, etc.

HISTORY
--------------------------------------------------------------------------------
05/03/24   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <string_view>
#include <cstring>

#include "FGLog.h"
#include "input_output/FGXMLElement.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class BufferLogger : public FGLogger
{
public:
  BufferLogger(std::shared_ptr<FGLogger> logger) : logger(logger) {
    logMessageBuffer[0] = '\0';
  }
  void FileLocation(const std::string& filename, int line) override {
    this->filename = filename;
    this->line = line;
  }
  void Message(const std::string& message) override;
  void Format(LogFormat format) override;
  const char* c_str(void) const noexcept { return logMessageBuffer; }
  ~BufferLogger() override;

private:
  struct MessageToken
  {
    std::string_view messageItem;
    LogFormat format = LogFormat::DEFAULT;
  };

  char logMessageBuffer[1024];
  size_t bufferUsed = 0;
  std::vector<MessageToken> tokens;
  const std::shared_ptr<FGLogger> logger;
  std::string filename;
  int line = -1;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void BufferLogger::Message(const std::string& message) {
  if (message.empty()) return;

  size_t available = sizeof(logMessageBuffer) - bufferUsed - 1;  // -1 for null terminator
  size_t toCopy = std::min(message.size(), available); // Prevent buffer overflow

  if (toCopy > 0) {
    // Copy the message to the buffer
    char* currentPos = logMessageBuffer + bufferUsed;
    memcpy(currentPos, message.c_str(), toCopy);
    bufferUsed += toCopy;
    logMessageBuffer[bufferUsed] = '\0';
    // Store the message in the tokens vector
    auto& new_token = tokens.emplace_back();
    new_token.messageItem = std::string_view(currentPos, toCopy);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void BufferLogger::Format(LogFormat format)
{
  auto& new_token = tokens.emplace_back();
  new_token.format = format;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

BufferLogger::~BufferLogger()
{
  if (tokens.empty()) return;

  logger->SetLevel(log_level);

  if (line > 0) logger->FileLocation(filename, line);

  for (const auto& token : tokens) {
    if (token.messageItem.empty()) {
      logger->Format(token.format);
      continue;
    }
    logger->Message(std::string(token.messageItem));
  }
  logger->Flush();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLogging::Flush(void)
{
  std::string message = buffer.str();

  if (!message.empty()) {
    logger->Message(message);
    buffer.str("");
  }

  logger->Flush();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLogging& FGLogging::operator<<(LogFormat format) {
  std::string message = buffer.str();

  if (!message.empty()) {
    logger->Message(message);
    buffer.str("");
  }

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

void FGLogConsole::Flush(void) {
  switch (log_level)
  {
  case LogLevel::BULK:
  case LogLevel::DEBUG:
  case LogLevel::INFO:
    std::cout << buffer;
    std::cout.flush(); // Force the message to be immediately displayed in the console
    break;
  default:
    std::cerr << buffer;
    std::cerr.flush(); // Force the message to be immediately displayed in the console
    break;
  }

  buffer.clear();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLogConsole::Format(LogFormat format) {
  switch (format)
  {
  case LogFormat::RED:
    buffer.append(FGJSBBase::fgred);
    break;
  case LogFormat::BLUE:
    buffer.append(FGJSBBase::fgblue);
    break;
  case LogFormat::BOLD:
    buffer.append(FGJSBBase::highint);
    break;
  case LogFormat::NORMAL:
    buffer.append(FGJSBBase::normint);
    break;
  case LogFormat::UNDERLINE_ON:
    buffer.append(FGJSBBase::underon);
    break;
  case LogFormat::UNDERLINE_OFF:
    buffer.append(FGJSBBase::underoff);
    break;
  case LogFormat::DEFAULT:
    buffer.append(FGJSBBase::fgdef);
    break;
  case LogFormat::RESET:
  default:
    buffer.append(FGJSBBase::reset);
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

LogException::LogException(std::shared_ptr<FGLogger> logger)
: BaseException(""), FGLogging(std::make_shared<BufferLogger>(logger), LogLevel::FATAL) {}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

LogException::LogException(LogException& other)
: BaseException(""), FGLogging(other.logger, LogLevel::FATAL)
{
  other.Flush(); // Make the data buffered in `other` accessible to all copies.
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const char* LogException::what() const noexcept
{
  // Although using const_cast is generally discouraged, it is justified here because:
  // 1. The what() method must be const to comply with std::exception interface
  // 2. We need to ensure all buffered messages are flushed before returning the error message
  // 3. Conceptually, getting the complete error message is a "logically const" operation
  //    i.e. from the user's perspective, it doesn't modify the state of the object.
  const_cast<LogException*>(this)->Flush();
  return static_cast<BufferLogger*>(logger.get())->c_str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

XMLLogException::XMLLogException(std::shared_ptr<FGLogger> logger, Element* el)
  : LogException(logger)
{
  this->logger->FileLocation(el->GetFileName(), el->GetLineNumber());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

XMLLogException::XMLLogException(LogException& exception, Element* el)
  : LogException(exception)
{
  logger->FileLocation(el->GetFileName(), el->GetLineNumber());
}
};
