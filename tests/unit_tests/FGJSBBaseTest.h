#include <string>
#include <limits>
#include <cxxtest/TestSuite.h>
#include <FGJSBBase.h>

class FGJSBBaseTest : public CxxTest::TestSuite, public JSBSim::FGJSBBase
{
public:
  void testMessages() {
    const std::string myMessage = "My message";
    std::set<int> mesId;
    // Check that the message queue is empty
    TS_ASSERT(!SomeMessages());
    TS_ASSERT(!ProcessNextMessage());
    ProcessMessage();
    // Process one text message
    PutMessage(myMessage);
    TS_ASSERT(SomeMessages());
    Message* message = ProcessNextMessage();
    TS_ASSERT(message);
    mesId.insert(message->messageId);
    TS_ASSERT_EQUALS(message->type, Message::eText);
    TS_ASSERT_EQUALS(message->text, myMessage);
    TS_ASSERT(!SomeMessages());
    // Check that the message queue is empty again
    TS_ASSERT(!SomeMessages());
    TS_ASSERT(!ProcessNextMessage());
    ProcessMessage();
    // Process several messages
    PutMessage(CreateIndexedPropertyName(myMessage,0), true);
    PutMessage(CreateIndexedPropertyName(myMessage,1), -1);
    PutMessage(CreateIndexedPropertyName(myMessage,2), 3.14159);
    for (int i=0; i<3; i++) {
      std::ostringstream os;
      TS_ASSERT(SomeMessages());
      message = ProcessNextMessage();
      TS_ASSERT(message);
      // Check the ID is a unique number
      TS_ASSERT(!mesId.count(message->messageId));
      mesId.insert(message->messageId);
      os << myMessage << "[" << i << "]";
      TS_ASSERT_EQUALS(message->text, os.str());
      switch(i) {
      case 0:
        TS_ASSERT_EQUALS(message->type, Message::eBool);
        TS_ASSERT(message->bVal);
        break;
      case 1:
        TS_ASSERT_EQUALS(message->type, Message::eInteger);
        TS_ASSERT_EQUALS(message->iVal, -1);
        break;
      case 2:
        TS_ASSERT_EQUALS(message->type, Message::eDouble);
        TS_ASSERT_EQUALS(message->dVal, 3.14159);
        break;
      }
    }
    // Check that the message queue is empty again
    TS_ASSERT(!SomeMessages());
    TS_ASSERT(!ProcessNextMessage());
    ProcessMessage();
    // Re-insert the last message in the queue
    Message backup = *message;
    PutMessage(*message);
    TS_ASSERT(SomeMessages());
    message = ProcessNextMessage();
    TS_ASSERT(message);
    TS_ASSERT_EQUALS(message->text, backup.text);
    // Check that the message ID has not been altered
    TS_ASSERT_EQUALS(message->messageId, backup.messageId);
    TS_ASSERT(mesId.count(message->messageId));
    TS_ASSERT_EQUALS(message->subsystem, backup.subsystem);
    TS_ASSERT_EQUALS(message->type, Message::eDouble);
    TS_ASSERT_EQUALS(message->dVal, 3.14159);
    // Check that the message queue is empty again
    TS_ASSERT(!SomeMessages());
    TS_ASSERT(!ProcessNextMessage());
    ProcessMessage();
    // Process several messages
    PutMessage(myMessage);
    PutMessage(CreateIndexedPropertyName(myMessage,0), true);
    PutMessage(CreateIndexedPropertyName(myMessage,1), -1);
    PutMessage(CreateIndexedPropertyName(myMessage,2), 3.14159);
    // Ugly hack to generate a malformed message
    unsigned char* _type = (unsigned char*)(&backup.type);
    *_type = 0xff;
    PutMessage(backup);
    TS_ASSERT(SomeMessages());
    ProcessMessage();
    // Check that the message queue is empty again
    TS_ASSERT(!SomeMessages());
    TS_ASSERT(!ProcessNextMessage());
    ProcessMessage();
  }

