/**
 * \file props.hxx
 * Interface definition for a property list.
 * Started Fall 2000 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * See props.html for documentation [replace with URL when available].
 *
 * $Id: props.hxx,v 1.7 2011/11/19 14:14:57 bcoconni Exp $
 */

#ifndef __PROPS_HXX
#define __PROPS_HXX

#define PROPS_STANDALONE 1

#ifndef PROPS_STANDALONE
#define PROPS_STANDALONE 0
#endif

#include <vector>

#if PROPS_STANDALONE

  #include <string>
  #include <iostream>

  using std::string;
  using std::vector;
  using std::istream;
  using std::ostream;

  #include "simgear/structure/SGReferenced.hxx"
  #include "simgear/structure/SGSharedPtr.hxx"

#else

  #include <simgear/compiler.h>
  #include <simgear/debug/logstream.hxx>
  #include STL_STRING
  #include STL_IOSTREAM
  SG_USING_STD(string);
  SG_USING_STD(vector);
  SG_USING_STD(istream);
  SG_USING_STD(ostream);
  #include <simgear/structure/SGReferenced.hxx>
  #include <simgear/structure/SGSharedPtr.hxx>
#endif


#ifdef NONE
#pragma warn A sloppy coder has defined NONE as a macro!
#undef NONE
#endif

#ifdef ALIAS
#pragma warn A sloppy coder has defined ALIAS as a macro!
#undef ALIAS
#endif

#ifdef UNSPECIFIED
#pragma warn A sloppy coder has defined UNSPECIFIED as a macro!
#undef UNSPECIFIED
#endif

#ifdef BOOL
#pragma warn A sloppy coder has defined BOOL as a macro!
#undef BOOL
#endif

#ifdef INT
#pragma warn A sloppy coder has defined INT as a macro!
#undef INT
#endif

#ifdef LONG
#pragma warn A sloppy coder has defined LONG as a macro!
#undef LONG
#endif

#ifdef FLOAT
#pragma warn A sloppy coder has defined FLOAT as a macro!
#undef FLOAT
#endif

#ifdef DOUBLE
#pragma warn A sloppy coder has defined DOUBLE as a macro!
#undef DOUBLE
#endif

#ifdef STRING
#pragma warn A sloppy coder has defined STRING as a macro!
#undef STRING
#endif



////////////////////////////////////////////////////////////////////////
// A raw value.
//
// This is the mechanism that information-providing routines can
// use to link their own values to the property manager.  Any
// SGValue can be tied to a raw value and then untied again.
//
// Note: we are forced to use inlined methods here to ensure
// that the templates will be instantiated.  We're probably taking
// a small performance hit for that.
////////////////////////////////////////////////////////////////////////


/**
 * Abstract base class for a raw value.
 *
 * <p>The property manager is implemented in two layers.  The {@link
 * SGPropertyNode} is the highest and most abstract layer,
 * representing an LValue/RValue pair: it records the position of the
 * property in the property tree and contains facilities for
 * navigation to other nodes.  It is guaranteed to be persistent: the
 * {@link SGPropertyNode} will not change during a session, even if
 * the property is bound and unbound multiple times.</p>
 *
 * <p>When the property value is not managed internally in the
 * SGPropertyNode, the SGPropertyNode will contain a reference to an
 * SGRawValue (this class), which provides an abstract way to get,
 * set, and clone the underlying value.  The SGRawValue may change
 * frequently during a session as a value is retyped or bound and
 * unbound to various data source, but the abstract SGPropertyNode
 * layer insulates the application from those changes.  The raw value
 * contains no facilities for data binding or for type conversion: it
 * is simply the abstraction of a primitive data type (or a compound
 * data type, in the case of a string).</p>
 *
 * <p>The SGPropertyNode class always keeps a *copy* of a raw value,
 * not the original one passed to it; if you override a derived class
 * but do not replace the {@link #clone} method, strange things will
 * happen.</p>
 *
 * <p>All derived SGRawValue classes must implement {@link #getValue},
 * {@link #setValue}, and {@link #clone} for the appropriate type.</p>
 *
 * @see SGPropertyNode
 * @see SGRawValuePointer
 * @see SGRawValueFunctions
 * @see SGRawValueFunctionsIndexed
 * @see SGRawValueMethods
 * @see SGRawValueMethodsIndexed
 */
template <class T>
class SGRawValue
{
public:

  /**
   * The default underlying value for this type.
   *
   * Every raw value has a default; the default is false for a
   * boolean, 0 for the various numeric values, and "" for a string.
   * If additional types of raw values are added in the future, they
   * may need different kinds of default values (such as epoch for a
   * date type).  The default value is used when creating new values.
   */
  static const T DefaultValue;	// Default for this kind of raw value.


  /**
   * Constructor.
   *
   * Use the default value for this type.
   */
  SGRawValue () {}


  /**
   * Destructor.
   */
  virtual ~SGRawValue () {}


  /**
   * Return the underlying value.
   *
   * @return The actual value for the property.
   * @see #setValue
   */
  virtual T getValue () const = 0;


  /**
   * Assign a new underlying value.
   *
   * If the new value cannot be set (because this is a read-only
   * raw value, or because the new value is not acceptable for
   * some reason) this method returns false and leaves the original
   * value unchanged.
   *
   * @param value The actual value for the property.
   * @return true if the value was set successfully, false otherwise.
   * @see #getValue
   */
  virtual bool setValue (T value) = 0;


  /**
   * Create a new deep copy of this raw value.
   *
   * The copy will contain its own version of the underlying value
   * as well.
   *
   * @return A deep copy of the current object.
   */
  virtual SGRawValue * clone () const = 0;
};


/**
 * A raw value bound to a pointer.
 *
 * This is the most efficient way to tie an external value, but also
 * the most dangerous, because there is no way for the supplier to
 * perform bounds checking and derived calculations except by polling
 * the variable to see if it has changed.  There is no default
 * constructor, because this class would be meaningless without a
 * pointer.
 */
template <class T>
class SGRawValuePointer : public SGRawValue<T>
{
public:

  /**
   * Explicit pointer constructor.
   *
   * Create a new raw value bound to the value of the variable
   * referenced by the pointer.
   *
   * @param ptr The pointer to the variable to which this raw value
   * will be bound.
   */
  SGRawValuePointer (T * ptr) : _ptr(ptr) {}

  /**
   * Destructor.
   */
  virtual ~SGRawValuePointer () {}

  /**
   * Get the underlying value.
   *
   * This method will dereference the pointer and return the
   * variable's value.
   */
  virtual T getValue () const { return *_ptr; }

  /**
   * Set the underlying value.
   *
   * This method will dereference the pointer and change the
   * variable's value.
   */
  virtual bool setValue (T value) { *_ptr = value; return true; }

  /**
   * Create a copy of this raw value.
   *
   * The copy will use the same external pointer as the original.
   */
  virtual SGRawValue<T> * clone () const {
    return new SGRawValuePointer<T>(_ptr);
  }

private:
  T * _ptr;
};


/**
 * A value managed through static functions.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class T>
class SGRawValueFunctions : public SGRawValue<T>
{
public:

  /**
   * The template type of a static getter function.
   */
  typedef T (*getter_t)();

  /**
   * The template type of a static setter function.
   */
  typedef void (*setter_t)(T);

  /**
   * Explicit constructor.
   *
   * Create a new raw value bound to the getter and setter supplied.
   *
   * @param getter A static function for getting a value, or 0
   * to read-disable the value.
   * @param setter A static function for setting a value, or 0
   * to write-disable the value.
   */
  SGRawValueFunctions (getter_t getter = 0, setter_t setter = 0)
    : _getter(getter), _setter(setter) {}

  /**
   * Destructor.
   */
  virtual ~SGRawValueFunctions () {}

  /**
   * Get the underlying value.
   *
   * This method will invoke the getter function to get a value.
   * If no getter function was supplied, this method will always
   * return the default value for the type.
   */
  virtual T getValue () const {
    if (_getter) return (*_getter)();
    else return SGRawValue<T>::DefaultValue;
  }

  /**
   * Set the underlying value.
   *
   * This method will invoke the setter function to change the
   * underlying value.  If no setter function was supplied, this
   * method will return false.
   */
  virtual bool setValue (T value) {
    if (_setter) { (*_setter)(value); return true; }
    else return false;
  }

  /**
   * Create a copy of this raw value, bound to the same functions.
   */
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueFunctions<T>(_getter,_setter);
  }

private:
  getter_t _getter;
  setter_t _setter;
};


/**
 * An indexed value bound to static functions.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.  An indexed value is useful for binding one
 * of a list of possible values (such as multiple engines for a
 * plane).  The index is hard-coded at creation time.
 *
 * @see SGRawValue
 */
