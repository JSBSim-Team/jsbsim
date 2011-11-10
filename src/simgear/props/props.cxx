// props.cxx - implementation of a property list.
// Started Fall 2000 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// See props.html for documentation [replace with URL when available].
//
// $Id: props.cxx,v 1.6 2011/11/10 12:06:15 jberndt Exp $

#include "props.hxx"

#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <string.h>

#if PROPS_STANDALONE

#include <iostream>
#include <ctype.h>
#include <stdlib.h>
using std::cerr;
using std::endl;
using std::find;
using std::sort;
using std::vector;
using std::stringstream;

#else

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>

SG_USING_STD(sort);
SG_USING_STD(find);
SG_USING_STD(vector);
SG_USING_STD(stringstream);

#if ( _MSC_VER == 1200 )
// MSVC 6 is buggy, and needs something strange here
SG_USING_STD(vector<SGPropertyNode_ptr>);
SG_USING_STD(vector<SGPropertyChangeListener *>);
SG_USING_STD(vector<SGPropertyNode *>);
#endif

#endif



////////////////////////////////////////////////////////////////////////
// Local classes.
////////////////////////////////////////////////////////////////////////

/**
 * Comparator class for sorting by index.
 */
class CompareIndices
{
public:
  int operator() (const SGPropertyNode_ptr n1, const SGPropertyNode_ptr n2) const {
    return (n1->getIndex() < n2->getIndex());
  }
};



////////////////////////////////////////////////////////////////////////
// Convenience macros for value access.
////////////////////////////////////////////////////////////////////////

#define TEST_READ(dflt) if (!getAttribute(READ)) return dflt
#define TEST_WRITE if (!getAttribute(WRITE)) return false



////////////////////////////////////////////////////////////////////////
// Default values for every type.
////////////////////////////////////////////////////////////////////////

