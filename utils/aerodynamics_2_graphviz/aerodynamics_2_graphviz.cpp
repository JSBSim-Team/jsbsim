#include "aerodynamics_2_graphviz.h"

std::set<std::string> jsbsim_parameters_without_axis;
gvpp::SubGraph<>* jsbsim_subgraphs[7];


const string version = "0.1" __DATE__ " " __TIME__;

std::string aerodynamics_2_graphviz::GetVersion()
{
    return version;
}

void aerodynamics_2_graphviz::graph_jsbsim_fdm_aerodynamics(std::shared_ptr<JSBSim::FGFDMExec> fdm
  , const std::string output_filename
  , bool show_table_png
)
{
  auto model_name = fdm->GetModelName();
  if (model_name.size() == 0) {
    cerr << "Model NOT Loaded, exit." << endl;
    return;
  }

  auto aero_dynamics = fdm->GetAerodynamics();
  auto aero_functions = aero_dynamics->GetAeroFunctions();

  gvpp::Graph<> all_axis_graph(true, model_name);
  all_axis_graph.set(gvpp::AttrType::GRAPH, "nodesep", "0.1");
  all_axis_graph.set(gvpp::AttrType::GRAPH, "rankdir", "LR");
  if (imagepath.size() > 0) {
    all_axis_graph.set(gvpp::AttrType::GRAPH, "imagepath", imagepath);
  }

  for (unsigned int axis_index = 0; axis_index < 6; axis_index++) {
    gvpp::Graph<> axis_graph(true, model_name);
    axis_graph.set(gvpp::AttrType::GRAPH, "nodesep", "0.1");
    axis_graph.set(gvpp::AttrType::GRAPH, "rankdir", "LR");
    if (imagepath.size() > 0) {
      axis_graph.set(gvpp::AttrType::GRAPH, "imagepath", imagepath);
    }

    auto axis_functions = &aero_functions[axis_index];
    auto subgraph_name = std::to_string(axis_index);
    auto& all_axis_subgraph = all_axis_graph.addSubGraph(subgraph_name, true, subgraph_name);
    auto& axis_subgraph = axis_graph.addSubGraph(subgraph_name, true, subgraph_name);
    jsbsim_subgraphs[axis_index] = &all_axis_subgraph;
    for (auto function : *axis_functions) {
      add_function_edges(axis_index, &all_axis_subgraph, function,show_table_png);

      add_function_edges(axis_index, &axis_subgraph, function,show_table_png);

    }
    
    std::string axis_file_name = "";
    if (output_filename.size() > 0) {
      axis_file_name = output_filename + "_" + std::to_string(axis_index) + ".dot";
    }
    else {
      axis_file_name = model_name + "_" + std::to_string(axis_index) + ".dot";
    }
    write_dot_file(axis_graph, axis_file_name);
    //gvpp::renderToFile(axis_graph, "dot","png", axis_file_name);
  }

  add_common_nodes_and_edges(&all_axis_graph);

  std::string all_axis_file_name = "";
  if (output_filename.size() > 0) {
    all_axis_file_name =  output_filename + ".dot";
  }
  else {
    all_axis_file_name = model_name + ".dot";
  }
  write_dot_file(all_axis_graph, all_axis_file_name);
  //gvpp::renderToFile(all_axis_graph, "dot","png", all_axis_file_name);

}

void aerodynamics_2_graphviz::add_function_edges(int axis_index
  , gvpp::SubGraph<>* graph
  , const JSBSim::FGFunction* function
  , bool show_table_png
)
{
  if (function == NULL) {
    return;
  }

  auto function_desc = function->GetParameterDescription();
  auto function_name = function->GetParameterName();
  auto function_operation = function->GetFunctionOperation();

  auto function_node_name = get_node_name_with_axis(axis_index, function_name);
  if (!graph->hasNode(function_node_name)) {
    auto& add_function_node = graph->addNode(function_node_name, function_desc);

    if (function_operation == "property") {
      add_function_node.set("shape", "point");
      add_function_node.set("color", "blue");
    }

    if (function_operation == "product") {
      add_function_node.set("shape", "box");
      add_function_node.set("color", "gold");
    }
  }

  auto& function_node = graph->getNode(function_node_name);

  auto parameters = function->GetParameters();
  auto parameters_count = parameters.size();
  if (parameters_count == 1) {

  }

  unsigned int parameter_index = 0;
  for (auto parameter : parameters) {
    parameter_index++;
    auto parameter_name = parameter->GetParameterName();
    auto parameter_desc = parameter->GetParameterDescription();
    auto parameter_node_name = get_node_name_with_axis(axis_index, parameter_name);

    if (!graph->hasNode(parameter_node_name)) {
      auto& add_parameter_node = graph->addNode(parameter_node_name, parameter_desc);

      config_parameter_node_by_name(&add_parameter_node, parameter_name);

      if (parameter_name._Starts_with("product_table_")) {
        auto parameter_2_table = dynamic_cast<const JSBSim::FGTable*>(parameter.ptr());
        if (parameter_2_table != NULL) {
          auto table_type = parameter_2_table->GetType();
          switch (table_type) {
          case JSBSim::FGTable::tt1D:
            add_parameter_node.set("style", "filled");
            add_parameter_node.set("shape", "polygon");
            add_parameter_node.set("sides", "6");
            add_parameter_node.set("fillcolor", "red");
            break;
          case JSBSim::FGTable::tt2D:
            add_parameter_node.set("style", "filled");
            add_parameter_node.set("shape", "doubleoctagon");
            add_parameter_node.set("fillcolor", "yellow");
            break;
          case JSBSim::FGTable::tt3D:
            add_parameter_node.set("style", "filled");
            add_parameter_node.set("shape", "tripleoctagon");
            add_parameter_node.set("fillcolor", "green");
            break;
          }

          if (show_table_png) {
            auto image_name = "\"" +parameter_2_table->GetName() + ".png\"";
            if (image_name.size() > 0) {
              add_parameter_node.set("image",image_name);
            }
          }
        }
      }

      auto parameter_2_functionvalue = dynamic_cast<const JSBSim::FGFunctionValue*>(parameter.ptr());
      if (parameter_2_functionvalue != NULL) {
        add_parameter_node.set("shape", "point");
        add_parameter_node.set("color", "blue");
      }
    }

    auto& parameter_node = graph->getNode(parameter_node_name);
    auto edge_label = std::to_string(parameter_index) + ":" + parameter_name;
    auto& add_edge = graph->addEdge(function_node, parameter_node, edge_label);

    auto parameter_2_function = dynamic_cast<const JSBSim::FGFunction*>(parameter.ptr());
    if (parameter_2_function != NULL) {
      add_function_edges(axis_index, graph, parameter_2_function, show_table_png);
    }
  }
}

