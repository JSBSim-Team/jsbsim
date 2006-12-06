#include "../simgear/xml/easyxml.hxx"
#include <string>
#include <vector>

struct Plots {
  string Title;
  string Axis_Caption[2];
  double Min[2], Max[2];
  bool Autoscale;
  vector <string> Y_Variables;
  string X_Variable;

  Plots(void) {
    Autoscale = false;
    Min[0] = Min[1] = 0.0;
    Max[0] = Max[1] = 0.0;
  }
};

enum Axis {unset=-1, eX=0, eY=1};

class plotXMLVisitor : public XMLVisitor
{
public:

  plotXMLVisitor(void);
  ~plotXMLVisitor(void);

  void startXML();
  void endXML();
  void startElement (const char * name, const XMLAttributes &atts);
  void endElement (const char * name);
  void data (const char * s, int length);
  void pi (const char * target, const char * data);
  void warning (const char * message, int line, int column);

  vector <struct Plots> vPlots;

private:
  bool first_element_read;
  string current_element;
//  const char* data_string;
  string data_string;
  Axis axis;
};