template<> const bool SGRawValue<bool>::DefaultValue = false;
template<> const int SGRawValue<int>::DefaultValue = 0;
template<> const long SGRawValue<long>::DefaultValue = 0L;
template<> const float SGRawValue<float>::DefaultValue = 0.0;
template<> const double SGRawValue<double>::DefaultValue = 0.0L;
template<> const char * const SGRawValue<const char *>::DefaultValue = "";



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
static inline const string
parse_name (const string &path, int &i)
{
  string name = "";
  int max = (int)path.size();

  if (path[i] == '.') {
    i++;
    if (i < max && path[i] == '.') {
      i++;
      name = "..";
    } else {
      name = ".";
    }
    if (i < max && path[i] != '/')
      throw string("Illegal character after " + name);
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

  for (int max = (int)path.size(); i < max; i++) {
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
  int max = (int)path.size();

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


static char *
copy_string (const char * s)
{
  // FIXME: potential buffer overflow.
  // For some reason, strnlen and
  // strncpy cause all kinds of crashes.
  char * copy = new char[strlen(s) + 1];
  strcpy(copy, s);
  return copy;
}

static bool
compare_strings (const char * s1, const char * s2)
{
  return !strncmp(s1, s2, SGPropertyNode::MAX_STRING_LEN);
}

/**
 * Locate a child node by name and index.
 */
static int
find_child (const char * name, int index, vector<SGPropertyNode_ptr> nodes)
{
  int nNodes = nodes.size();
  for (int i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];
    if (compare_strings(node->getName(), name) && node->getIndex() == index)
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
    return (current->getAttribute(SGPropertyNode::REMOVED) ? 0 : current);
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
      current->getChild(components[position].name.c_str(),
      components[position].index,
      create);
    return find_node(child, components, position + 1, create);
  }
}



////////////////////////////////////////////////////////////////////////
// Private methods from SGPropertyNode (may be inlined for speed).
////////////////////////////////////////////////////////////////////////

inline bool
SGPropertyNode::get_bool () const
{
  if (_tied)
    return _value.bool_val->getValue();
  else
    return _local_val.bool_val;
}

inline int
SGPropertyNode::get_int () const
{
  if (_tied)
    return _value.int_val->getValue();
  else
    return _local_val.int_val;
}

inline long
SGPropertyNode::get_long () const
{
  if (_tied)
    return _value.long_val->getValue();
  else
    return _local_val.long_val;
}

inline float
SGPropertyNode::get_float () const
{
  if (_tied)
    return _value.float_val->getValue();
  else
    return _local_val.float_val;
}

inline double
SGPropertyNode::get_double () const
{
  if (_tied)
    return _value.double_val->getValue();
  else
    return _local_val.double_val;
}

inline const char *
SGPropertyNode::get_string () const
{
  if (_tied)
    return _value.string_val->getValue();
  else
    return _local_val.string_val;
}

inline bool
SGPropertyNode::set_bool (bool val)
{
  if (_tied) {
    if (_value.bool_val->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.bool_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_int (int val)
{
  if (_tied) {
    if (_value.int_val->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.int_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_long (long val)
{
  if (_tied) {
    if (_value.long_val->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.long_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_float (float val)
{
  if (_tied) {
    if (_value.float_val->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.float_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_double (double val)
{
  if (_tied) {
    if (_value.double_val->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    _local_val.double_val = val;
    fireValueChanged();
    return true;
  }
}

inline bool
SGPropertyNode::set_string (const char * val)
{
  if (_tied) {
    if (_value.string_val->setValue(val)) {
      fireValueChanged();
      return true;
    } else {
      return false;
    }
  } else {
    delete [] _local_val.string_val;
    _local_val.string_val = copy_string(val);
    fireValueChanged();
    return true;
  }
}

void
SGPropertyNode::clearValue ()
{
  switch (_type) {
  case NONE:
    break;
  case ALIAS:
    _value.alias = 0;
    break;
  case BOOL:
    if (_tied) {
      delete _value.bool_val;
      _value.bool_val = 0;
    }
    _local_val.bool_val = SGRawValue<bool>::DefaultValue;
    break;
  case INT:
    if (_tied) {
      delete _value.int_val;
      _value.int_val = 0;
    }
    _local_val.int_val = SGRawValue<int>::DefaultValue;
    break;
  case LONG:
    if (_tied) {
      delete _value.long_val;
      _value.long_val = 0L;
    }
    _local_val.long_val = SGRawValue<long>::DefaultValue;
    break;
  case FLOAT:
    if (_tied) {
      delete _value.float_val;
      _value.float_val = 0;
    }
    _local_val.float_val = SGRawValue<float>::DefaultValue;
    break;
  case DOUBLE:
    if (_tied) {
      delete _value.double_val;
      _value.double_val = 0;
    }
    _local_val.double_val = SGRawValue<double>::DefaultValue;
    break;
  case STRING:
  case UNSPECIFIED:
    if (_tied) {
      delete _value.string_val;
      _value.string_val = 0;
    } else {
      delete [] _local_val.string_val;
    }
    _local_val.string_val = 0;
    break;
  }
  _tied = false;
  _type = NONE;
}


/**
 * Get the value as a string.
 */
const char *
SGPropertyNode::make_string () const
{
  if (!getAttribute(READ))
    return "";

  switch (_type) {
  case ALIAS:
    return _value.alias->getStringValue();
  case BOOL:
    if (get_bool())
      return "true";
    else
      return "false";
  case INT:
    {
      stringstream sstr;
      sstr << get_int();
      _buffer = sstr.str();
      return _buffer.c_str();
    }
  case LONG:
    {
      stringstream sstr;
      sstr << get_long();
      _buffer = sstr.str();
      return _buffer.c_str();
    }
  case FLOAT:
    {
      stringstream sstr;
      sstr << get_float();
      _buffer = sstr.str();
      return _buffer.c_str();
    }
  case DOUBLE:
    {
      stringstream sstr;
      sstr.precision( 10 );
      sstr << get_double();
      _buffer = sstr.str();
      return _buffer.c_str();
    }
  case STRING:
  case UNSPECIFIED:
    return get_string();
  case NONE:
  default:
    return "";
  }
}

/**
 * Trace a write access for a property.
 */
void
SGPropertyNode::trace_write () const
{
#if PROPS_STANDALONE
  cerr << "TRACE: Write node " << getPath () << ", value\""
       << make_string() << '"' << endl;
#else
  SG_LOG(SG_GENERAL, SG_INFO, "TRACE: Write node " << getPath()
  << ", value\"" << make_string() << '"');
#endif
}

/**
 * Trace a read access for a property.
 */
void
SGPropertyNode::trace_read () const
{
#if PROPS_STANDALONE
  cerr << "TRACE: Write node " << getPath () << ", value \""
       << make_string() << '"' << endl;
#else
  SG_LOG(SG_GENERAL, SG_INFO, "TRACE: Read node " << getPath()
	 << ", value \"" << make_string() << '"');
#endif
}


////////////////////////////////////////////////////////////////////////
// Public methods from SGPropertyNode.
////////////////////////////////////////////////////////////////////////

/**
 * Last used attribute
 * Update as needed when enum Attribute is changed
 */
const int SGPropertyNode::LAST_USED_ATTRIBUTE = TRACE_WRITE;

/**
 * Default constructor: always creates a root node.
 */
SGPropertyNode::SGPropertyNode ()
  : _index(0),
    _parent(0),
    _path_cache(0),
    _type(NONE),
    _tied(false),
    _attr(READ|WRITE),
    _listeners(0)
{
  _local_val.string_val = 0;
}


/**
 * Copy constructor.
 */
SGPropertyNode::SGPropertyNode (const SGPropertyNode &node)
  : _index(node._index),
    _name(node._name),
    _parent(0),			// don't copy the parent
    _path_cache(0),
    _type(node._type),
    _tied(node._tied),
    _attr(node._attr),
    _listeners(0)		// CHECK!!
{
  _local_val.string_val = 0;
  switch (_type) {
  case NONE:
    break;
  case ALIAS:
    _value.alias = node._value.alias;
    _tied = false;
    break;
  case BOOL:
    if (_tied) {
      _tied = true;
      _value.bool_val = node._value.bool_val->clone();
    } else {
      _tied = false;
      set_bool(node.get_bool());
    }
    break;
  case INT:
    if (_tied) {
      _tied = true;
      _value.int_val = node._value.int_val->clone();
    } else {
      _tied = false;
      set_int(node.get_int());
    }
    break;
  case LONG:
    if (_tied) {
      _tied = true;
      _value.long_val = node._value.long_val->clone();
    } else {
      _tied = false;
      set_long(node.get_long());
    }
    break;
  case FLOAT:
    if (_tied) {
      _tied = true;
      _value.float_val = node._value.float_val->clone();
    } else {
      _tied = false;
      set_float(node.get_float());
    }
    break;
  case DOUBLE:
    if (_tied) {
      _tied = true;
      _value.double_val = node._value.double_val->clone();
    } else {
      _tied = false;
      set_double(node.get_double());
    }
    break;
  case STRING:
  case UNSPECIFIED:
    if (_tied) {
      _tied = true;
      _value.string_val = node._value.string_val->clone();
    } else {
      _tied = false;
      set_string(node.get_string());
    }
    break;
  }
}


/**
 * Convenience constructor.
 */
SGPropertyNode::SGPropertyNode (const char * name,
				int index,
				SGPropertyNode * parent)
  : _index(index),
    _parent(parent),
    _path_cache(0),
    _type(NONE),
    _tied(false),
    _attr(READ|WRITE),
    _listeners(0)
{
  _name = name;
  _local_val.string_val = 0;
}


/**
 * Destructor.
 */
SGPropertyNode::~SGPropertyNode ()
{
  delete _path_cache;
  clearValue();
  delete _listeners;
}


/**
 * Alias to another node.
 */
bool
SGPropertyNode::alias (SGPropertyNode * target)
{
  if (target == 0 || _type == ALIAS || _tied)
    return false;
  clearValue();
  _value.alias = target;
  _type = ALIAS;
  return true;
}


/**
 * Alias to another node by path.
 */
bool
SGPropertyNode::alias (const char * path)
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
SGPropertyNode::getChild (const char * name, int index, bool create)
{
  int pos = find_child(name, index, _children);
  if (pos >= 0) {
    return _children[pos];
  } else if (create) {
    SGPropertyNode_ptr node;
    pos = find_child(name, index, _removedChildren);
    if (pos >= 0) {
      vector<SGPropertyNode_ptr>::iterator it = _removedChildren.begin();
      it += pos;
      node = _removedChildren[pos];
      _removedChildren.erase(it);
      node->setAttribute(REMOVED, false);
    } else {
      node = new SGPropertyNode(name, index, this);
    }
    _children.push_back(node);
    fireChildAdded(node);
    return node;
  } else {
    return 0;
  }
}


/**
 * Get a const child by name and index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (const char * name, int index) const
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
vector<SGPropertyNode_ptr>
SGPropertyNode::getChildren (const char * name) const
{
  vector<SGPropertyNode_ptr> children;
  int max = _children.size();

  for (int i = 0; i < max; i++)
    if (compare_strings(_children[i]->getName(), name))
      children.push_back(_children[i]);

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


/**
 * Remove child by position.
 */
SGPropertyNode_ptr
SGPropertyNode::removeChild (int pos, bool keep)
{
  SGPropertyNode_ptr node;
  if (pos < 0 || pos >= (int)_children.size())
    return node;

  vector<SGPropertyNode_ptr>::iterator it = _children.begin();
  it += pos;
  node = _children[pos];
  _children.erase(it);
  if (keep) {
    _removedChildren.push_back(node);
  }
  if (_path_cache)
     _path_cache->erase(node->getName()); // EMH - TODO: Take "index" into account!
  node->setAttribute(REMOVED, true);
  node->clearValue();
  fireChildRemoved(node);
  return node;
}


/**
 * Remove a child node
 */
SGPropertyNode_ptr
SGPropertyNode::removeChild (const char * name, int index, bool keep)
{
  SGPropertyNode_ptr ret;
  int pos = find_child(name, index, _children);
  if (pos >= 0)
    ret = removeChild(pos, keep);
  return ret;
}


/**
  * Remove all children with the specified name.
  */
vector<SGPropertyNode_ptr>
SGPropertyNode::removeChildren (const char * name, bool keep)
{
  vector<SGPropertyNode_ptr> children;

  for (int pos = _children.size() - 1; pos >= 0; pos--)
    if (compare_strings(_children[pos]->getName(), name))
      children.push_back(removeChild(pos, keep));

  sort(children.begin(), children.end(), CompareIndices());
  return children;
}


const char *
SGPropertyNode::getDisplayName (bool simplify) const
{
  _display_name = _name;
  if (_index != 0 || !simplify) {
    stringstream sstr;
    sstr << '[' << _index << ']';
    _display_name += sstr.str();
  }
  return _display_name.c_str();
}


const char *
SGPropertyNode::getPath (bool simplify) const
{
  // Calculate the complete path only once.
  if (_parent != 0 && _path.empty()) {
    _path = _parent->getPath(simplify);
    _path += '/';
    _path += getDisplayName(simplify);
  }

  return _path.c_str();
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
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == BOOL)
    return get_bool();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<bool>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getBoolValue();
  case BOOL:
    return get_bool();
  case INT:
    return get_int() == 0 ? false : true;
  case LONG:
    return get_long() == 0L ? false : true;
  case FLOAT:
    return get_float() == 0.0 ? false : true;
  case DOUBLE:
    return get_double() == 0.0L ? false : true;
  case STRING:
  case UNSPECIFIED:
    return (compare_strings(get_string(), "true") || getDoubleValue() != 0.0L);
  case NONE:
  default:
    return SGRawValue<bool>::DefaultValue;
  }
}

int
SGPropertyNode::getIntValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == INT)
    return get_int();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<int>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getIntValue();
  case BOOL:
    return int(get_bool());
  case INT:
    return get_int();
  case LONG:
    return int(get_long());
  case FLOAT:
    return int(get_float());
  case DOUBLE:
    return int(get_double());
  case STRING:
  case UNSPECIFIED:
    return atoi(get_string());
  case NONE:
  default:
    return SGRawValue<int>::DefaultValue;
  }
}

long
SGPropertyNode::getLongValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == LONG)
    return get_long();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<long>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getLongValue();
  case BOOL:
    return long(get_bool());
  case INT:
    return long(get_int());
  case LONG:
    return get_long();
  case FLOAT:
    return long(get_float());
  case DOUBLE:
    return long(get_double());
  case STRING:
  case UNSPECIFIED:
    return strtol(get_string(), 0, 0);
  case NONE:
  default:
    return SGRawValue<long>::DefaultValue;
  }
}

float
SGPropertyNode::getFloatValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == FLOAT)
    return get_float();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<float>::DefaultValue;
  switch (_type) {
  case ALIAS:
    return _value.alias->getFloatValue();
  case BOOL:
    return float(get_bool());
  case INT:
    return float(get_int());
  case LONG:
    return float(get_long());
  case FLOAT:
    return get_float();
  case DOUBLE:
    return float(get_double());
  case STRING:
  case UNSPECIFIED:
    return atof(get_string());
  case NONE:
  default:
    return SGRawValue<float>::DefaultValue;
  }
}

double
SGPropertyNode::getDoubleValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == DOUBLE)
    return get_double();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<double>::DefaultValue;

  switch (_type) {
  case ALIAS:
    return _value.alias->getDoubleValue();
  case BOOL:
    return double(get_bool());
  case INT:
    return double(get_int());
  case LONG:
    return double(get_long());
  case FLOAT:
    return double(get_float());
  case DOUBLE:
    return get_double();
  case STRING:
  case UNSPECIFIED:
    return strtod(get_string(), 0);
  case NONE:
  default:
    return SGRawValue<double>::DefaultValue;
  }
}

const char *
SGPropertyNode::getStringValue () const
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == STRING)
    return get_string();

  if (getAttribute(TRACE_READ))
    trace_read();
  if (!getAttribute(READ))
    return SGRawValue<const char *>::DefaultValue;
  return make_string();
}

bool
SGPropertyNode::setBoolValue (bool value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == BOOL)
    return set_bool(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _tied = false;
    _type = BOOL;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setBoolValue(value);
    break;
  case BOOL:
    result = set_bool(value);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED:
    result = set_string(value ? "true" : "false");
    break;
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setIntValue (int value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == INT)
    return set_int(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = INT;
    _local_val.int_val = 0;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setIntValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0 ? false : true);
    break;
  case INT:
    result = set_int(value);
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%d", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setLongValue (long value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == LONG)
    return set_long(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = LONG;
    _local_val.long_val = 0L;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setLongValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0L ? false : true);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(value);
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%ld", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setFloatValue (float value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == FLOAT)
    return set_float(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = FLOAT;
    _local_val.float_val = 0;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setFloatValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0.0 ? false : true);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(value);
    break;
  case DOUBLE:
    result = set_double(double(value));
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%f", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setDoubleValue (double value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == DOUBLE)
    return set_double(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _local_val.double_val = value;
    _type = DOUBLE;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setDoubleValue(value);
    break;
  case BOOL:
    result = set_bool(value == 0.0L ? false : true);
    break;
  case INT:
    result = set_int(int(value));
    break;
  case LONG:
    result = set_long(long(value));
    break;
  case FLOAT:
    result = set_float(float(value));
    break;
  case DOUBLE:
    result = set_double(value);
    break;
  case STRING:
  case UNSPECIFIED: {
    char buf[128];
    sprintf(buf, "%f", value);
    result = set_string(buf);
    break;
  }
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setStringValue (const char * value)
{
				// Shortcut for common case
  if (_attr == (READ|WRITE) && _type == STRING)
    return set_string(value);

  bool result = false;
  TEST_WRITE;
  if (_type == NONE || _type == UNSPECIFIED) {
    clearValue();
    _type = STRING;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setStringValue(value);
    break;
  case BOOL:
    result = set_bool((compare_strings(value, "true")
		       || atoi(value)) ? true : false);
    break;
  case INT:
    result = set_int(atoi(value));
    break;
  case LONG:
    result = set_long(strtol(value, 0, 0));
    break;
  case FLOAT:
    result = set_float(atof(value));
    break;
  case DOUBLE:
    result = set_double(strtod(value, 0));
    break;
  case STRING:
  case UNSPECIFIED:
    result = set_string(value);
    break;
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
  return result;
}

bool
SGPropertyNode::setUnspecifiedValue (const char * value)
{
  bool result = false;
  TEST_WRITE;
  if (_type == NONE) {
    clearValue();
    _type = UNSPECIFIED;
  }

  switch (_type) {
  case ALIAS:
    result = _value.alias->setUnspecifiedValue(value);
    break;
  case BOOL:
    result = set_bool((compare_strings(value, "true")
		       || atoi(value)) ? true : false);
    break;
  case INT:
    result = set_int(atoi(value));
    break;
  case LONG:
    result = set_long(strtol(value, 0, 0));
    break;
  case FLOAT:
    result = set_float(atof(value));
    break;
  case DOUBLE:
    result = set_double(strtod(value, 0));
    break;
  case STRING:
  case UNSPECIFIED:
    result = set_string(value);
    break;
  case NONE:
  default:
    break;
  }

  if (getAttribute(TRACE_WRITE))
    trace_write();
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

  clearValue();
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

  clearValue();
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

  clearValue();
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

  clearValue();
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

  clearValue();
  _type = DOUBLE;
  _tied = true;
  _value.double_val = rawValue.clone();

  if (useDefault)
    setDoubleValue(old_val);

  return true;

}

bool
SGPropertyNode::tie (const SGRawValue<const char *> &rawValue, bool useDefault)
{
  if (_type == ALIAS || _tied)
    return false;

  useDefault = useDefault && hasValue();
  string old_val;
  if (useDefault)
    old_val = getStringValue();

  clearValue();
  _type = STRING;
  _tied = true;
  _value.string_val = rawValue.clone();

  if (useDefault)
    setStringValue(old_val.c_str());

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
    clearValue();
    _type = BOOL;
    _local_val.bool_val = val;
    break;
  }
  case INT: {
    int val = getIntValue();
    clearValue();
    _type = INT;
    _local_val.int_val = val;
    break;
  }
  case LONG: {
    long val = getLongValue();
    clearValue();
    _type = LONG;
    _local_val.long_val = val;
    break;
  }
  case FLOAT: {
    float val = getFloatValue();
    clearValue();
    _type = FLOAT;
    _local_val.float_val = val;
    break;
  }
  case DOUBLE: {
    double val = getDoubleValue();
    clearValue();
    _type = DOUBLE;
    _local_val.double_val = val;
    break;
  }
  case STRING:
  case UNSPECIFIED: {
    string val = getStringValue();
    clearValue();
    _type = STRING;
    _local_val.string_val = copy_string(val.c_str());
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
SGPropertyNode::getNode (const char * relative_path, bool create)
{
  if (_path_cache == 0)
    _path_cache = new hash_table;

  SGPropertyNode * result = _path_cache->get(relative_path);
  if (result == 0) {
    vector<PathComponent> components;
    parse_path(relative_path, components);
    result = find_node(this, components, 0, create);
    if (result != 0)
      _path_cache->put(relative_path, result);
  }

  return result;
}

SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, int index, bool create)
{
  vector<PathComponent> components;
  parse_path(relative_path, components);
  if (components.size() > 0)
    components.back().index = index;
  return find_node(this, components, 0, create);
}

const SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path) const
{
  return ((SGPropertyNode *)this)->getNode(relative_path, false);
}

const SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, int index) const
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
SGPropertyNode::hasValue (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->hasValue());
}


/**
 * Get the value type for another node.
 */
SGPropertyNode::Type
SGPropertyNode::getType (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? UNSPECIFIED : (Type)(node->getType()));
}


