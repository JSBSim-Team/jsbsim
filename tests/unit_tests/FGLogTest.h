#include <cxxtest/TestSuite.h>
#include <input_output/FGLog.h>
#include <input_output/FGXMLElement.h>

class DummyLogger : public JSBSim::FGLogger
{
public:
  JSBSim::LogLevel GetLogLevel() const { return log_level; }
  void Message(const std::string& message) override { buffer.append(message); }
  void FileLocation(const std::string& filename, int line) override {
    buffer.append(filename);
    buffer.append(":");
    buffer.append(std::to_string(line));
  }
  void Format(JSBSim::LogFormat format) override {
  switch (format)
  {
  case JSBSim::LogFormat::NORMAL:
    buffer.append("NORMAL");
    break;
  default:
    buffer.append("UNKNOWN");
    break;
  }
  }
  void Flush(void) override { flushed = true; }

  std::string buffer;
  bool flushed = false;
};

class FGLogTest : public CxxTest::TestSuite
{
public:
std::shared_ptr<DummyLogger> logger;

void setUp() {
  logger = std::make_shared<DummyLogger>();
  JSBSim::SetLogger(logger);
}

void testConstructor() {
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);

  JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testDestructor() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testCharMessage() {
  const char* message = "Hello, World!";
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log <<message;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testStringMessage() {
  std::string message = "Hello, World!";
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << message;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testConcatenatedMessages() {
  std::string message1 = "Hello";
  std::string message2 = "World!";
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << message1 << " " << message2;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message1 + " " + message2);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testEndl() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << "Hello" << std::endl << "World!";
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Hello\nWorld!");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testNumbers() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << 1 << 2.1 << -3.4f;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "12.1-3.4");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testSetPrecision() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << std::setprecision(3) << 1.23456789;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "1.23");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testSetWidthRight() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << std::setw(5) << 123;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "  123");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testSetWidthLeft() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << std::left << std::setw(5) << 123;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "123  ");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testPath() {
  SGPath path("path/to");
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << (path/"file");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Path \"path/to/file\"");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testColumnVector3() {
  JSBSim::FGColumnVector3 vec(1, 2, 3);
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << vec;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "1 , 2 , 3");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testFormatOnly() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    log << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "NORMAL");
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "NORMAL");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testClosingFormat() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << "Hello,";
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testMidFormat() {
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << "Hello,";
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
    log << " World!";
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL World!");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testXMLLogging() {
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  {
    JSBSim::FGXMLLogging log(&el, JSBSim::LogLevel::DEBUG);
    TS_ASSERT_EQUALS(logger->buffer, "file.xml:42");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::DEBUG);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "file.xml:42");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::DEBUG);
}
};

class FGLogConsoleTest : public CxxTest::TestSuite
{
public:
std::shared_ptr<JSBSim::FGLogConsole> logger;

void setUp() {
  logger = std::make_shared<JSBSim::FGLogConsole>();
  JSBSim::SetLogger(logger);
}

void testNormalMessage() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::DEBUG);
    log << "Hello, World!";
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "Hello, World!");
}

void testErrorMessage() {
  std::ostringstream buffer;
  auto cerr_buffer = std::cerr.rdbuf();
  std::cerr.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::ERROR);
    log << "Hello, World!";
  }
  std::cerr.rdbuf(cerr_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "Hello, World!");
}

void testXMLLogging() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  JSBSim::Element el("element");
  el.SetFileName("name.xml");
  el.SetLineNumber(42);
  {
    JSBSim::FGXMLLogging log(&el, JSBSim::LogLevel::DEBUG);
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\nIn file name.xml: line 42\n");
}

void testMinLevel() {
  logger->SetMinLevel(JSBSim::LogLevel::DEBUG);
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::BULK);
    log << "BULK";
  }
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << "INFO";
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "INFO");
}

