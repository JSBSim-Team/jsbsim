// props.cxx - implementation of a property list.
// Started Fall 2000 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// See props.html for documentation [replace with URL when available].
//
// $Id: props.cxx,v 1.1 2002/03/09 12:01:06 apeden Exp $

#include <simgear/compiler.h>

#include <stdio.h>
#include <stdlib.h>
#include STL_IOSTREAM
#include <algorithm>
#include "props.hxx"

SG_USING_STD(sort);



////////////////////////////////////////////////////////////////////////
// Local classes.
////////////////////////////////////////////////////////////////////////

/**
 * Comparator class for sorting by index.
 */
class CompareIndices
{
public:
  int operator() (const SGPropertyNode * n1, const SGPropertyNode *n2) const {
    return (n1->getIndex() < n2->getIndex());
  }
};



////////////////////////////////////////////////////////////////////////
// Convenience macros for value access.
////////////////////////////////////////////////////////////////////////

#define TEST_READ(dflt) if (!getAttribute(READ)) return dflt
#define TEST_WRITE if (!getAttribute(WRITE)) return false

#define DO_TRACE_READ(type) if(getAttribute(TRACE_READ)) trace_read(type)
#define DO_TRACE_WRITE(type) if (getAttribute(TRACE_WRITE)) trace_write(type)

#define GET_BOOL (_value.bool_val->getValue())
#define GET_INT (_value.int_val->getValue())
#define GET_LONG (_value.long_val->getValue())
#define GET_FLOAT (_value.float_val->getValue())
#define GET_DOUBLE (_value.double_val->getValue())
#define GET_STRING (_value.string_val->getValue())

#define SET_BOOL(val) (_value.bool_val->setValue(val))
#define SET_INT(val) (_value.int_val->setValue(val))
#define SET_LONG(val) (_value.long_val->setValue(val))
#define SET_FLOAT(val) (_value.float_val->setValue(val))
#define SET_DOUBLE(val) (_value.double_val->setValue(val))
#define SET_STRING(val) (_value.string_val->setValue(val))



////////////////////////////////////////////////////////////////////////
// Default values for every type.
////////////////////////////////////////////////////////////////////////

const bool SGRawValue<bool>::DefaultValue = false;
const int SGRawValue<int>::DefaultValue = 0;
const long SGRawValue<long>::DefaultValue = 0L;
const float SGRawValue<float>::DefaultValue = 0.0;
const double SGRawValue<double>::DefaultValue = 0.0L;
const string SGRawValue<string>::DefaultValue = "";



////////////////////////////////////////////////////////////////////////
// Local path normalization code.
////////////////////////////////////////////////////////////////////////

/**
 * A component in a path.
 */
struct PathComponent
{
  string name;
  int index;
};

/**
 * Parse the name for a path component.
 *
 * Name: [_a-zA-Z][-._a-zA-Z0-9]*
 */
static inline string
parse_name (const string &path, int &i)
{
  string name = "";
  int max = path.size();

  if (path[i] == '.') {
    i++;
    if (i < max && path[i] == '.') {
      i++;
      name = "..";
    } else {
      name = ".";
    }
    if (i < max && path[i] != '/')
      throw string(string("Illegal character after ") + name);
  }

  else if (isalpha(path[i]) || path[i] == '_') {
    name += path[i];
    i++;

				// The rules inside a name are a little
				// less restrictive.
    while (i < max) {
      if (isalpha(path[i]) || isdigit(path[i]) || path[i] == '_' ||
	  path[i] == '-' || path[i] == '.') {
	name += path[i];
      } else if (path[i] == '[' || path[i] == '/') {
	break;
      } else {
	throw string("name may contain only ._- and alphanumeric characters");
      }
      i++;
    }
  }

  else {
    if (name.size() == 0)
      throw string("name must begin with alpha or '_'");
  }

  return name;
}


/**
 * Parse the optional integer index for a path component.
 *
 * Index: "[" [0-9]+ "]"
 */
static inline int
parse_index (const string &path, int &i)
{
  int index = 0;

  if (path[i] != '[')
    return 0;
  else
    i++;

  for (int max = path.size(); i < max; i++) {
    if (isdigit(path[i])) {
      index = (index * 10) + (path[i] - '0');
    } else if (path[i] == ']') {
      i++;
      return index;
    } else {
      break;
    }
  }

  throw string("unterminated index (looking for ']')");
}