/**
 * Get a bool value for another node.
 */
bool
SGPropertyNode::getBoolValue (const char * relative_path,
			      bool defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getBoolValue());
}


/**
 * Get an int value for another node.
 */
int
SGPropertyNode::getIntValue (const char * relative_path,
			     int defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getIntValue());
}


/**
 * Get a long value for another node.
 */
long
SGPropertyNode::getLongValue (const char * relative_path,
			      long defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getLongValue());
}


/**
 * Get a float value for another node.
 */
float
SGPropertyNode::getFloatValue (const char * relative_path,
			       float defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getFloatValue());
}


/**
 * Get a double value for another node.
 */
double
SGPropertyNode::getDoubleValue (const char * relative_path,
				double defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getDoubleValue());
}


/**
 * Get a string value for another node.
 */
const char *
SGPropertyNode::getStringValue (const char * relative_path,
				const char * defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? defaultValue : node->getStringValue());
}


/**
 * Set a bool value for another node.
 */
bool
SGPropertyNode::setBoolValue (const char * relative_path, bool value)
{
  return getNode(relative_path, true)->setBoolValue(value);
}


/**
 * Set an int value for another node.
 */
bool
SGPropertyNode::setIntValue (const char * relative_path, int value)
{
  return getNode(relative_path, true)->setIntValue(value);
}


