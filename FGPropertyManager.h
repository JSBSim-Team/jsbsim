// FGPropertyManager.h
// Class wrapper for property handling.
// based on FlightGear source src/Main/fg_props.hxx 
// Originally written by David Megginson, started 2000.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef FGPROPERTYMANAGER_H
#define FGPROPERTYMANAGER_H

#include <simgear/misc/props.hxx>

using namespace std;

class FGPropertyManager:public SGPropertyNode {
  public:
    FGPropertyManager::FGPropertyManager(void) {
      
    }
    
    FGPropertyManager::~FGPropertyManager(void) {
      
    }   
    /**
     * Get a property node.
     *
     * @param path The path of the node, relative to root.
     * @param create true to create the node if it doesn't exist.
     * @return The node, or 0 if none exists and none was created.
     */
    inline FGPropertyManager* 
    GetNode (const string &path, bool create = false)
    {
      SGPropertyNode* node=this->getNode(path.c_str(), create);
      if(node == 0) 
        cout << "FGPropertyManager::GetNode() No node found for " 
             << path << endl;
      return (FGPropertyManager*)node;
    }
  
    inline FGPropertyManager* 
    GetNode (const string &relpath, int index, bool create = false)
    {
        return (FGPropertyManager*)getNode(relpath.c_str(),index,create);
    }    


    /**
     * Test whether a given node exists.
     *
     * @param path The path of the node, relative to root.
     * @return true if the node exists, false otherwise.
     */
    inline bool
    HasNode (const string &path)
    {
      return (GetNode(path, false) != 0);
    }


    /**
     * Get a bool value for a property.
     *
     * This method is convenient but inefficient.  It should be used
     * infrequently (i.e. for initializing, loading, saving, etc.),
     * not in the main loop.  If you need to get a value frequently,
     * it is better to look up the node itself using GetNode and then
     * use the node's getBoolValue() method, to avoid the lookup overhead.
     *
     * @param name The property name.
     * @param defaultValue The default value to return if the property
     *        does not exist.
     * @return The property's value as a bool, or the default value provided.
     */
    inline bool GetBool (const string &name, bool defaultValue = false)
    {
      return getBoolValue(name.c_str(), defaultValue);
    }


    /**
     * Get an int value for a property.
     *
     * This method is convenient but inefficient.  It should be used
     * infrequently (i.e. for initializing, loading, saving, etc.),
     * not in the main loop.  If you need to get a value frequently,
     * it is better to look up the node itself using GetNode and then
     * use the node's getIntValue() method, to avoid the lookup overhead.
     *
     * @param name The property name.
     * @param defaultValue The default value to return if the property
     *        does not exist.
     * @return The property's value as an int, or the default value provided.
     */
    inline int GetInt (const string &name, int defaultValue = 0)
    {
      return getIntValue(name.c_str(), defaultValue);
    }


    /**
     * Get a long value for a property.
     *
     * This method is convenient but inefficient.  It should be used
     * infrequently (i.e. for initializing, loading, saving, etc.),
     * not in the main loop.  If you need to get a value frequently,
     * it is better to look up the node itself using GetNode and then
     * use the node's getLongValue() method, to avoid the lookup overhead.
     *
     * @param name The property name.
     * @param defaultValue The default value to return if the property
     *        does not exist.
     * @return The property's value as a long, or the default value provided.
     */
    inline int GetLong (const string &name, long defaultValue = 0L)
    {
      return getLongValue(name.c_str(), defaultValue);
    }


    /**
     * Get a float value for a property.
     *
     * This method is convenient but inefficient.  It should be used
     * infrequently (i.e. for initializing, loading, saving, etc.),
     * not in the main loop.  If you need to get a value frequently,
     * it is better to look up the node itself using GetNode and then
     * use the node's getFloatValue() method, to avoid the lookup overhead.
     *
     * @param name The property name.
     * @param defaultValue The default value to return if the property
     *        does not exist.
     * @return The property's value as a float, or the default value provided.
     */
    inline float GetFloat (const string &name, float defaultValue = 0.0)
    {
      return getFloatValue(name.c_str(), defaultValue);
    }


    /**
     * Get a double value for a property.
     *
     * This method is convenient but inefficient.  It should be used
     * infrequently (i.e. for initializing, loading, saving, etc.),
     * not in the main loop.  If you need to get a value frequently,
     * it is better to look up the node itself using GetNode and then
     * use the node's getDoubleValue() method, to avoid the lookup overhead.
     *
     * @param name The property name.
     * @param defaultValue The default value to return if the property
     *        does not exist.
     * @return The property's value as a double, or the default value provided.
     */
    inline double GetDouble (const string &name, double defaultValue = 0.0)
    {
      return getDoubleValue(name.c_str(), defaultValue);
    }