/**
 * Parse a single path component.
 *
 * Component: Name Index?
 */
static inline PathComponent
parse_component (const string &path, int &i)
{
  PathComponent component;
  component.name = parse_name(path, i);
  if (component.name[0] != '.')
    component.index = parse_index(path, i);
  else
    component.index = -1;
  return component;
}


/**
 * Parse a path into its components.
 */
static void
parse_path (const string &path, vector<PathComponent> &components)
{
  int pos = 0;
  int max = path.size();

				// Check for initial '/'
  if (path[pos] == '/') {
    PathComponent root;
    root.name = "";
    root.index = -1;
    components.push_back(root);
    pos++;
    while (pos < max && path[pos] == '/')
      pos++;
  }

  while (pos < max) {
    components.push_back(parse_component(path, pos));
    while (pos < max && path[pos] == '/')
      pos++;
  }
}



////////////////////////////////////////////////////////////////////////
// Other static utility functions.
////////////////////////////////////////////////////////////////////////


/**
 * Locate a child node by name and index.
 */
static int
find_child (const string &name, int index, vector<SGPropertyNode *> nodes)
{
  int nNodes = nodes.size();
  for (int i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];
    if (node->getName() == name && node->getIndex() == index)
      return i;
  }
  return -1;
}


/**
 * Locate another node, given a relative path.
 */