/**
 * Set a long value for another node.
 */
bool
SGPropertyNode::setLongValue (const char * relative_path, long value)
{
  return getNode(relative_path, true)->setLongValue(value);
}


/**
 * Set a float value for another node.
 */
bool
SGPropertyNode::setFloatValue (const char * relative_path, float value)
{
  return getNode(relative_path, true)->setFloatValue(value);
}


/**
 * Set a double value for another node.
 */
bool
SGPropertyNode::setDoubleValue (const char * relative_path, double value)
{
  return getNode(relative_path, true)->setDoubleValue(value);
}


/**
 * Set a string value for another node.
 */
bool
SGPropertyNode::setStringValue (const char * relative_path, const char * value)
{
  return getNode(relative_path, true)->setStringValue(value);
}


/**
 * Set an unknown value for another node.
 */
bool
SGPropertyNode::setUnspecifiedValue (const char * relative_path,
				     const char * value)
{
  return getNode(relative_path, true)->setUnspecifiedValue(value);
}


/**
 * Test whether another node is tied.
 */
bool
SGPropertyNode::isTied (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->isTied());
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<bool> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<int> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<long> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<float> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<double> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie (const char * relative_path,
		     const SGRawValue<const char *> &rawValue,
		     bool useDefault)
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}


/**
 * Attempt to untie another node reached by a relative path.
 */