    /**
     * Get a string value for a property.
     *
     * This method is convenient but inefficient.  It should be used
     * infrequently (i.e. for initializing, loading, saving, etc.),
     * not in the main loop.  If you need to get a value frequently,
     * it is better to look up the node itself using GetNode and then
     * use the node's getStringValue() method, to avoid the lookup overhead.
     *
     * @param name The property name.
     * @param defaultValue The default value to return if the property
     *        does not exist.
     * @return The property's value as a string, or the default value provided.
     */
    inline string GetString (const string &name, string defaultValue = "")
    {
      return string(getStringValue(name.c_str(), defaultValue.c_str()));
    }


    /**
     * Set a bool value for a property.
     *
     * Assign a bool value to a property.  If the property does not
     * yet exist, it will be created and its type will be set to
     * BOOL; if it has a type of UNKNOWN, the type will also be set to
     * BOOL; otherwise, the bool value will be converted to the property's
     * type.
     *
     * @param name The property name.
     * @param val The new value for the property.
     * @return true if the assignment succeeded, false otherwise.
     */
    inline bool SetBool (const string &name, bool val)
    {
      return setBoolValue(name.c_str(), val);
    }


    /**
     * Set an int value for a property.
     *
     * Assign an int value to a property.  If the property does not
     * yet exist, it will be created and its type will be set to
     * INT; if it has a type of UNKNOWN, the type will also be set to
     * INT; otherwise, the bool value will be converted to the property's
     * type.
     *
     * @param name The property name.
     * @param val The new value for the property.
     * @return true if the assignment succeeded, false otherwise.
     */
    inline bool SetInt (const string &name, int val)
    {
      return setIntValue(name.c_str(), val);
    }


    /**
     * Set a long value for a property.
     *
     * Assign a long value to a property.  If the property does not
     * yet exist, it will be created and its type will be set to
     * LONG; if it has a type of UNKNOWN, the type will also be set to
     * LONG; otherwise, the bool value will be converted to the property's
     * type.
     *
     * @param name The property name.
     * @param val The new value for the property.
     * @return true if the assignment succeeded, false otherwise.
     */
    inline bool SetLong (const string &name, long val)
    {
      return setLongValue(name.c_str(), val);
    }


    /**
     * Set a float value for a property.
     *
     * Assign a float value to a property.  If the property does not
     * yet exist, it will be created and its type will be set to
     * FLOAT; if it has a type of UNKNOWN, the type will also be set to
     * FLOAT; otherwise, the bool value will be converted to the property's
     * type.
     *
     * @param name The property name.
     * @param val The new value for the property.
     * @return true if the assignment succeeded, false otherwise.
     */
    inline bool SetFloat (const string &name, float val)
    {
      return setFloatValue(name.c_str(), val);
    }


    /**
     * Set a double value for a property.
     *
     * Assign a double value to a property.  If the property does not
     * yet exist, it will be created and its type will be set to
     * DOUBLE; if it has a type of UNKNOWN, the type will also be set to
     * DOUBLE; otherwise, the double value will be converted to the property's
     * type.
     *
     * @param name The property name.
     * @param val The new value for the property.
     * @return true if the assignment succeeded, false otherwise.
     */
    inline bool SetDouble (const string &name, double val)
    {
      return setDoubleValue(name.c_str(), val);
    }


    /**
     * Set a string value for a property.
     *
     * Assign a string value to a property.  If the property does not
     * yet exist, it will be created and its type will be set to
     * STRING; if it has a type of UNKNOWN, the type will also be set to
     * STRING; otherwise, the string value will be converted to the property's
     * type.
     *
     * @param name The property name.
     * @param val The new value for the property.
     * @return true if the assignment succeeded, false otherwise.
     */
    inline bool SetString (const string &name, const string &val)
    {
      return setStringValue(name.c_str(), val.c_str());
    }


    
    ////////////////////////////////////////////////////////////////////////
    // Convenience functions for setting property attributes.
    ////////////////////////////////////////////////////////////////////////


    /**
     * Set the state of the archive attribute for a property.
     *
     * If the archive attribute is true, the property will be written
     * when a flight is saved; if it is false, the property will be
     * skipped.
     *
     * A warning message will be printed if the property does not exist.
     *
     * @param name The property name.
     * @param state The state of the archive attribute (defaults to true).
     */
    inline void
    SetArchivable (const string &name, bool state = true)
    {
      SGPropertyNode * node = getNode(name.c_str());
      if (node == 0)
        cout <<
	       "Attempt to set archive flag for non-existant property "
	       << name;
      else
        node->setAttribute(SGPropertyNode::ARCHIVE, state);
    }