template <class T>
class SGRawValueFunctionsIndexed : public SGRawValue<T>
{
public:
  typedef T (*getter_t)(int);
  typedef void (*setter_t)(int,T);
  SGRawValueFunctionsIndexed (int index, getter_t getter = 0, setter_t setter = 0)
    : _index(index), _getter(getter), _setter(setter) {}
  virtual ~SGRawValueFunctionsIndexed () {}
  virtual T getValue () const {
    if (_getter) return (*_getter)(_index);
    else return SGRawValue<T>::DefaultValue;
  }
  virtual bool setValue (T value) {
    if (_setter) { (*_setter)(_index, value); return true; }
    else return false;
  }
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueFunctionsIndexed<T>(_index, _getter, _setter);
  }
private:
  int _index;
  getter_t _getter;
  setter_t _setter;
};


/**
 * A value managed through an object and access methods.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C, class T>
class SGRawValueMethods : public SGRawValue<T>
{
public:
  typedef T (C::*getter_t)() const;
  typedef void (C::*setter_t)(T);
  SGRawValueMethods (C &obj, getter_t getter = 0, setter_t setter = 0)
    : _obj(obj), _getter(getter), _setter(setter) {}
  virtual ~SGRawValueMethods () {}
  virtual T getValue () const {
    if (_getter) { return (_obj.*_getter)(); }
    else { return SGRawValue<T>::DefaultValue; }
  }
  virtual bool setValue (T value) {
    if (_setter) { (_obj.*_setter)(value); return true; }
    else return false;
  }
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueMethods<C,T>(_obj, _getter, _setter);
  }
private:
  C &_obj;
  getter_t _getter;
  setter_t _setter;
};


/**
 * An indexed value managed through an object and access methods.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C, class T>
class SGRawValueMethodsIndexed : public SGRawValue<T>
{
public:
  typedef T (C::*getter_t)(int) const;
  typedef void (C::*setter_t)(int, T);
  SGRawValueMethodsIndexed (C &obj, int index,
		     getter_t getter = 0, setter_t setter = 0)
    : _obj(obj), _index(index), _getter(getter), _setter(setter) {}
  virtual ~SGRawValueMethodsIndexed () {}
  virtual T getValue () const {
    if (_getter) { return (_obj.*_getter)(_index); }
    else { return SGRawValue<T>::DefaultValue; }
  }
  virtual bool setValue (T value) {
    if (_setter) { (_obj.*_setter)(_index, value); return true; }
    else return false;
  }
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueMethodsIndexed<C,T>(_obj, _index, _getter, _setter);
  }
private:
  C &_obj;
  int _index;
  getter_t _getter;
  setter_t _setter;
};


/**
 * The smart pointer that manage reference counting
 */
class SGPropertyNode;
typedef SGSharedPtr<SGPropertyNode> SGPropertyNode_ptr;
typedef SGSharedPtr<const SGPropertyNode> SGConstPropertyNode_ptr;


/**
 * The property change listener interface.
 *
 * <p>Any class that needs to listen for property changes must implement
 * this interface.</p>
 */
class SGPropertyChangeListener
{
public:
  virtual ~SGPropertyChangeListener ();
  virtual void valueChanged (SGPropertyNode * node);
  virtual void childAdded (SGPropertyNode * parent, SGPropertyNode * child);
  virtual void childRemoved (SGPropertyNode * parent, SGPropertyNode * child);

protected:
  friend class SGPropertyNode;
  virtual void register_property (SGPropertyNode * node);
  virtual void unregister_property (SGPropertyNode * node);

private:
  vector<SGPropertyNode *> _properties;
};



/**
 * A node in a property tree.
 */
class SGPropertyNode : public SGReferenced
{
public:

  /**
   * Public constants.
   */
  enum {
    MAX_STRING_LEN = 1024
  };

  /**
   * Property value types.
   */
  enum Type {
    NONE = 0,
    ALIAS,
    BOOL,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    STRING,
    UNSPECIFIED
  };


  /**
   * Access mode attributes.
   *
   * <p>The ARCHIVE attribute is strictly advisory, and controls
   * whether the property should normally be saved and restored.</p>
   */
  enum Attribute {
    READ = 1,
    WRITE = 2,
    ARCHIVE = 4,
    REMOVED = 8,
    TRACE_READ = 16,
    TRACE_WRITE = 32,
    USERARCHIVE = 64
  };