bool
SGPropertyNode::untie (const char * relative_path)
{
  SGPropertyNode * node = getNode(relative_path);
  return (node == 0 ? false : node->untie());
}

void
SGPropertyNode::addChangeListener (SGPropertyChangeListener * listener,
                                   bool initial)
{
  if (_listeners == 0)
    _listeners = new vector<SGPropertyChangeListener*>;
  _listeners->push_back(listener);
  listener->register_property(this);
  if (initial)
    listener->valueChanged(this);
}

void
SGPropertyNode::removeChangeListener (SGPropertyChangeListener * listener)
{
  vector<SGPropertyChangeListener*>::iterator it =
    find(_listeners->begin(), _listeners->end(), listener);
  if (it != _listeners->end()) {
    _listeners->erase(it);
    listener->unregister_property(this);
    if (_listeners->empty()) {
      vector<SGPropertyChangeListener*>* tmp = _listeners;
      _listeners = 0;
      delete tmp;
    }
  }
}

void
SGPropertyNode::fireValueChanged ()
{
  fireValueChanged(this);
}

void
SGPropertyNode::fireChildAdded (SGPropertyNode * child)
{
  fireChildAdded(this, child);
}

void
SGPropertyNode::fireChildRemoved (SGPropertyNode * child)
{
  fireChildRemoved(this, child);
}