    /**
     * Set the state of the read attribute for a property.
     *
     * If the read attribute is true, the property value will be readable;
     * if it is false, the property value will always be the default value
     * for its type.
     *
     * A warning message will be printed if the property does not exist.
     *
     * @param name The property name.
     * @param state The state of the read attribute (defaults to true).
     */
    inline void
    SetReadable (const string &name, bool state = true)
    {
      SGPropertyNode * node = getNode(name.c_str());
      if (node == 0)
        cout <<
	       "Attempt to set read flag for non-existant property "
	       << name;
      else
        node->setAttribute(SGPropertyNode::READ, state);
    }


    /**
     * Set the state of the write attribute for a property.
     *
     * If the write attribute is true, the property value may be modified
     * (depending on how it is tied); if the write attribute is false, the
     * property value may not be modified.
     *
     * A warning message will be printed if the property does not exist.
     *
     * @param name The property name.
     * @param state The state of the write attribute (defaults to true).
     */
    inline void
    SetWritable (const string &name, bool state = true)
    {
      SGPropertyNode * node = getNode(name.c_str());
      if (node == 0)
        cout <<
	       "Attempt to set write flag for non-existant property "
	       << name;
      else
        node->setAttribute(SGPropertyNode::WRITE, state);
    }


    
    ////////////////////////////////////////////////////////////////////////
    // Convenience functions for tying properties, with logging.
    ////////////////////////////////////////////////////////////////////////


    /**
     * Untie a property from an external data source.
     *
     * Classes should use this function to release control of any
     * properties they are managing.
     */
    inline void
    Untie (const string &name)
    {
      if (!untie(name.c_str()))
        cout << "Failed to untie property " << name;
    }


				    // Templates cause ambiguity here

    /**
     * Tie a property to an external bool variable.
     *
     * The property's value will automatically mirror the variable's
     * value, and vice-versa, until the property is untied.
     *
     * @param name The property name to tie (full path).
     * @param pointer A pointer to the variable.
     * @param useDefault true if any existing property value should be
     *        copied to the variable; false if the variable should not
     *        be modified; defaults to true.
     */
    inline void
    Tie (const string &name, bool *pointer, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValuePointer<bool>(pointer),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to a pointer";
    }


    /**
     * Tie a property to an external int variable.
     *
     * The property's value will automatically mirror the variable's
     * value, and vice-versa, until the property is untied.
     *
     * @param name The property name to tie (full path).
     * @param pointer A pointer to the variable.
     * @param useDefault true if any existing property value should be
     *        copied to the variable; false if the variable should not
     *        be modified; defaults to true.
     */
    inline void
    Tie (const string &name, int *pointer, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValuePointer<int>(pointer),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to a pointer";
    }


    /**
     * Tie a property to an external long variable.
     *
     * The property's value will automatically mirror the variable's
     * value, and vice-versa, until the property is untied.
     *
     * @param name The property name to tie (full path).
     * @param pointer A pointer to the variable.
     * @param useDefault true if any existing property value should be
     *        copied to the variable; false if the variable should not
     *        be modified; defaults to true.
     */
    inline void
    Tie (const string &name, long *pointer, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValuePointer<long>(pointer),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to a pointer";
    }


    /**
     * Tie a property to an external float variable.
     *
     * The property's value will automatically mirror the variable's
     * value, and vice-versa, until the property is untied.
     *
     * @param name The property name to tie (full path).
     * @param pointer A pointer to the variable.
     * @param useDefault true if any existing property value should be
     *        copied to the variable; false if the variable should not
     *        be modified; defaults to true.
     */
    inline void
    Tie (const string &name, float *pointer, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValuePointer<float>(pointer),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to a pointer";
    }


    /**
     * Tie a property to an external double variable.
     *
     * The property's value will automatically mirror the variable's
     * value, and vice-versa, until the property is untied.
     *
     * @param name The property name to tie (full path).
     * @param pointer A pointer to the variable.
     * @param useDefault true if any existing property value should be
     *        copied to the variable; false if the variable should not
     *        be modified; defaults to true.
     */
    inline void
    Tie (const string &name, double *pointer, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValuePointer<double>(pointer),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to a pointer";
    }