  /**
   * Last used attribute
   * Update as needed when enum Attribute is changed
   */
  static const int LAST_USED_ATTRIBUTE;


  /**
   * Default constructor.
   */
  SGPropertyNode ();


  /**
   * Copy constructor.
   */
  SGPropertyNode (const SGPropertyNode &node);


  /**
   * Destructor.
   */
  virtual ~SGPropertyNode ();



  //
  // Basic properties.
  //

  /**
   * Test whether this node contains a primitive leaf value.
   */
  bool hasValue () const { return (_type != NONE); }


  /**
   * Get the node's simple (XML) name.
   */
  const char * getName () const { return _name.c_str(); }


  /**
   * Get the node's pretty display name, with subscript when needed.
   */
  const char * getDisplayName (bool simplify = false) const;


  /**
   * Get the node's integer index.
   */
  int getIndex () const { return _index; }


  /**
   * Get a non-const pointer to the node's parent.
   */
  SGPropertyNode * getParent () { return _parent; }


  /**
   * Get a const pointer to the node's parent.
   */
  const SGPropertyNode * getParent () const { return _parent; }


  //
  // Children.
  //


  /**
   * Get the number of child nodes.
   */
  int nChildren () const { return (int)_children.size(); }


  /**
   * Get a child node by position (*NOT* index).
   */
  SGPropertyNode * getChild (int position);


  /**
   * Get a const child node by position (*NOT* index).
   */
  const SGPropertyNode * getChild (int position) const;


  /**
   * Test whether a named child exists.
   */
  bool hasChild (const char * name, int index = 0) const
  {
    return (getChild(name, index) != 0);
  }


  /**
   * Get a child node by name and index.
   */
  SGPropertyNode * getChild (const char * name, int index = 0,
			     bool create = false);


  /**
   * Get a const child node by name and index.
   */
  const SGPropertyNode * getChild (const char * name, int index = 0) const;


  /**
   * Get a vector of all children with the specified name.
   */
  vector<SGPropertyNode_ptr> getChildren (const char * name) const;


  /**
   * Remove child by position.
   */
  SGPropertyNode_ptr removeChild (int pos, bool keep = true);


  /**
   * Remove a child node
   */
  SGPropertyNode_ptr removeChild (const char * name, int index = 0,
                                  bool keep = true);

  /**
   * Remove all children with the specified name.
   */
  vector<SGPropertyNode_ptr> removeChildren (const char * name,
                                             bool keep = true);


  //
  // Alias support.
  //


  /**
   * Alias this node's leaf value to another's.
   */
  bool alias (SGPropertyNode * target);


  /**
   * Alias this node's leaf value to another's by relative path.
   */
  bool alias (const char * path);


  /**
   * Remove any alias for this node.
   */
  bool unalias ();


  /**
   * Test whether the node's leaf value is aliased to another's.
   */
  bool isAlias () const { return (_type == ALIAS); }


  /**
   * Get a non-const pointer to the current alias target, if any.
   */
  SGPropertyNode * getAliasTarget ();


  /**
   * Get a const pointer to the current alias target, if any.
   */
  const SGPropertyNode * getAliasTarget () const;


  //
  // Path information.
  //


  /**
   * Get the path to this node from the root.
   */
  const char * getPath (bool simplify = false) const;


  /**
   * Get a pointer to the root node.
   */
  SGPropertyNode * getRootNode ();


  /**
   * Get a const pointer to the root node.
   */
  const SGPropertyNode * getRootNode () const;


  /**
   * Get a pointer to another node by relative path.
   */
  SGPropertyNode * getNode (const char * relative_path, bool create = false);


  /**
   * Get a pointer to another node by relative path.
   *
   * This method leaves the index off the last member of the path,
   * so that the user can specify it separately (and save some
   * string building).  For example, getNode("/bar[1]/foo", 3) is
   * exactly equivalent to getNode("bar[1]/foo[3]").  The index
   * provided overrides any given in the path itself for the last
   * component.
   */
  SGPropertyNode * getNode (const char * relative_path, int index,
			    bool create = false);


  /**
   * Get a const pointer to another node by relative path.
   */
  const SGPropertyNode * getNode (const char * relative_path) const;


  /**
   * Get a const pointer to another node by relative path.
   *
   * This method leaves the index off the last member of the path,
   * so that the user can specify it separate.
   */
  const SGPropertyNode * getNode (const char * relative_path,
				  int index) const;


  //
  // Access Mode.
  //

