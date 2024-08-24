#include <cxxtest/TestSuite.h>
#include <input_output/FGLog.h>
#include <input_output/FGXMLElement.h>

class DummyLogger : public JSBSim::FGLogger
{
public:
  JSBSim::LogLevel GetLogLevel() const { return level; }
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
void testConstructor() {
  auto logger = std::make_shared<DummyLogger>();
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::BULK);

  JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
  TS_ASSERT(log.str().empty());
  TS_ASSERT(!logger->flushed);
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::INFO);
}

void testDestructor() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    TS_ASSERT(log.str().empty());
    TS_ASSERT(!logger->flushed);
  }
  TS_ASSERT(logger->buffer.empty());
  TS_ASSERT(logger->flushed);
}

void testCharMessage() {
  auto logger = std::make_shared<DummyLogger>();
  const char* message = "Hello, World!";
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log <<message;
    TS_ASSERT_EQUALS(log.str(), message);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message);
}

void testStringMessage() {
  auto logger = std::make_shared<DummyLogger>();
  std::string message = "Hello, World!";
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << message;
    TS_ASSERT_EQUALS(log.str(), message);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message);
}

void testConcatenatedMessages() {
  auto logger = std::make_shared<DummyLogger>();
  std::string message1 = "Hello";
  std::string message2 = "World!";
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << message1 << " " << message2;
    TS_ASSERT_EQUALS(log.str(), message1 + " " + message2);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, message1 + " " + message2);
}

void testEndl() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << "Hello" << std::endl << "World!";
    TS_ASSERT_EQUALS(log.str(), "Hello\nWorld!");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Hello\nWorld!");
}

void testNumbers() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << 1 << 2.1 << -3.4f;
    TS_ASSERT_EQUALS(log.str(), "12.1-3.4");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "12.1-3.4");
}

void testSetPrecision() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << std::setprecision(3) << 1.23456789;
    TS_ASSERT_EQUALS(log.str(), "1.23");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "1.23");
}

void testSetWidthRight() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << std::setw(5) << 123;
    TS_ASSERT_EQUALS(log.str(), "  123");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "  123");
}

void testSetWidthLeft() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << std::left << std::setw(5) << 123;
    TS_ASSERT_EQUALS(log.str(), "123  ");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "123  ");
}

void testPath() {
  auto logger = std::make_shared<DummyLogger>();
  SGPath path("path/to");
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << (path/"file");
    TS_ASSERT_EQUALS(log.str(), "Path \"path/to/file\"");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Path \"path/to/file\"");
}

void testColumnVector3() {
  auto logger = std::make_shared<DummyLogger>();
  JSBSim::FGColumnVector3 vec(1, 2, 3);
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << vec;
    TS_ASSERT_EQUALS(log.str(), "1 , 2 , 3");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "1 , 2 , 3");
}

void testFormatOnly() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    log << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(log.str().empty());
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "NORMAL");
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "NORMAL");
}

void testClosingFormat() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << "Hello,";
    TS_ASSERT_EQUALS(log.str(), "Hello,");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    log << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(log.str().empty());
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
}

void testMidFormat() {
  auto logger = std::make_shared<DummyLogger>();
  {
    JSBSim::FGLogging log(logger, JSBSim::LogLevel::INFO);
    log << "Hello,";
    TS_ASSERT_EQUALS(log.str(), "Hello,");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT(logger->buffer.empty());
    log << JSBSim::LogFormat::NORMAL;
    TS_ASSERT(log.str().empty());
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
    log << " World!";
    TS_ASSERT_EQUALS(log.str(), " World!");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL");
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "Hello,NORMAL World!");
}

void testXMLLogging() {
  auto logger = std::make_shared<DummyLogger>();
  JSBSim::Element el("element");
  el.SetFileName("file.xml");
  el.SetLineNumber(42);
  {
    JSBSim::FGXMLLogging log(logger, &el, JSBSim::LogLevel::DEBUG);
    TS_ASSERT(log.str().empty());
    TS_ASSERT_EQUALS(logger->buffer, "file.xml:42");
    TS_ASSERT(!logger->flushed);
    TS_ASSERT_EQUALS(logger->GetLogLevel(), JSBSim::LogLevel::DEBUG);
  }
  TS_ASSERT(logger->flushed);
  TS_ASSERT_EQUALS(logger->buffer, "file.xml:42");
}
};