void
SGPropertyNode::fireValueChanged (SGPropertyNode * node)
{
  if (_listeners != 0) {
    for (unsigned int i = 0; i < _listeners->size(); i++) {
      (*_listeners)[i]->valueChanged(node);
    }
  }
  if (_parent != 0)
    _parent->fireValueChanged(node);
}

void
SGPropertyNode::fireChildAdded (SGPropertyNode * parent,
				SGPropertyNode * child)
{
  if (_listeners != 0) {
    for (unsigned int i = 0; i < _listeners->size(); i++) {
      (*_listeners)[i]->childAdded(parent, child);
    }
  }
  if (_parent != 0)
    _parent->fireChildAdded(parent, child);
}

void
SGPropertyNode::fireChildRemoved (SGPropertyNode * parent,
				  SGPropertyNode * child)
{
  if (_listeners != 0) {
    for (unsigned int i = 0; i < _listeners->size(); i++) {
      (*_listeners)[i]->childRemoved(parent, child);
    }
  }
  if (_parent != 0)
    _parent->fireChildRemoved(parent, child);
}



////////////////////////////////////////////////////////////////////////
// Simplified hash table for caching paths.
////////////////////////////////////////////////////////////////////////

#define HASH_TABLE_SIZE 199

SGPropertyNode::hash_table::entry::entry ()
  : _value(0)
{
}