  /**
   * Check a single mode attribute for the property node.
   */
  bool getAttribute (Attribute attr) const { return ((_attr & attr) != 0); }


  /**
   * Set a single mode attribute for the property node.
   */
  void setAttribute (Attribute attr, bool state) {
    (state ? _attr |= attr : _attr &= ~attr);
  }


  /**
   * Get all of the mode attributes for the property node.
   */
  int getAttributes () const { return _attr; }


  /**
   * Set all of the mode attributes for the property node.
   */
  void setAttributes (int attr) { _attr = attr; }


  //
  // Leaf Value (primitive).
  //


  /**
   * Get the type of leaf value, if any, for this node.
   */
  Type getType () const;


  /**
   * Get a bool value for this node.
   */
  bool getBoolValue () const;


  /**
   * Get an int value for this node.
   */
  int getIntValue () const;


  /**
   * Get a long int value for this node.
   */
  long getLongValue () const;


  /**
   * Get a float value for this node.
   */
  float getFloatValue () const;


  /**
   * Get a double value for this node.
   */
  double getDoubleValue () const;


  /**
   * Get a string value for this node.
   */
  const char * getStringValue () const;



  /**
   * Set a bool value for this node.
   */
  bool setBoolValue (bool value);


  /**
   * Set an int value for this node.
   */
  bool setIntValue (int value);


  /**
   * Set a long int value for this node.
   */
  bool setLongValue (long value);


  /**
   * Set a float value for this node.
   */
  bool setFloatValue (float value);


  /**
   * Set a double value for this node.
   */
  bool setDoubleValue (double value);


  /**
   * Set a string value for this node.
   */
  bool setStringValue (const char * value);


  /**
   * Set a value of unspecified type for this node.
   */
  bool setUnspecifiedValue (const char * value);


  //
  // Data binding.
  //


  /**
   * Test whether this node is bound to an external data source.
   */
  bool isTied () const { return _tied; }


