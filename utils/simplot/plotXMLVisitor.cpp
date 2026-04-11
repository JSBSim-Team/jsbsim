#include "plotXMLVisitor.h"
#define BASE
#include "input_output/string_utilities.h"

using namespace std;

plotXMLVisitor::plotXMLVisitor(void)
{
  first_element_read = false;
  inPage = false;
  axis = unset;
}

plotXMLVisitor::~plotXMLVisitor(void)
{
}

void plotXMLVisitor::startXML(void)
{
//  cout << "Starting XML parse ..." << endl;
}

void plotXMLVisitor::endXML(void)
{
//  cout << "Finished XML parse." << endl;
}

void plotXMLVisitor::startElement (const char * name, const XMLAttributes &atts)
{
  current_element = name;
  
  // defaults
  plotType = lines;

  for (int i=0; i<atts.size();i++) {
    string thisAttribute = atts.getName(i);
    string thisValue = atts.getValue(i);
    if (thisAttribute == string("axis")) {
      if (thisValue == string("x")) axis = eX;
      else if (thisValue == string("y2")) axis = eY2;
      else axis = eY;
    } else if (thisAttribute == string("type")) {
      if (thisValue == string("lines")) {
        plotType = lines;
      } else if (thisValue == string("points")) {
        plotType = points;
    } else {
        cerr << endl << "Plot type " << thisValue << " is not valid. Using lines type for default." << endl;
        plotType = lines;
      }
    } else {
      if (string(name) == "plotset") break;
      cerr << "Unknown attribute " << thisAttribute << " encountered in element, " << name << endl;
      exit (-1);
    }
    if (i == 1) {
      cerr << "Too many attributes. Offending attribute (item:" << i << ") is " << thisAttribute << endl;
      exit (-1);
    }
  }

  if (!first_element_read) {
    if (current_element != string("plotset")) {
      cerr << endl << "  This is not a valid plotset description (" << current_element << ")" << endl;
      exit (-1);
    } else {
      first_element_read = true;
    }
  }

  if (current_element == "page") {
    vPages.push_back(Page());
    inPage = true;
  } else if (current_element == "plot") {
    if (!inPage) {
      vPlots.push_back(Plots());
      vPlots.back().plotType = plotType;
    } else {
      vPages.back().vPlots.push_back(Plots());
      vPages.back().vPlots.back().plotType = plotType;
    }
  }
}

void plotXMLVisitor::endElement (const char * name)
{
  if (string(name) == string("title")) {                                               // Reading title
    if (!inPage)
      vPlots.back().Title = trim(data_string);
    else
      vPages.back().vPlots.back().Title = trim(data_string);

  } else if (string(name) == string("label")) {                                        // Axis captions

    if (axis < 0) {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
    if (!inPage)
      vPlots.back().Axis_Caption[axis] = trim(data_string);
    else
      vPages.back().vPlots.back().Axis_Caption[axis] = trim(data_string);

  } else if (string(name) == string("scale")) {                                        // Scaling

    if (!inPage)
      if (trim(data_string) == "auto") vPlots.back().Autoscale = true;
    else
      if (trim(data_string) == "auto") vPages.back().vPlots.back().Autoscale = true;    

  } else if (string(name) == string("min")) {                                          // Minimum

    if (axis < 0) {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
    if (!inPage) {
      vPlots.back().Min[axis] = data_string.c_str();
    } else {
      vPages.back().vPlots.back().Min[axis] = data_string.c_str();
    }

  } else if (string(name) == string("max")) {                                          // Maximum

    if (axis < 0) {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
    if (!inPage) {
      vPlots.back().Max[axis] = data_string.c_str();
    } else {
      vPages.back().vPlots.back().Max[axis] = data_string.c_str();
    }

  } else if (string(name) == string("parameter")) {                                    // Parameter

    if (axis == eX) {                                                                      //-> X axis
      if (!inPage)
        vPlots.back().X_Variable = trim(data_string);
      else
        vPages.back().vPlots.back().X_Variable = trim(data_string);
    } else if (axis == eY) {                                                               // -> Y axis
      if (!inPage)
        vPlots.back().Y_Variables.push_back(trim(data_string));
      else
        vPages.back().vPlots.back().Y_Variables.push_back(trim(data_string));
    } else if (axis == eY2) {                                                              // -> Y2 axis
      if (!inPage)
        vPlots.back().Y2_Variables.push_back(trim(data_string));
      else
        vPages.back().vPlots.back().Y2_Variables.push_back(trim(data_string));
    } else {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
  } else if (string(name) == string("plotset")) {
  } else if (string(name) == string("plot")) {
  } else if (string(name) == string("page")) {
    inPage = false;
  } else {
    cerr << "Unknown data element." << endl;
    exit(-1);
  }
}

void plotXMLVisitor::data (const char * s, int length)
{
//  const char *local_string = s;
  data_string = s;
  data_string.resize(length);
}

void plotXMLVisitor::pi (const char * target, const char * data)
{
}

void plotXMLVisitor::warning (const char * message, int line, int column)
{
}