SGPropertyNode::hash_table::entry::~entry ()
{
				// Don't delete the value; we don't own
				// the pointer.
}

void
SGPropertyNode::hash_table::entry::set_key (const char * key)
{
  _key = key;
}

void
SGPropertyNode::hash_table::entry::set_value (SGPropertyNode * value)
{
  _value = value;
}

SGPropertyNode::hash_table::bucket::bucket ()
  : _length(0),
    _entries(0)
{
}

SGPropertyNode::hash_table::bucket::~bucket ()
{
  for (int i = 0; i < _length; i++)
    delete _entries[i];
  delete [] _entries;
}

SGPropertyNode::hash_table::entry *
SGPropertyNode::hash_table::bucket::get_entry (const char * key, bool create)
{
  int i;
  for (i = 0; i < _length; i++) {
    if (!strcmp(_entries[i]->get_key(), key))
      return _entries[i];
  }
  if (create) {
    entry ** new_entries = new entry*[_length+1];
    for (i = 0; i < _length; i++) {
      new_entries[i] = _entries[i];
    }
    delete [] _entries;
    _entries = new_entries;
    _entries[_length] = new entry;
    _entries[_length]->set_key(key);
    _length++;
    return _entries[_length - 1];
  } else {
    return 0;
  }
}

void
SGPropertyNode::hash_table::bucket::erase (const char * key)
{
  int i;
  for (i = 0; i < _length; i++) {
    if (!strcmp(_entries[i]->get_key(), key))
       break;
  }

  if (i < _length) {
    for (++i; i < _length; i++) {
      _entries[i-1] = _entries[i];
    }
     _length--;
  }
}


