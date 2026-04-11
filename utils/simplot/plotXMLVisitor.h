#include "../simgear/xml/easyxml.hxx"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

enum ePlotType {lines=0, points};

using std::string;
using std::vector;

struct Plots {
  string Title;
  string Axis_Caption[3];
  string Min[3], Max[3];
  bool Autoscale;
  vector <string> Y_Variables;
  vector <string> Y2_Variables;
  string X_Variable;
  ePlotType plotType; // should eventually be on a per-parameter basis?

  Plots(void) {
    Autoscale = true;
    Min[0] = Min[1] = Min[2] = "*";
    Max[0] = Max[1] = Max[2] = "*";
    Title="";
    Axis_Caption[0] = Axis_Caption[1] = Axis_Caption[2] = "";
    plotType = lines;
  }
};

struct Page {
  vector <struct Plots> vPlots;
};

enum Axis {unset=-1, eX=0, eY, eY2};

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
  vector <struct Page> vPages;
  
  bool inPage;

private:
  bool first_element_read;
  string current_element;
//  const char* data_string;
  string data_string;
  Axis axis;
  ePlotType plotType;
};