  /**
   * Bind this node to an external bool source.
   */
  bool tie (const SGRawValue<bool> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external int source.
   */
  bool tie (const SGRawValue<int> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external long int source.
   */
  bool tie (const SGRawValue<long> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external float source.
   */
  bool tie (const SGRawValue<float> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external double source.
   */
  bool tie (const SGRawValue<double> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external string source.
   */
  bool tie (const SGRawValue<const char *> &rawValue, bool useDefault = true);


  /**
   * Unbind this node from any external data source.
   */
  bool untie ();


  //
  // Convenience methods using paths.
  // TODO: add attribute methods
  //


  /**
   * Get another node's type.
   */
  Type getType (const char * relative_path) const;


  /**
   * Test whether another node has a leaf value.
   */
  bool hasValue (const char * relative_path) const;


  /**
   * Get another node's value as a bool.
   */
  bool getBoolValue (const char * relative_path,
		     bool defaultValue = false) const;


  /**
   * Get another node's value as an int.
   */
  int getIntValue (const char * relative_path,
		   int defaultValue = 0) const;


  /**
   * Get another node's value as a long int.
   */
  long getLongValue (const char * relative_path,
		     long defaultValue = 0L) const;


  /**
   * Get another node's value as a float.
   */
  float getFloatValue (const char * relative_path,
		       float defaultValue = 0.0) const;


  /**
   * Get another node's value as a double.
   */
  double getDoubleValue (const char * relative_path,
			 double defaultValue = 0.0L) const;


  /**
   * Get another node's value as a string.
   */
  const char * getStringValue (const char * relative_path,
			       const char * defaultValue = "") const;


  /**
   * Set another node's value as a bool.
   */
  bool setBoolValue (const char * relative_path, bool value);


  /**
   * Set another node's value as an int.
   */
  bool setIntValue (const char * relative_path, int value);


  /**
   * Set another node's value as a long int.
   */
  bool setLongValue (const char * relative_path, long value);


  /**
   * Set another node's value as a float.
   */
  bool setFloatValue (const char * relative_path, float value);


  /**
   * Set another node's value as a double.
   */
  bool setDoubleValue (const char * relative_path, double value);


  /**
   * Set another node's value as a string.
   */
  bool setStringValue (const char * relative_path, const char * value);


  /**
   * Set another node's value with no specified type.
   */
  bool setUnspecifiedValue (const char * relative_path, const char * value);


  /**
   * Test whether another node is bound to an external data source.
   */
  bool isTied (const char * relative_path) const;


  /**
   * Bind another node to an external bool source.
   */
  bool tie (const char * relative_path, const SGRawValue<bool> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external int source.
   */
  bool tie (const char * relative_path, const SGRawValue<int> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external long int source.
   */
  bool tie (const char * relative_path, const SGRawValue<long> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external float source.
   */
  bool tie (const char * relative_path, const SGRawValue<float> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external double source.
   */
  bool tie (const char * relative_path, const SGRawValue<double> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external string source.
   */
  bool tie (const char * relative_path, const SGRawValue<const char *> &rawValue,
	    bool useDefault = true);


  /**
   * Unbind another node from any external data source.
   */
  bool untie (const char * relative_path);


  /**
   * Add a change listener to the property. If "initial" is set call the
   * listener initially.
   */
  void addChangeListener (SGPropertyChangeListener * listener,
                          bool initial = false);


  /**
   * Remove a change listener from the property.
   */
  void removeChangeListener (SGPropertyChangeListener * listener);


  /**
   * Fire a value change event to all listeners.
   */
  void fireValueChanged ();


  /**
   * Fire a child-added event to all listeners.
   */
  void fireChildAdded (SGPropertyNode * child);


  /**
   * Fire a child-removed event to all listeners.
   */
  void fireChildRemoved (SGPropertyNode * child);


  /**
   * Clear any existing value and set the type to NONE.
   */
  void clearValue ();


protected:

  void fireValueChanged (SGPropertyNode * node);
  void fireChildAdded (SGPropertyNode * parent, SGPropertyNode * child);
  void fireChildRemoved (SGPropertyNode * parent, SGPropertyNode * child);

  /**
   * Protected constructor for making new nodes on demand.
   */
  SGPropertyNode (const char * name, int index, SGPropertyNode * parent);


private:

				// Get the raw value
  bool get_bool () const;
  int get_int () const;
  long get_long () const;
  float get_float () const;
  double get_double () const;
  const char * get_string () const;

				// Set the raw value
  bool set_bool (bool value);
  bool set_int (int value);
  bool set_long (long value);
  bool set_float (float value);
  bool set_double (double value);
  bool set_string (const char * value);


  /**
   * Get the value as a string.
   */
  const char * make_string () const;


  /**
   * Trace a read access.
   */
  void trace_read () const;


  /**
   * Trace a write access.
   */
  void trace_write () const;


  class hash_table;

  int _index;
  string _name;
  mutable string _display_name;
  /// To avoid cyclic reference counting loops this shall not be a reference
  /// counted pointer
  SGPropertyNode * _parent;
  vector<SGPropertyNode_ptr> _children;
  vector<SGPropertyNode_ptr> _removedChildren;
  mutable string _path;
  mutable string _buffer;
  hash_table * _path_cache;
  Type _type;
  bool _tied;
  int _attr;

				// The right kind of pointer...
  union {
    SGPropertyNode * alias;
    SGRawValue<bool> * bool_val;
    SGRawValue<int> * int_val;
    SGRawValue<long> * long_val;
    SGRawValue<float> * float_val;
    SGRawValue<double> * double_val;
    SGRawValue<const char *> * string_val;
  } _value;

  union {
    bool bool_val;
    int int_val;
    long long_val;
    float float_val;
    double double_val;
    char * string_val;
  } _local_val;

  vector <SGPropertyChangeListener *> * _listeners;



  /**
   * A very simple hash table with no remove functionality.
   */
  class hash_table {
  public:

    /**
     * An entry in a bucket in a hash table.
     */
    class entry {
    public:
      entry ();
      ~entry ();
      const char * get_key () { return _key.c_str(); }
      void set_key (const char * key);
      SGPropertyNode * get_value () { return _value; }
      void set_value (SGPropertyNode * value);
    private:
      string _key;
      SGSharedPtr<SGPropertyNode>  _value;
    };


    /**
     * A bucket in a hash table.
     */
    class bucket {
    public:
      bucket ();
      ~bucket ();
      entry * get_entry (const char * key, bool create = false);
      void erase(const char * key);
    private:
      int _length;
      entry ** _entries;
    };

    friend class bucket;

    hash_table ();
    ~hash_table ();
    SGPropertyNode * get (const char * key);
    void put (const char * key, SGPropertyNode * value);
    void erase(const char * key);

  private:
    unsigned int hashcode (const char * key);
    unsigned int _data_length;
    bucket ** _data;
  };

};

#endif // __PROPS_HXX

// end of props.hxx