std::string aerodynamics_2_graphviz::get_node_name(const std::string name)
{
  std::string node_name = "\"" + name + "\"";
  return node_name;
}

std::string aerodynamics_2_graphviz::get_node_name_with_axis(int axis_index, const std::string name)
{
  std::string node_name = "\"" + name + "_axis_" + std::to_string(axis_index) + "\"";

  if (auto search = jsbsim_parameters_without_axis.find(name); search != jsbsim_parameters_without_axis.end()) {
  }
  else {
    auto is_common_node = test_if_name_is_common_node(name);
    if (is_common_node) {
      jsbsim_parameters_without_axis.insert(name);
    }
  }

  return node_name;
}

bool aerodynamics_2_graphviz::test_if_name_is_common_node(const std::string name)
{
  if (name._Starts_with("function")) {
    return false;
  }

  if (name._Starts_with("product_table")) {
    return false;
  }

  if (name._Starts_with("constant value")) {
    return false;
  }

  if (name._Starts_with("axis_")) {
    return false;
  }

  return true;
}

void aerodynamics_2_graphviz::add_common_nodes_and_edges(gvpp::Graph<>* graph)
{
  auto& common_subgraph = graph->addSubGraph("common", true, "common");
  jsbsim_subgraphs[6] = &common_subgraph;

  for (auto& name : jsbsim_parameters_without_axis) {
    auto node_name = get_node_name(name);
    auto& add_node = common_subgraph.addNode(node_name);
    config_parameter_node_by_name(&add_node, name);

    for (unsigned int axis_index = 0; axis_index < 6; axis_index++) {
      auto subgraph_name = std::to_string(axis_index);
      auto subgraph = jsbsim_subgraphs[axis_index];
      auto name_with_axis = get_node_name_with_axis(axis_index, name);
      if (subgraph->hasNode(name_with_axis)) {
        auto& axis_node = subgraph->getNode(name_with_axis);
        auto& add_edge = graph->addEdge(axis_node, add_node);
        add_edge.set("splines", "curved");
      }
    }
  }
}

void aerodynamics_2_graphviz::config_parameter_node_by_name(gvpp::Node<>* node, const std::string name)
{
  if (name._Starts_with("fcs")) {
    node->set("style", "filled");
    node->set("fillcolor", "blue");
    node->set("shape", "polygon");
    node->set("sides", "5");
    node->set("skew", "-0.5");
    node->set("distortion", "0");
    node->set("height", "2.0");
  }

  if (name._Starts_with("metrics")) {
    node->set("style", "filled");
    node->set("fillcolor", "yellow");
    node->set("shape", "polygon");
    node->set("sides", "6");
    node->set("skew", "-0.5");
    node->set("distortion", "0");
    node->set("height", "2.0");
  }

  if (name._Starts_with("aero")) {
    node->set("style", "filled");
    node->set("fillcolor", "green");
    node->set("shape", "polygon");
    node->set("sides", "7");
    node->set("skew", "-0.5");
    node->set("distortion", "0");
    node->set("height", "2.0");
  }

  if (name._Starts_with("constant value")) {
    node->set("style", "filled");
    node->set("fillcolor", "red");
    node->set("shape", "egg");
  }

  if (name._Starts_with("velocities")) {
    node->set("style", "filled");
    node->set("fillcolor", "olivedrab");
    node->set("shape", "tab");
  }
}

//#TODO: gen subgraphs for each axis
void aerodynamics_2_graphviz::graph_jsbsim_fdm_subgraphs()
{
}

void aerodynamics_2_graphviz::set_imagepath(const std::string imagepath)
{
  this->imagepath = "\"" + imagepath + "\"";
}

void aerodynamics_2_graphviz::write_dot_file(gvpp::Graph<>& graph, const std::string file_name)
{
  std::fstream file;
  file.open(file_name, std::ios::out); // 以输出模式打开文件

  if (!file) {
    std::cerr << "Unable to open file!" << std::endl;
  }

  file << graph;
  file.flush();
  file.close(); // 关闭文件
}