    /* template <class V> void
    FGPropertyManager::Tie (const string &name, V (*getter)(), void (*setter)(V) = 0,
           bool useDefault = true);
    
    template <class V> void
    FGPropertyManager::Tie (const string &name, int index, V (*getter)(int),
           void (*setter)(int, V) = 0, bool useDefault = true);
    
    template <class T, class V> void
    FGPropertyManager::Tie (const string &name, T * obj, V (T::*getter)() const,
           void (T::*setter)(V) = 0, bool useDefault = true);

    template <class T, class V> void 
    FGPropertyManager::Tie (const string &name, T * obj, int index,
           V (T::*getter)(int) const, void (T::*setter)(int, V) = 0,
           bool useDefault = true); */

/**
     * Tie a property to a pair of simple functions.
     *
     * Every time the property value is queried, the getter (if any) will
     * be invoked; every time the property value is modified, the setter
     * (if any) will be invoked.  The getter can be 0 to make the property
     * unreadable, and the setter can be 0 to make the property
     * unmodifiable.
     *
     * @param name The property name to tie (full path).
     * @param getter The getter function, or 0 if the value is unreadable.
     * @param setter The setter function, or 0 if the value is unmodifiable.
     * @param useDefault true if the setter should be invoked with any existing 
     *        property value should be; false if the old value should be
     *        discarded; defaults to true.
     */
    template <class V>
    inline void
    FGPropertyManager::Tie (const string &name, V (*getter)(), void (*setter)(V) = 0,
           bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValueFunctions<V>(getter, setter),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to functions";
    }


    /**
     * Tie a property to a pair of indexed functions.
     *
     * Every time the property value is queried, the getter (if any) will
     * be invoked with the index provided; every time the property value
     * is modified, the setter (if any) will be invoked with the index
     * provided.  The getter can be 0 to make the property unreadable, and
     * the setter can be 0 to make the property unmodifiable.
     *
     * @param name The property name to tie (full path).
     * @param index The integer argument to pass to the getter and
     *        setter functions.
     * @param getter The getter function, or 0 if the value is unreadable.
     * @param setter The setter function, or 0 if the value is unmodifiable.
     * @param useDefault true if the setter should be invoked with any existing 
     *        property value should be; false if the old value should be
     *        discarded; defaults to true.
     */
    template <class V>
    inline void
    FGPropertyManager::Tie (const string &name, int index, V (*getter)(int),
           void (*setter)(int, V) = 0, bool useDefault = true)
    {
      if (!tie(name.c_str(),
				     SGRawValueFunctionsIndexed<V>(index,
							           getter,
							           setter),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to indexed functions";
    }


    /**
     * Tie a property to a pair of object methods.
     *
     * Every time the property value is queried, the getter (if any) will
     * be invoked; every time the property value is modified, the setter
     * (if any) will be invoked.  The getter can be 0 to make the property
     * unreadable, and the setter can be 0 to make the property
     * unmodifiable.
     *
     * @param name The property name to tie (full path).
     * @param obj The object whose methods should be invoked.
     * @param getter The object's getter method, or 0 if the value is
     *        unreadable.
     * @param setter The object's setter method, or 0 if the value is
     *        unmodifiable.
     * @param useDefault true if the setter should be invoked with any existing 
     *        property value should be; false if the old value should be
     *        discarded; defaults to true.
     */
    template <class T, class V>
    inline void
    FGPropertyManager::Tie (const string &name, T * obj, V (T::*getter)() const,
           void (T::*setter)(V) = 0, bool useDefault = true)
    {
      if (!tie(name.c_str(),
				     SGRawValueMethods<T,V>(*obj, getter, setter),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to object methods";
    }


    /**
     * Tie a property to a pair of indexed object methods.
     *
     * Every time the property value is queried, the getter (if any) will
     * be invoked with the index provided; every time the property value
     * is modified, the setter (if any) will be invoked with the index
     * provided.  The getter can be 0 to make the property unreadable, and
     * the setter can be 0 to make the property unmodifiable.
     *
     * @param name The property name to tie (full path).
     * @param obj The object whose methods should be invoked.
     * @param index The integer argument to pass to the getter and
     *        setter methods.
     * @param getter The getter method, or 0 if the value is unreadable.
     * @param setter The setter method, or 0 if the value is unmodifiable.
     * @param useDefault true if the setter should be invoked with any existing 
     *        property value should be; false if the old value should be
     *        discarded; defaults to true.
     */
    template <class T, class V>
    inline void 
    FGPropertyManager::Tie (const string &name, T * obj, int index,
           V (T::*getter)(int) const, void (T::*setter)(int, V) = 0,
           bool useDefault = true)
    {
      if (!tie(name.c_str(),
				     SGRawValueMethodsIndexed<T,V>(*obj,
							           index,
							           getter,
							           setter),
				     useDefault))
        cout <<
	       "Failed to tie property " << name << " to indexed object methods";
    }

};        


#endif // FGPROPERTYMANAGER_H