  void testCASConversion() {
    double p = 2116.228;
    TS_ASSERT_EQUALS(VcalibratedFromMach(-0.1, p), 0.0);
    TS_ASSERT_EQUALS(VcalibratedFromMach(0, p), 0.0);
    TS_ASSERT_DELTA(VcalibratedFromMach(0.5, p), 558.2243, 1E-4);
    TS_ASSERT_DELTA(VcalibratedFromMach(1.0, p), 1116.4486, 1E-4);
    TS_ASSERT_DELTA(VcalibratedFromMach(1.5, p), 1674.6728, 1E-4);
    TS_ASSERT_EQUALS(MachFromVcalibrated(0.0, p), 0.0);
    TS_ASSERT_DELTA(MachFromVcalibrated(558.2243, p), 0.5, 1E-4);
    TS_ASSERT_DELTA(MachFromVcalibrated(1116.4486, p), 1.0, 1E-4);
    TS_ASSERT_DELTA(MachFromVcalibrated(1674.6728, p), 1.5, 1E-4);
  }

  void testNumericRoutines() {
    double dx = 1.0;
    float  fx = 1.0;
    double dy = dx + std::numeric_limits<double>::epsilon();
    float  fy = fx + std::numeric_limits<float>::epsilon();
    TS_ASSERT(EqualToRoundoff(dx, dy));
    TS_ASSERT(EqualToRoundoff(dx, fy));
    TS_ASSERT(EqualToRoundoff(fx, fy));
    TS_ASSERT(EqualToRoundoff(fx, dy));
    TS_ASSERT_EQUALS(sign(1.235), 1.0);
    TS_ASSERT_EQUALS(sign(0.0), 1.0);
    TS_ASSERT_EQUALS(sign(-1E-5), -1.0);
    TS_ASSERT_EQUALS(Constrain(0.0,-1E-5,1.0), 0.0);
    TS_ASSERT_EQUALS(Constrain(0.0,  0.5,1.0), 0.5);
    TS_ASSERT_EQUALS(Constrain(0.0, 10.0,1.0), 1.0);
    Filter f0;
    Filter f(1.0, 1E-5);
    double x = f.execute(3.0);
    // Called twice for 100% coverage
    // Need to test that the numbers follow a Gaussian law ?
    double ran0 = GaussianRandomNumber();
    double ran1 = GaussianRandomNumber();
  }

  void testTemperatureConversion() {
    TS_ASSERT(EqualToRoundoff(KelvinToFahrenheit(0.0), -459.4));
    TS_ASSERT(EqualToRoundoff(KelvinToFahrenheit(288.15), 59.27));
    TS_ASSERT(EqualToRoundoff(CelsiusToRankine(0.0), 491.67));
    TS_ASSERT(EqualToRoundoff(CelsiusToRankine(15.0), 518.67));
    TS_ASSERT(EqualToRoundoff(RankineToCelsius(491.67), 0.0));
    TS_ASSERT_DELTA(RankineToCelsius(518.67), 15.0, 1E-8);
    TS_ASSERT(EqualToRoundoff(KelvinToRankine(0.0), 0.0));
    TS_ASSERT(EqualToRoundoff(KelvinToRankine(288.15), 518.67));
    TS_ASSERT(EqualToRoundoff(RankineToKelvin(0.0), 0.0));
    TS_ASSERT_DELTA(RankineToKelvin(518.67), 288.15, 1E-8);
    TS_ASSERT(EqualToRoundoff(CelsiusToFahrenheit(0.0), 32.0));
    TS_ASSERT(EqualToRoundoff(CelsiusToFahrenheit(15.0), 59.0));
    TS_ASSERT(EqualToRoundoff(FahrenheitToCelsius(32.0), 0.0));
    TS_ASSERT_DELTA(FahrenheitToCelsius(59.0), 15.0, 1E-8);
    TS_ASSERT(EqualToRoundoff(KelvinToCelsius(0.0), -273.15));
    TS_ASSERT(EqualToRoundoff(KelvinToCelsius(288.15), 15.0));
    TS_ASSERT(EqualToRoundoff(CelsiusToKelvin(-273.15), 0.0));
    TS_ASSERT(EqualToRoundoff(CelsiusToKelvin(15.0), 288.15));
  }

  void testMisc() {
    std::string version = GetVersion();
    disableHighLighting();
  }
};
