#include "plotXMLVisitor.h"

using namespace std;

plotXMLVisitor::plotXMLVisitor(void)
{
  first_element_read = false;
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
  for (int i=0; i<atts.size();i++) {
    if (string(atts.getName(i)) == string("axis")) {
      if (string(atts.getValue(i)) == string("x")) axis = eX;
      else axis = eY;
    } else {
      cerr << "Unknown attribute " << atts.getName(i) << " encountered." << endl;
      exit (-1);
    }
    if (i == 1) {
      cerr << "Too many attributes. Offending attribute (item:" << i << ") is " << atts.getName(i) << endl;
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

  if (current_element == "plot") vPlots.push_back(Plots());
}

void plotXMLVisitor::endElement (const char * name)
{
  if (string(name) == string("title")) {
    vPlots.back().Title = data_string;
  } else if (string(name) == string("label")) {
    if (axis < 0) {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
    vPlots.back().Axis_Caption[axis] = data_string;
  } else if (string(name) == string("scale")) {
    if (data_string == "auto") vPlots.back().Autoscale = true;
  } else if (string(name) == string("min")) {
    if (axis < 0) {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
    vPlots.back().Min[axis] = atof(data_string.c_str());
  } else if (string(name) == string("max")) {
    if (axis < 0) {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
    vPlots.back().Max[axis] = atof(data_string.c_str());
  } else if (string(name) == string("parameter")) {
    if (axis == eX) {
      vPlots.back().X_Variable = data_string;
    } else if (axis == eY) {
      vPlots.back().Y_Variables.push_back(data_string);
    } else {
      cerr << "Axis not chosen." << endl;
      exit(-1);
    }
  } else if (string(name) == string("plotset")) {
//    cout << "End of plot set." << endl;
  } else if (string(name) == string("plot")) {
//    cout << endl << "Title: " << vPlots.back().Title << endl;
//    cout << "X Axis title: " << vPlots.back().Axis_Caption[eX] << endl;
//    cout << "Y Axis title: " << vPlots.back().Axis_Caption[eY] << endl;
//    cout << "Autoscale: " << vPlots.back().Autoscale << endl;
//    cout << "Minimum X axis value: " << vPlots.back().Min[eX] << endl;
//    cout << "Maximum X axis value: " << vPlots.back().Max[eX] << endl;
//    cout << "Minimum Y axis value: " << vPlots.back().Min[eY] << endl;
//    cout << "Maximum Y axis value: " << vPlots.back().Max[eY] << endl;
//    cout << "X Parameter: " << vPlots.back().X_Variable << endl;
//    cout << vPlots.back().Y_Variables.size() << " Y parameters:" << endl;
    for (int i=0; i<vPlots.back().Y_Variables.size(); i++) {
//      cout << vPlots.back().Y_Variables[i] << endl;
    }
  } else {
    cerr << "Unknown data element." << endl;
    exit(-1);
  }
}

void plotXMLVisitor::data (const char * s, int length)
{
//  data_string = s;
  const char *local_string = s;
  data_string = local_string;
  data_string.resize(length);
}

void plotXMLVisitor::pi (const char * target, const char * data)
{
}

void plotXMLVisitor::warning (const char * message, int line, int column)
{
}