SGPropertyNode::hash_table::hash_table ()
  : _data_length(0),
    _data(0)
{
}

SGPropertyNode::hash_table::~hash_table ()
{
  for (unsigned int i = 0; i < _data_length; i++)
    delete _data[i];
  delete [] _data;
}

SGPropertyNode *
SGPropertyNode::hash_table::get (const char * key)
{
  if (_data_length == 0)
    return 0;
  unsigned int index = hashcode(key) % _data_length;
  if (_data[index] == 0)
    return 0;
  entry * e = _data[index]->get_entry(key);
  if (e == 0)
    return 0;
  else
    return e->get_value();
}

void
SGPropertyNode::hash_table::put (const char * key, SGPropertyNode * value)
{
  if (_data_length == 0) {
    _data = new bucket*[HASH_TABLE_SIZE];
    _data_length = HASH_TABLE_SIZE;
    for (unsigned int i = 0; i < HASH_TABLE_SIZE; i++)
      _data[i] = 0;
  }
  unsigned int index = hashcode(key) % _data_length;
  if (_data[index] == 0) {
    _data[index] = new bucket;
  }
  entry * e = _data[index]->get_entry(key, true);
  e->set_value(value);
}

void
SGPropertyNode::hash_table::erase (const char * key)
{
   if (_data_length == 0)
    return;
  unsigned int index = hashcode(key) % _data_length;
  if (_data[index] == 0)
    return;
  _data[index]->erase(key);
}

unsigned int
SGPropertyNode::hash_table::hashcode (const char * key)
{
  unsigned int hash = 0;
  while (*key != 0) {
    hash = 31 * hash + *key;
    key++;
  }
  return hash;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGPropertyChangeListener.
////////////////////////////////////////////////////////////////////////

SGPropertyChangeListener::~SGPropertyChangeListener ()
{
				// This will come back and remove
				// the current item each time.  Is
				// that OK?
  vector<SGPropertyNode *>::iterator it;
  for (it = _properties.begin(); it != _properties.end(); it++)
    (*it)->removeChangeListener(this);
}

void
SGPropertyChangeListener::valueChanged (SGPropertyNode * node)
{
  // NO-OP
}

void
SGPropertyChangeListener::childAdded (SGPropertyNode * node,
				      SGPropertyNode * child)
{
  // NO-OP
}

void
SGPropertyChangeListener::childRemoved (SGPropertyNode * parent,
					SGPropertyNode * child)
{
  // NO-OP
}

void
SGPropertyChangeListener::register_property (SGPropertyNode * node)
{
  _properties.push_back(node);
}

void
SGPropertyChangeListener::unregister_property (SGPropertyNode * node)
{
  vector<SGPropertyNode *>::iterator it =
    find(_properties.begin(), _properties.end(), node);
  if (it != _properties.end())
    _properties.erase(it);
}


// end of props.cxx