void testRedFormat() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::RED;
    log << "Hello, World!";
    log << JSBSim::LogFormat::RESET;
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\033[31mHello, World!\033[0m");
}

void testCyanFormat() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::BLUE;
    log << "Hello, World!";
    log << JSBSim::LogFormat::RESET;
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\033[34mHello, World!\033[0m");
}

void testBoldFormat() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::BOLD;
    log << "Hello, World!";
    log << JSBSim::LogFormat::RESET;
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\033[1mHello, World!\033[0m");
}

void testNormalFormat() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::NORMAL;
    log << "Hello, World!";
    log << JSBSim::LogFormat::RESET;
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\033[22mHello, World!\033[0m");
}

void testUnderlineFormat() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::UNDERLINE_ON;
    log << "Hello, World!";
    log << JSBSim::LogFormat::UNDERLINE_OFF;
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\033[4mHello, World!\033[24m");
}

void testDefaultFormat() {
  std::ostringstream buffer;
  auto cout_buffer = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());
  {
    JSBSim::FGLogging log(JSBSim::LogLevel::INFO);
    log << JSBSim::LogFormat::DEFAULT;
    log << "Hello, World!";
    log << JSBSim::LogFormat::RESET;
  }
  std::cout.rdbuf(cout_buffer);
  TS_ASSERT_EQUALS(buffer.str(), "\033[39mHello, World!\033[0m");
}
};

class LogExceptionTest : public CxxTest::TestSuite
{
public:
std::shared_ptr<DummyLogger> logger;

void setUp() {
  logger = std::make_shared<DummyLogger>();
  JSBSim::SetLogger(logger);
}

void testConstructor() {
  JSBSim::LogException logException;
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testDestructor() {
  {
    JSBSim::LogException logException;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testThrow() {
  try {
    JSBSim::LogException logException;
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testThrowEmptyMessage() {
  try {
    JSBSim::LogException logException;
    logException << "";
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testThrowWithMessage() {
  std::string message = "Hello, World!";
  try {
    JSBSim::LogException logException;
    logException << message;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testThrowConcatenatedMessages1() {
  std::string message1 = "Hello";
  std::string message2 = ", World!";
  try {
    JSBSim::LogException logException;
    logException << message1 << message2;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message1 + message2);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testThrowConcatenatedMessages2() {
  std::string message1 = "Hello";
  std::string message2 = ", World!";
  try {
    JSBSim::LogException logException;
    logException << message1;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    logException << message2;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message1 + message2);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testThrowFormattedMessages1() {
  std::string message = "Hello, World!";
  try {
    JSBSim::LogException logException;
    logException << JSBSim::LogFormat::NORMAL << message;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "NORMAL" + message);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testThrowFormattedMessages2() {
  std::string message = "Hello, World!";
  try {
    JSBSim::LogException logException;
    logException << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    logException << message;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "NORMAL" + message);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testThrowFormattedMessages3() {
  std::string message1 = "Hello";
  std::string message2 = ", World!";
  try {
    JSBSim::LogException logException;
    logException << message1 << JSBSim::LogFormat::NORMAL << message2;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (const JSBSim::BaseException& e) {
    TS_ASSERT_EQUALS(std::string(e.what()), message1 + message2);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message1 + "NORMAL" + message2);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testThrowAndAppendMessage() {
  std::string message1 = "Hello";
  std::string message2 = ", World!";
  try {
    JSBSim::LogException logException;
    logException << message1;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::LogException& e) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    e << message2;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message1 + message2);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}
};

class testXMLLogException : public CxxTest::TestSuite
{
public:
std::shared_ptr<DummyLogger> logger;

void setUp() {
  logger = std::make_shared<DummyLogger>();
  JSBSim::SetLogger(logger);
}

void testConstructor() {
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  JSBSim::XMLLogException logException(&el);
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testThrow() {
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  try {
    JSBSim::XMLLogException logException(&el);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testEmptyMessage() {
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  try {
    JSBSim::XMLLogException logException(&el);
    logException << "";
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
}

void testWithMessage() {
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  std::string message = "Hello, World!";
  try {
    JSBSim::XMLLogException logException(&el);
    logException << message;
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
    throw logException;
  } catch (JSBSim::BaseException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "file.xml:42" + message);
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}

void testPromoteLogException() {
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  try {
    try {
      JSBSim::LogException logException;
      logException << "Hello, World!";
      TS_ASSERT(!logger->flushed);
      TS_ASSERT(logger->buffer.empty());
      TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
      throw logException;
    } catch (JSBSim::LogException& e) {
      JSBSim::XMLLogException logException(e, &el);
      TS_ASSERT(!logger->flushed);
      TS_ASSERT(logger->buffer.empty());
      TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
      throw logException;
    }
  } catch (JSBSim::LogException&) {
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "file.xml:42Hello, World!");
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::FATAL);
}
};
