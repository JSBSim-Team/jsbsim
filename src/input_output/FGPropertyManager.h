/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropertyManager.h
 Author:       Tony Peden
               Based on work originally by David Megginson
 Date:         2/2002

 ------------- Copyright (C) 2002 -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPERTYMANAGER_H
#define FGPROPERTYMANAGER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <iostream>
#include "simgear/props/props.hxx"

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPERTYMANAGER "$Id: FGPropertyManager.h,v 1.9 2008/07/22 02:42:17 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Class wrapper for property handling.
    @author David Megginson, Tony Peden
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropertyManager : public SGPropertyNode, public FGJSBBase
{
  private:
    static bool suppress_warning;
  public:
    /// Constructor
    FGPropertyManager(void) {suppress_warning = false;}
    /// Destructor
    virtual ~FGPropertyManager(void) {}

    /** Property-ify a name
     *  replaces spaces with '-' and, optionally, makes name all lower case
     *  @param name string to change
     *  @param lowercase true to change all upper case chars to lower
     *  NOTE: this function changes its argument and thus relies
     *  on pass by value
     */
    string mkPropertyName(string name, bool lowercase);

    /**
     * Get a property node.
     *
     * @param path The path of the node, relative to root.
     * @param create true to create the node if it doesn't exist.
     * @return The node, or 0 if none exists and none was created.
     */
    FGPropertyManager*
    GetNode (const string &path, bool create = false);

    FGPropertyManager*
    GetNode (const string &relpath, int index, bool create = false);

    /**
     * Test whether a given node exists.
     *
     * @param path The path of the node, relative to root.
     * @return true if the node exists, false otherwise.
     */
    bool HasNode (const string &path);

    /**
     * Get the name of a node
     */
    string GetName( void );

    /**
     * Get the name of a node without underscores, etc.
     */
    string GetPrintableName( void );

    /**
     * Get the fully qualified name of a node
     * This function is very slow, so is probably useful for debugging only.
     */
    string GetFullyQualifiedName(void);

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
    bool GetBool (const string &name, bool defaultValue = false);


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
    int GetInt (const string &name, int defaultValue = 0);


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
    int GetLong (const string &name, long defaultValue = 0L);


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
    float GetFloat (const string &name, float defaultValue = 0.0);


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
    double GetDouble (const string &name, double defaultValue = 0.0);


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
    string GetString (const string &name, string defaultValue = "");


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
    bool SetBool (const string &name, bool val);


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
    bool SetInt (const string &name, int val);


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
    bool SetLong (const string &name, long val);


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
    bool SetFloat (const string &name, float val);


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
    bool SetDouble (const string &name, double val);


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
    bool SetString (const string &name, const string &val);


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
    void SetArchivable (const string &name, bool state = true);


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
    void SetReadable (const string &name, bool state = true);


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
    void SetWritable (const string &name, bool state = true);


    ////////////////////////////////////////////////////////////////////////
    // Convenience functions for tying properties, with logging.
    ////////////////////////////////////////////////////////////////////////


    /**
     * Untie a property from an external data source.
     *
     * Classes should use this function to release control of any
     * properties they are managing.
     */
    void Untie (const string &name);


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
    void
    Tie (const string &name, bool *pointer, bool useDefault = true);


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
    void
    Tie (const string &name, int *pointer, bool useDefault = true);


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
    void
    Tie (const string &name, long *pointer, bool useDefault = true);


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
    void
    Tie (const string &name, float *pointer, bool useDefault = true);

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
    void
    Tie (const string &name, double *pointer, bool useDefault = true);

//============================================================================
//
//  All of the following functions *must* be inlined, otherwise linker
//  errors will result
//
//============================================================================

    /* template <class V> void
    Tie (const string &name, V (*getter)(), void (*setter)(V) = 0,
           bool useDefault = true);

    template <class V> void
    Tie (const string &name, int index, V (*getter)(int),
           void (*setter)(int, V) = 0, bool useDefault = true);

    template <class T, class V> void
    Tie (const string &name, T * obj, V (T::*getter)() const,
           void (T::*setter)(V) = 0, bool useDefault = true);

    template <class T, class V> void
    Tie (const string &name, T * obj, int index,
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

    template <class V> inline void
    Tie (const string &name, V (*getter)(), void (*setter)(V) = 0, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValueFunctions<V>(getter, setter), useDefault))
        cout << "Failed to tie property " << name << " to functions" << endl;
      else if (debug_lvl & 0x20)
        cout << name << endl;
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
     *        property value should there be one; false if the old value should be
     *        discarded; defaults to true.
     */
    template <class V> inline void Tie (const string &name, int index, V (*getter)(int),
                                void (*setter)(int, V) = 0, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValueFunctionsIndexed<V>(index, getter, setter), useDefault))
        cout << "Failed to tie property " << name << " to indexed functions" << endl;
      else if (debug_lvl & 0x20)
        cout << name << endl;
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
     *        property value should there be one; false if the old value should be
     *        discarded; defaults to true.
     */
    template <class T, class V> inline void
    Tie (const string &name, T * obj, V (T::*getter)() const,
           void (T::*setter)(V) = 0, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValueMethods<T,V>(*obj, getter, setter), useDefault))
        cout << "Failed to tie property " << name << " to object methods" << endl;
      else if (debug_lvl & 0x20)
        cout << name << endl;
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
    template <class T, class V> inline void
    Tie (const string &name, T * obj, int index, V (T::*getter)(int) const,
                         void (T::*setter)(int, V) = 0, bool useDefault = true)
    {
      if (!tie(name.c_str(), SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter), useDefault))
        cout << "Failed to tie property " << name << " to indexed object methods" << endl;
      else if (debug_lvl & 0x20)
        cout << name << endl;
   }
};
}
#endif // FGPROPERTYMANAGER_H

