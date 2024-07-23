#pragma once

#include <set>
#include <fstream>

#include <FGFDMExec.h>
#include <math/FGFunction.h>
#include <math/FGParameter.h>
#include <math/FGTable.h>
#include <math/FGFunctionValue.h>
#include <input_output/FGScript.h>
#include <models/flight_control/FGFCSComponent.h>
#include <models/FGFCS.h>
#include <models/FGFCSChannel.h>
#include <models/FGAerodynamics.h>
#include <models/FGAircraft.h>

#include "gvpp.hpp"

class aerodynamics_2_graphviz
{
public :
  static std::string GetVersion();

  void graph_jsbsim_fdm_aerodynamics(
    std::shared_ptr<JSBSim::FGFDMExec> fdm
    , const std::string output_filename=""
    , bool show_table_png = false
  );
  void add_function_edges(int axis_index
    , gvpp::SubGraph<>* graph
    , const JSBSim::FGFunction* function
    , bool show_table_png = false
  );
  std::string get_node_name(const std::string name);
  std::string get_node_name_with_axis(int axis_index, const std::string name);
  bool test_if_name_is_common_node(const std::string name);
  void add_common_nodes_and_edges(gvpp::Graph<>* graph);
  void config_parameter_node_by_name(gvpp::Node<>* node, const std::string name);
  void graph_jsbsim_fdm_subgraphs();
  void set_imagepath(const std::string imagepath);
  void write_dot_file(gvpp::Graph<>& graph, const std::string file_name);
private:
  std::string imagepath;
};

