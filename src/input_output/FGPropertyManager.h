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

// This is needed by MSVC9 when included in FlightGear because of
// the new Vec4d class in props.hxx
#if defined( HAVE_CONFIG_H )
# include <config.h>
#endif

#include <string>
#include <list>
#include <memory>
#include <type_traits>
#include "simgear/props/props.hxx"
#if !PROPS_STANDALONE
# include "simgear/math/SGMath.hxx"
#endif

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

template <class C, class T>
class SGRawValueMethodsEnum : public SGRawValue<int>
{
public:
  typedef T(C::* getter_t)() const;
  typedef void (C::* setter_t)(T);
  SGRawValueMethodsEnum(C& obj,
    getter_t getter = nullptr, setter_t setter = nullptr)
    : _obj(obj), _getter(getter), _setter(setter) {}
  int getValue() const override {
    if (_getter) { return static_cast<int>((_obj.*_getter)()); }
    else { return SGRawValue<int>::DefaultValue(); }
  }
  bool setValue(int value) override {
    if (_setter) { (_obj.*_setter)(static_cast<T>(value)); return true; }
    else return false;
  }
  SGRaw* clone() const override {
    return new SGRawValueMethodsEnum(_obj, _getter, _setter);
  }
private:
  C& _obj;
  getter_t _getter;
  setter_t _setter;
};

template <class C, class T, class U>
class SGRawValueMethodsIndexedEnum : public SGRawValue<T>
{
public:
  typedef T(C::* getter_t)(U) const;
  typedef void (C::* setter_t)(U, T);
  SGRawValueMethodsIndexedEnum(C& obj, U index,
    getter_t getter = nullptr, setter_t setter = nullptr)
    : _obj(obj), _index(index), _getter(getter), _setter(setter) {}
  T getValue() const override {
    if (_getter) { return (_obj.*_getter)(_index); }
    else { return SGRawValue<T>::DefaultValue(); }
  }
  bool setValue(T value) override {
    if (_setter) { (_obj.*_setter)(_index, value); return true; }
    else return false;
  }
  SGRaw* clone() const override {
    return new SGRawValueMethodsIndexedEnum(_obj, _index, _getter, _setter);
  }
private:
  C& _obj;
  U _index;
  getter_t _getter;
  setter_t _setter;
};