static SGPropertyNode *
find_node (SGPropertyNode * current,
	   const vector<PathComponent> &components,
	   int position,
	   bool create)
{
				// Run off the end of the list
  if (current == 0) {
    return 0;
  }

				// Success! This is the one we want.
  else if (position >= (int)components.size()) {
    return current;
  }

				// Empty component means root.
  else if (components[position].name == "") {
    return find_node(current->getRootNode(), components, position + 1, create);
  }

				// . means current directory
  else if (components[position].name == ".") {
    return find_node(current, components, position + 1, create);
  }

				// .. means parent directory
  else if (components[position].name == "..") {
    SGPropertyNode * parent = current->getParent();
    if (parent == 0)
      throw string("Attempt to move past root with '..'");
    else
      return find_node(parent, components, position + 1, create);
  }

				// Otherwise, a child name
  else {
    SGPropertyNode * child =
      current->getChild(components[position].name,
			components[position].index,
			create);
    return find_node(child, components, position + 1, create);
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGPropertyNode.
////////////////////////////////////////////////////////////////////////


/**
 * Default constructor: always creates a root node.
 */
SGPropertyNode::SGPropertyNode ()
  : _name(""),
    _index(0),
    _parent(0),
    _path_cache(0),
    _type(NONE),
    _tied(false),
    _attr(READ|WRITE)
{
}


/**
 * Copy constructor.
 */
SGPropertyNode::SGPropertyNode (const SGPropertyNode &node)
  : _name(node._name),
    _index(node._index),
    _parent(0),			// don't copy the parent
    _path_cache(0),
    _type(node._type),
    _tied(node._tied),
    _attr(node._attr)
{
  switch (_type) {
  case NONE:
    break;
  case ALIAS:
    _value.alias = node._value.alias;
    break;
  case BOOL:
    _value.bool_val = node._value.bool_val->clone();
    break;
  case INT:
    _value.int_val = node._value.int_val->clone();
    break;
  case LONG:
    _value.long_val = node._value.long_val->clone();
    break;
  case FLOAT:
    _value.float_val = node._value.float_val->clone();
    break;
  case DOUBLE:
    _value.double_val = node._value.double_val->clone();
    break;
  case STRING:
  case UNSPECIFIED:
    _value.string_val = node._value.string_val->clone();
    break;
  }
}


/**
 * Convenience constructor.
 */
SGPropertyNode::SGPropertyNode (const string &name,
				int index, SGPropertyNode * parent)
  : _name(name),
    _index(index),
    _parent(parent),
    _path_cache(0),
    _type(NONE),
    _tied(false),
    _attr(READ|WRITE)
{
}


/**
 * Destructor.
 */
SGPropertyNode::~SGPropertyNode ()
{
  for (int i = 0; i < (int)_children.size(); i++) {
    delete _children[i];
  }
  delete _path_cache;
  clear_value();
}


/**
 * Delete and clear the current value.
 */
void
SGPropertyNode::clear_value ()
{
  switch (_type) {
  case NONE:
  case ALIAS:
    _value.alias = 0;
    break;
  case BOOL:
    delete _value.bool_val;
    _value.bool_val = 0;
    break;
  case INT:
    delete _value.int_val;
    _value.int_val = 0;
    break;
  case LONG:
    delete _value.long_val;
    _value.long_val = 0L;
    break;
  case FLOAT:
    delete _value.float_val;
    _value.float_val = 0;
    break;
  case DOUBLE:
    delete _value.double_val;
    _value.double_val = 0;
    break;
  case STRING:
  case UNSPECIFIED:
    delete _value.string_val;
    _value.string_val = 0;
    break;
  }
  _type = NONE;
}


/**
 * Get the value as a string.
 */
string
SGPropertyNode::get_string () const
{
  TEST_READ("");
  char buf[128];

  switch (_type) {
  case ALIAS:
    return _value.alias->getStringValue();
  case BOOL:
    if (GET_BOOL)
      return "true";
    else
      return "false";
  case INT:
    sprintf(buf, "%d", GET_INT);
    return buf;
  case LONG:
    sprintf(buf, "%ld", GET_LONG);
    return buf;
  case FLOAT:
    sprintf(buf, "%f", GET_FLOAT);
    return buf;
  case DOUBLE:
    sprintf(buf, "%f", GET_DOUBLE);
    return buf;
  case STRING:
  case UNSPECIFIED:
    return GET_STRING;
  case NONE:
  default:
    return "";
  }
}


/**
 * Trace a read access for a property.
 */
void
SGPropertyNode::trace_read (SGPropertyNode::Type accessType) const
{
  SG_LOG(SG_GENERAL, SG_INFO, "TRACE: Read node " << getPath()
	 << ", value \"" << get_string() << '"');
}


/**
 * Trace a write access for a property.
 */
void
SGPropertyNode::trace_write (SGPropertyNode::Type accessType) const
{
  SG_LOG(SG_GENERAL, SG_INFO, "TRACE: Write node " << getPath()
	 << ", value\"" << get_string() << '"');
}


/**
 * Alias to another node.
 */
bool
SGPropertyNode::alias (SGPropertyNode * target)
{
  if (target == 0 || _type == ALIAS || _tied)
    return false;
  clear_value();
  _value.alias = target;
  _type = ALIAS;
  return true;
}


/**
 * Alias to another node by path.
 */
bool
SGPropertyNode::alias (const string &path)
{
  return alias(getNode(path, true));
}


/**
 * Remove an alias.
 */
bool
SGPropertyNode::unalias ()
{
  if (_type != ALIAS)
    return false;
  _type = NONE;
  _value.alias = 0;
  return true;
}


/**
 * Get the target of an alias.
 */
SGPropertyNode *
SGPropertyNode::getAliasTarget ()
{
  return (_type == ALIAS ? _value.alias : 0);
}


const SGPropertyNode *
SGPropertyNode::getAliasTarget () const
{
  return (_type == ALIAS ? _value.alias : 0);
}


/**
 * Get a non-const child by index.
 */
SGPropertyNode *
SGPropertyNode::getChild (int position)
{
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}


/**
 * Get a const child by index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (int position) const
{
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}


/**
 * Get a non-const child by name and index, creating if necessary.
 */
SGPropertyNode *
SGPropertyNode::getChild (const string &name, int index, bool create)
{
  int pos = find_child(name, index, _children);
  if (pos >= 0) {
    return _children[pos];
  } else if (create) {
    _children.push_back(new SGPropertyNode(name, index, this));
    return _children[_children.size()-1];
  } else {
    return 0;
  }
}


/**
 * Get a const child by name and index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (const string &name, int index) const
{
  int pos = find_child(name, index, _children);
  if (pos >= 0)
    return _children[pos];
  else
    return 0;
}


/**
 * Get all children with the same name (but different indices).
 */
vector<SGPropertyNode *>
SGPropertyNode::getChildren (const string &name)
{
  vector<SGPropertyNode *> children;
  int max = _children.size();

  for (int i = 0; i < max; i++)
    if (_children[i]->getName() == name)
      children.push_back(_children[i]);

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


/**
 * Get all children const with the same name (but different indices).
 */
vector<const SGPropertyNode *>
SGPropertyNode::getChildren (const string &name) const
{
  vector<const SGPropertyNode *> children;
  int max = _children.size();

  for (int i = 0; i < max; i++)
    if (_children[i]->getName() == name)
      children.push_back(_children[i]);

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


string
SGPropertyNode::getPath (bool simplify) const
{
  if (_parent == 0)
    return "";

  string path = _parent->getPath(simplify);
  path += '/';
  path += _name;
  if (_index != 0 || !simplify) {
    char buffer[128];
    sprintf(buffer, "[%d]", _index);
    path += buffer;
  }
  return path;
}

SGPropertyNode::Type
SGPropertyNode::getType () const
{
  if (_type == ALIAS)
    return _value.alias->getType();
  else
    return _type;
}


bool 
SGPropertyNode::getBoolValue () const
{
  DO_TRACE_READ(BOOL);
  TEST_READ(false);
  switch (_type) {
  case ALIAS:
    return _value.alias->getBoolValue();
  case BOOL:
    return GET_BOOL;
  case INT:
    return GET_INT == 0 ? false : true;
  case LONG:
    return GET_LONG == 0L ? false : true;
  case FLOAT:
    return GET_FLOAT == 0.0 ? false : true;
  case DOUBLE:
    return GET_DOUBLE == 0.0L ? false : true;
  case STRING:
  case UNSPECIFIED:
    return (GET_STRING == "true" || getDoubleValue() != 0.0L);
  case NONE:
  default:
    return false;
  }
}

int 
SGPropertyNode::getIntValue () const
{
  DO_TRACE_READ(INT);
  TEST_READ(0);
  switch (_type) {
  case ALIAS:
    return _value.alias->getIntValue();
  case BOOL:
    return int(GET_BOOL);
  case INT:
    return GET_INT;
  case LONG:
    return int(GET_LONG);
  case FLOAT:
    return int(GET_FLOAT);
  case DOUBLE:
    return int(GET_DOUBLE);
  case STRING:
  case UNSPECIFIED:
    return atoi(GET_STRING.c_str());
  case NONE:
  default:
    return 0;
  }
}

long 
SGPropertyNode::getLongValue () const
{
  DO_TRACE_READ(LONG);
  TEST_READ(0L);
  switch (_type) {
  case ALIAS:
    return _value.alias->getLongValue();
  case BOOL:
    return long(GET_BOOL);
  case INT:
    return long(GET_INT);
  case LONG:
    return GET_LONG;
  case FLOAT:
    return long(GET_FLOAT);
  case DOUBLE:
    return long(GET_DOUBLE);
  case STRING:
  case UNSPECIFIED:
    return strtol(GET_STRING.c_str(), 0, 0);
  case NONE:
  default:
    return 0L;
  }
}

float 
SGPropertyNode::getFloatValue () const
{
  DO_TRACE_READ(FLOAT);
  TEST_READ(0.0);
  switch (_type) {
  case ALIAS:
    return _value.alias->getFloatValue();
  case BOOL:
    return float(GET_BOOL);
  case INT:
    return float(GET_INT);
  case LONG:
    return float(GET_LONG);
  case FLOAT:
    return GET_FLOAT;
  case DOUBLE:
    return float(GET_DOUBLE);
  case STRING:
  case UNSPECIFIED:
    return atof(GET_STRING.c_str());
  case NONE:
  default:
    return 0.0;
  }
}

double 
SGPropertyNode::getDoubleValue () const
{
  DO_TRACE_READ(DOUBLE);
  TEST_READ(0.0L);
  switch (_type) {
  case ALIAS:
    return _value.alias->getDoubleValue();
  case BOOL:
    return double(GET_BOOL);
  case INT:
    return double(GET_INT);
  case LONG:
    return double(GET_LONG);
  case FLOAT:
    return double(GET_FLOAT);
  case DOUBLE:
    return GET_DOUBLE;
  case STRING:
  case UNSPECIFIED:
    return strtod(GET_STRING.c_str(), 0);
  case NONE:
  default:
    return 0.0L;
  }
}

string
SGPropertyNode::getStringValue () const
{
  DO_TRACE_READ(STRING);
  return get_string();
}

bool
SGPropertyNode::setBoolValue (bool value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clear_value();
    _value.bool_val = new SGRawValueInternal<bool>;
    _type = BOOL;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setBoolValue(value);
    break;
  case BOOL:
    result = SET_BOOL(value);
    break;
  case INT:
    result = SET_INT(int(value));
    break;
  case LONG:
    result = SET_LONG(long(value));
    break;
  case FLOAT:
    result = SET_FLOAT(float(value));
    break;
  case DOUBLE:
    result = SET_DOUBLE(double(value));
    break;
  case STRING:
  case UNSPECIFIED:
    result = SET_STRING(value ? "true" : "false");
    break;
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(BOOL);
  return result;
}

bool
SGPropertyNode::setIntValue (int value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clear_value();
    _value.int_val = new SGRawValueInternal<int>;
    _type = INT;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setIntValue(value);
    break;
  case BOOL:
    result = SET_BOOL(value == 0 ? false : true);
    break;
  case INT:
    result = SET_INT(value);
    break;
  case LONG:
    result = SET_LONG(long(value));
    break;
  case FLOAT:
    result = SET_FLOAT(float(value));
    break;
  case DOUBLE:
    result = SET_DOUBLE(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%d", value);
    result = SET_STRING(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(INT);
  return result;
}

bool
SGPropertyNode::setLongValue (long value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clear_value();
    _value.long_val = new SGRawValueInternal<long>;
    _type = LONG;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setLongValue(value);
    break;
  case BOOL:
    result = SET_BOOL(value == 0L ? false : true);
    break;
  case INT:
    result = SET_INT(int(value));
    break;
  case LONG:
    result = SET_LONG(value);
    break;
  case FLOAT:
    result = SET_FLOAT(float(value));
    break;
  case DOUBLE:
    result = SET_DOUBLE(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%ld", value);
    result = SET_STRING(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(LONG);
  return result;
}

bool
SGPropertyNode::setFloatValue (float value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clear_value();
    _value.float_val = new SGRawValueInternal<float>;
    _type = FLOAT;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setFloatValue(value);
    break;
  case BOOL:
    result = SET_BOOL(value == 0.0 ? false : true);
    break;
  case INT:
    result = SET_INT(int(value));
    break;
  case LONG:
    result = SET_LONG(long(value));
    break;
  case FLOAT:
    result = SET_FLOAT(value);
    break;
  case DOUBLE:
    result = SET_DOUBLE(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%f", value);
    result = SET_STRING(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(FLOAT);
  return result;
}

bool
SGPropertyNode::setDoubleValue (double value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clear_value();
    _value.double_val = new SGRawValueInternal<double>;
    _type = DOUBLE;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setDoubleValue(value);
    break;
  case BOOL:
    result = SET_BOOL(value == 0.0L ? false : true);
    break;
  case INT:
    result = SET_INT(int(value));
    break;
  case LONG:
    result = SET_LONG(long(value));
    break;
  case FLOAT:
    result = SET_FLOAT(float(value));
    break;
  case DOUBLE:
    result = SET_DOUBLE(value);
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%f", value);
    result = SET_STRING(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(DOUBLE);
  return result;
}

bool
SGPropertyNode::setStringValue (string value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clear_value();
    _value.string_val = new SGRawValueInternal<string>;
    _type = STRING;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setStringValue(value);
    break;
  case BOOL:
    result = SET_BOOL((value == "true" || atoi(value.c_str())) ? true : false);
    break;
  case INT:
    result = SET_INT(atoi(value.c_str()));
    break;
  case LONG:
    result = SET_LONG(strtol(value.c_str(), 0, 0));
    break;
  case FLOAT:
    result = SET_FLOAT(atof(value.c_str()));
    break;
  case DOUBLE:
    result = SET_DOUBLE(strtod(value.c_str(), 0));
    break;
  case STRING:
  case UNSPECIFIED:
    result = SET_STRING(value);
    break;
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(STRING);
  return result;
}

bool
SGPropertyNode::setUnspecifiedValue (string value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE) {
    clear_value();
    _value.string_val = new SGRawValueInternal<string>;
    _type = UNSPECIFIED;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setUnspecifiedValue(value);
    break;
  case BOOL:
    result = SET_BOOL((value == "true" || atoi(value.c_str())) ? true : false);
    break;
  case INT:
    result = SET_INT(atoi(value.c_str()));
    break;
  case LONG:
    result = SET_LONG(strtol(value.c_str(), 0, 0));
    break;
  case FLOAT:
    result = SET_FLOAT(atof(value.c_str()));
    break;
  case DOUBLE:
    result = SET_DOUBLE(strtod(value.c_str(), 0));
    break;
  case STRING:
  case UNSPECIFIED:
    result = SET_STRING(value);
    break;
  case NONE:
  default:
    break;
  }

  DO_TRACE_WRITE(UNSPECIFIED);
  return result;
}

bool
SGPropertyNode::tie (const SGRawValue<bool> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  bool old_val = false;
  if (useDefault)
    old_val = getBoolValue();

  clear_value();
  _type = BOOL;
  _tied = true;
  _value.bool_val = rawValue.clone();

  if (useDefault)
    setBoolValue(old_val);

  return true;
}

bool
SGPropertyNode::tie (const SGRawValue<int> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  int old_val = 0;
  if (useDefault)
    old_val = getIntValue();

  clear_value();
  _type = INT;
  _tied = true;
  _value.int_val = rawValue.clone();

  if (useDefault)
    setIntValue(old_val);

  return true;
}

bool
SGPropertyNode::tie (const SGRawValue<long> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  long old_val = 0;
  if (useDefault)
    old_val = getLongValue();

  clear_value();
  _type = LONG;
  _tied = true;
  _value.long_val = rawValue.clone();

  if (useDefault)
    setLongValue(old_val);

  return true;
}

bool
SGPropertyNode::tie (const SGRawValue<float> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  float old_val = 0.0;
  if (useDefault)
    old_val = getFloatValue();

  clear_value();
  _type = FLOAT;
  _tied = true;
  _value.float_val = rawValue.clone();

  if (useDefault)
    setFloatValue(old_val);

  return true;
}

bool
SGPropertyNode::tie (const SGRawValue<double> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  double old_val = 0.0;
  if (useDefault)
    old_val = getDoubleValue();

  clear_value();
  _type = DOUBLE;
  _tied = true;
  _value.double_val = rawValue.clone();

  if (useDefault)
    setDoubleValue(old_val);

  return true;

}

bool
SGPropertyNode::tie (const SGRawValue<string> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  string old_val;
  if (useDefault)
    old_val = getStringValue();

  clear_value();
  _type = STRING;
  _tied = true;
  _value.string_val = rawValue.clone();

  if (useDefault)
    setStringValue(old_val);

  return true;
}

bool
SGPropertyNode::untie ()
{
  if (!_tied)
    return false;

  switch (_type) {
  case BOOL: {
    bool val = getBoolValue();
    clear_value();
    _type = BOOL;
    _value.bool_val = new SGRawValueInternal<bool>;
    SET_BOOL(val);
    break;
  }
  case INT: {
    int val = getIntValue();
    clear_value();
    _type = INT;
    _value.int_val = new SGRawValueInternal<int>;
    SET_INT(val);
    break;
  }
  case LONG: {
    long val = getLongValue();
    clear_value();
    _type = LONG;
    _value.long_val = new SGRawValueInternal<long>;
    SET_LONG(val);
    break;
  }
  case FLOAT: {
    float val = getFloatValue();
    clear_value();
    _type = FLOAT;
    _value.float_val = new SGRawValueInternal<float>;
    SET_FLOAT(val);
    break;
  }
  case DOUBLE: {
    double val = getDoubleValue();
    clear_value();
    _type = DOUBLE;
    _value.double_val = new SGRawValueInternal<double>;
    SET_DOUBLE(val);
    break;
  }
  case STRING:
  case UNSPECIFIED: {
    string val = getStringValue();
    clear_value();
    _type = STRING;
    _value.string_val = new SGRawValueInternal<string>;
    SET_STRING(val);
    break;
  }
  case NONE:
  default:
    break;
  }

  _tied = false;
  return true;
}

SGPropertyNode *
SGPropertyNode::getRootNode ()
{
  if (_parent == 0)
    return this;
  else
    return _parent->getRootNode();
}

const SGPropertyNode *
SGPropertyNode::getRootNode () const
{
  if (_parent == 0)
    return this;
  else
    return _parent->getRootNode();
}

SGPropertyNode *
SGPropertyNode::getNode (const string &relative_path, bool create)
{
  if (_path_cache == 0)
    _path_cache = new cache_map;

  SGPropertyNode * result = (*_path_cache)[relative_path];
  if (result == 0) {
    vector<PathComponent> components;
    parse_path(relative_path, components);
    result = find_node(this, components, 0, create);
    (*_path_cache)[relative_path] = result;
  }
  
  return result;
}

SGPropertyNode *
SGPropertyNode::getNode (const string &relative_path, int index, bool create)
{
  vector<PathComponent> components;
  parse_path(relative_path, components);
  if (components.size() > 0)
    components[components.size()-1].index = index;
  return find_node(this, components, 0, create);
}

const SGPropertyNode *
SGPropertyNode::getNode (const string &relative_path) const
{
  return ((SGPropertyNode *)this)->getNode(relative_path, false);
}

const SGPropertyNode *
SGPropertyNode::getNode (const string &relative_path, int index) const
{
  return ((SGPropertyNode *)this)->getNode(relative_path, index, false);
}


////////////////////////////////////////////////////////////////////////
// Convenience methods using relative paths.
////////////////////////////////////////////////////////////////////////


/**
 * Test whether another node has a value attached.
 */
bool
SGPropertyNode::hasValue (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->hasValue());
}


/**
 * Get the value type for another node.
 */
SGPropertyNode::Type
SGPropertyNode::getType (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? UNSPECIFIED : (Type)(node->getType()));
}


/**
 * Get a bool value for another node.
 */
bool
SGPropertyNode::getBoolValue (const string &relative_path,
			      bool defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getBoolValue());
}


/**
 * Get an int value for another node.
 */
int
SGPropertyNode::getIntValue (const string &relative_path,
			     int defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getIntValue());
}


/**
 * Get a long value for another node.
 */
long
SGPropertyNode::getLongValue (const string &relative_path,
			      long defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getLongValue());
}


/**
 * Get a float value for another node.
 */
float
SGPropertyNode::getFloatValue (const string &relative_path,
			       float defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getFloatValue());
}


/**
 * Get a double value for another node.
 */
double
SGPropertyNode::getDoubleValue (const string &relative_path,
				double defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getDoubleValue());
}


/**
 * Get a string value for another node.
 */
string
SGPropertyNode::getStringValue (const string &relative_path,
				string defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getStringValue());
}


/**
 * Set a bool value for another node.
 */
bool
SGPropertyNode::setBoolValue (const string &relative_path, bool value)
{
  return getNode(relative_path, true)->setBoolValue(value);
}


/**
 * Set an int value for another node.
 */
bool
SGPropertyNode::setIntValue (const string &relative_path, int value)
{
  return getNode(relative_path, true)->setIntValue(value);
}


/**
 * Set a long value for another node.
 */
bool
SGPropertyNode::setLongValue (const string &relative_path, long value)
{
  return getNode(relative_path, true)->setLongValue(value);
}


/**
 * Set a float value for another node.
 */
bool
SGPropertyNode::setFloatValue (const string &relative_path, float value)
{
  return getNode(relative_path, true)->setFloatValue(value);
}


/**
 * Set a double value for another node.
 */
bool
SGPropertyNode::setDoubleValue (const string &relative_path, double value)
{
  return getNode(relative_path, true)->setDoubleValue(value);
}


/**
 * Set a string value for another node.
 */
bool
SGPropertyNode::setStringValue (const string &relative_path, string value)
{
  return getNode(relative_path, true)->setStringValue(value);
}


/**
 * Set an unknown value for another node.
 */
bool
SGPropertyNode::setUnspecifiedValue (const string &relative_path, string value)
{
  return getNode(relative_path, true)->setUnspecifiedValue(value);
}


/**
 * Test whether another node is tied.
 */
bool
SGPropertyNode::isTied (const string &relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->isTied());
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<bool> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<int> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<long> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<float> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<double> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const string &relative_path,
		     const SGRawValue<string> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Attempt to untie another node reached by a relative path.
 */
bool
SGPropertyNode::untie (const string &relative_path)
{
  SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->untie());
}

// end of props.cxx
