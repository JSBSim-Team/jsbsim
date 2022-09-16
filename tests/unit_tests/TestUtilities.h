#include <input_output/FGXMLParse.h>

JSBSim::Element_ptr readFromXML(const std::string& XML) {
  std::istringstream data(XML);
  JSBSim::FGXMLParse parser;
  readXML(data, parser);
  return parser.GetDocument();
}