namespace JSBSim {

JSBSIM_API std::string GetPrintableName(const SGPropertyNode* node);
JSBSIM_API std::string GetFullyQualifiedName(const SGPropertyNode* node);
JSBSIM_API std::string GetRelativeName(const SGPropertyNode* node, const std::string &path);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Class wrapper for property handling.
    @author David Megginson, Tony Peden
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGPropertyManager
{
  public:
    /// Default constructor
    FGPropertyManager(void) { root = new SGPropertyNode; }

    /// Constructor
    explicit FGPropertyManager(SGPropertyNode* _root) : root(_root) {};

    /// Destructor
    virtual ~FGPropertyManager(void) { Unbind(); }

    SGPropertyNode* GetNode(void) const { return root; }
    SGPropertyNode* GetNode(const std::string &path, bool create = false)
    { return root->getNode(path, create); }
    SGPropertyNode* GetNode(const std::string &relpath, int index, bool create = false)
    { return root->getNode(relpath, index, create); }
    bool HasNode(const std::string& path) const
    {
      std::string newPath = path;
      if (newPath[0] == '-') newPath.erase(0,1);
      SGPropertyNode* prop = root->getNode(newPath);
      return prop != nullptr;
    }

    /** Property-ify a name
     *  replaces spaces with '-' and, optionally, makes name all lower case
     *  @param name string to change
     *  @param lowercase true to change all upper case chars to lower
     *  NOTE: this function changes its argument and thus relies
     *  on pass by value
     */
    std::string mkPropertyName(std::string name, bool lowercase);

    ////////////////////////////////////////////////////////////////////////
    // Convenience functions for tying properties, with logging.
    ////////////////////////////////////////////////////////////////////////


    /**
     * Untie a property from an external data source.
     *
     * Classes should use this function to release control of any
     * properties they are managing.
     *
     * @param name The property name to untie (full path).
     */
    void Untie (const std::string &name);

    /**
     * Untie a property from an external data source.
     *
     * Classes should use this function to release control of any
     * properties they are managing.
     *
     * @param property A pointer to the property to untie.
     */
    void Untie (SGPropertyNode* property);

    /// Unbind all properties bound by this manager to an external data source.
    void Unbind (void);

    /**
     * Unbind all properties bound by this manager to an instance.
     *
     * Classes should use this function to release control of any
     * properties they have bound using this property manager.
     * @param instance The instance which properties shall be unbound.
     */
    void Unbind(const void* instance);

    /**
     * Unbind all properties bound by this manager to an instance.
     *
     * Classes should use this function to release control of any
     * properties they have bound using this property manager.
     * Helper function for shared_ptr
     * @see Unbind(const void*)
     */
    template <typename T> void Unbind(const std::shared_ptr<T>& instance) {
      Unbind(instance.get());
    }

    /**
     * Tie a property to an external variable.
     *
     * The property's value will automatically mirror the variable's
     * value, and vice-versa, until the property is untied.
     *
     * @param name The property name to tie (full path).
     * @param pointer A pointer to the variable.
     */
    template <typename T> void
    Tie (const std::string &name, T *pointer)
    {
      SGPropertyNode* property = root->getNode(name.c_str(), true);
      if (!property) {
        std::cerr << "Could not get or create property " << name << std::endl;
        return;
      }

      if (!property->tie(SGRawValuePointer<T>(pointer), false))
          std::cerr << "Failed to tie property " << name << " to a pointer" << std::endl;
      else {
        tied_properties.push_back(PropertyState(property, nullptr));
        if (FGJSBBase::debug_lvl & 0x20) std::cout << name << std::endl;
      }
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
     */
    template <class T, class V>
    typename std::enable_if_t<std::is_enum_v<V>, void>
    Tie (const std::string &name, T * obj, V (T::*getter)() const,
         void (T::*setter)(V) = nullptr)
    {
      SGPropertyNode* property = root->getNode(name.c_str(), true);
      if (!property) {
        std::cerr << "Could not get or create property " << name << std::endl;
        return;
      }

      if (!property->tie(SGRawValueMethodsEnum<T,V>(*obj, getter, setter), false))
        std::cerr << "Failed to tie property " << name << " to object methods"
                  << std::endl;
      else {
        tied_properties.push_back(PropertyState(property, obj));
        if (!setter) property->setAttribute(SGPropertyNode::WRITE, false);
        if (!getter) property->setAttribute(SGPropertyNode::READ, false);
        if (FGJSBBase::debug_lvl & 0x20) std::cout << name << std::endl;
      }
    }

    template <class T, class V>
    typename std::enable_if_t<!std::is_enum_v<V>, void>
    Tie (const std::string &name, T * obj, V (T::*getter)() const,
         void (T::*setter)(V) = nullptr)
    {
      SGPropertyNode* property = root->getNode(name.c_str(), true);
      if (!property) {
        std::cerr << "Could not get or create property " << name << std::endl;
        return;
      }

      if (!property->tie(SGRawValueMethods<T,V>(*obj, getter, setter), false))
        std::cerr << "Failed to tie property " << name << " to object methods"
                  << std::endl;
      else {
        tied_properties.push_back(PropertyState(property, obj));
        if (!setter) property->setAttribute(SGPropertyNode::WRITE, false);
        if (!getter) property->setAttribute(SGPropertyNode::READ, false);
        if (FGJSBBase::debug_lvl & 0x20) std::cout << name << std::endl;
      }
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
     */
    template <class T, class V> void
    Tie (const std::string &name, T * obj, int index, V (T::*getter)(int) const,
         void (T::*setter)(int, V) = nullptr)
    {
      SGPropertyNode* property = root->getNode(name.c_str(), true);
      if (!property) {
        std::cerr << "Could not get or create property " << name << std::endl;
        return;
      }

      if (!property->tie(SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter),
                                                       false))
        std::cerr << "Failed to tie property " << name
                  << " to indexed object methods" << std::endl;
      else {
        tied_properties.push_back(PropertyState(property, obj));
        if (!setter) property->setAttribute(SGPropertyNode::WRITE, false);
        if (!getter) property->setAttribute(SGPropertyNode::READ, false);
        if (FGJSBBase::debug_lvl & 0x20) std::cout << name << std::endl;
      }
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
     * @param index The enum argument to pass to the getter and
     *        setter methods.
     * @param getter The getter method, or 0 if the value is unreadable.
     * @param setter The setter method, or 0 if the value is unmodifiable.
     */
    template <class T, class V, class U>
    typename std::enable_if_t<std::is_enum_v<U>, void>
    Tie(const std::string& name, T* obj, U index, V(T::* getter)(U) const,
        void (T::* setter)(U, V) = nullptr)
    {
      SGPropertyNode* property = root->getNode(name.c_str(), true);
      if (!property) {
        std::cerr << "Could not get or create property " << name << std::endl;
        return;
      }
      if (!property->tie(SGRawValueMethodsIndexedEnum<T, V, U>(*obj, index, getter, setter),
        false))
        std::cerr << "Failed to tie property " << name
        << " to indexed object methods" << std::endl;
      else {
        tied_properties.push_back(PropertyState(property, obj));
        if (!setter) property->setAttribute(SGPropertyNode::WRITE, false);
        if (!getter) property->setAttribute(SGPropertyNode::READ, false);
        if (FGJSBBase::debug_lvl & 0x20) std::cout << name << std::endl;
      }
    }

  private:
    struct PropertyState {
      SGPropertyNode_ptr node;
      const void* BindingInstance = nullptr;
      bool WriteAttribute = true;
      bool ReadAttribute = true;
      PropertyState(SGPropertyNode* property, const void* instance)
        : node(property), BindingInstance(instance) {
        WriteAttribute = node->getAttribute(SGPropertyNode::WRITE);
        ReadAttribute = node->getAttribute(SGPropertyNode::READ);
      }
      void untie(void) {
        node->setAttribute(SGPropertyNode::WRITE, WriteAttribute);
        node->setAttribute(SGPropertyNode::READ, ReadAttribute);
        node->untie();
      }
    };
    std::list<PropertyState> tied_properties;
    SGPropertyNode_ptr root;
};
}
#endif // FGPROPERTYMANAGER_H
