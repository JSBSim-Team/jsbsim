// Copyright (C) 2010  James Turner - zakalawe@mac.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SG_PROPERTY_OBJECT
#define SG_PROPERTY_OBJECT

#include <simgear/props/props.hxx>

namespace simgear
{

class PropertyObjectBase
{
public:
  static void setDefaultRoot(SGPropertyNode* aRoot);
  
  PropertyObjectBase();
  
  PropertyObjectBase(const PropertyObjectBase& aOther);
    
  PropertyObjectBase(const char* aChild);
  
  PropertyObjectBase(SGPropertyNode* aNode, const char* aChild = NULL);
  
  SGPropertyNode* node(bool aCreate) const;

  /**
   * Resolve the property node, or throw an exception if it could not
   * be resolved.
   */
  SGPropertyNode* getOrThrow() const;
protected:
  mutable const char* _path;

  /**
   * Important - if _path is NULL, this is the actual prop.
   * If path is non-NULL, this is the parent which path should be resolved
   * against (or NULL, if _path is absolute). Use node() instead of accessing
   * this directly, and the above is handled automatically. 
   */
  mutable SGPropertyNode* _prop;
};

template <typename T>
class PropertyObject : PropertyObjectBase
{
public:
  PropertyObject()
  {}
  
  /**
   * Create from path relative to the default root, and option default value
   */
  explicit PropertyObject(const char* aChild) :
    PropertyObjectBase(aChild)
  { }
  
  /**
   * Create from a node, with optional relative path
   */
  explicit PropertyObject(SGPropertyNode* aNode, const char* aChild = NULL) :
    PropertyObjectBase(aNode, aChild)
  {
  
  }
  
// copy-constructor
  PropertyObject(const PropertyObject<T>& aOther) :
    PropertyObjectBase(aOther)
  {
  }

// create() form creates the property immediately
  static PropertyObject<T> create(const char* aPath, T aValue)
  {
    PropertyObject<T> p(aPath);
    p = aValue;
    return p;
  }
  
  static PropertyObject<T> create(SGPropertyNode* aNode, T aValue)
  {
    PropertyObject<T> p(aNode);
    p = aValue;
    return p;
  }

  static PropertyObject<T> create(SGPropertyNode* aNode, const char* aChild, T aValue)
  {
    PropertyObject<T> p(aNode, aChild);
    p = aValue;
    return p;
  }
  
// conversion operators
  operator T () const
  {
    return getOrThrow()->template getValue<T>();
  }

  T operator=(const T& aValue)
  {
    SGPropertyNode* n = PropertyObjectBase::node(true);
    if( !n )
      return aValue;

    n->setValue<T>(aValue);
    return aValue;
  }

#define SG_DEF_ASSIGN_OP(op)\
  T operator op##=(const T rhs)\
  {\
    SGPropertyNode* n = getOrThrow();\
    T new_val = n->getValue<T>() op rhs;\
    n->setValue<T>(new_val);\
    return new_val;\
  }

  SG_DEF_ASSIGN_OP(+)
  SG_DEF_ASSIGN_OP(-)
  SG_DEF_ASSIGN_OP(*)
  SG_DEF_ASSIGN_OP(/)
  SG_DEF_ASSIGN_OP(%)
  SG_DEF_ASSIGN_OP(>>)
  SG_DEF_ASSIGN_OP(<<)
  SG_DEF_ASSIGN_OP(&)
  SG_DEF_ASSIGN_OP(^)
  SG_DEF_ASSIGN_OP(|)

#undef SG_DEF_ASSIGN_OP

  SGPropertyNode* node(bool aCreate = false) const
  {
    return PropertyObjectBase::node(aCreate);
  }
}; // of template PropertyObject


// specialization for const char* / std::string

template <>
class PropertyObject<std::string> : PropertyObjectBase
{
public:
  explicit PropertyObject(const char* aChild) :
    PropertyObjectBase(aChild)
  { }
  

  
  explicit PropertyObject(SGPropertyNode* aNode, const char* aChild = NULL) :
    PropertyObjectBase(aNode, aChild)
  {
  
  }
  
// copy-constructor
  PropertyObject(const PropertyObject<std::string>& aOther) :
    PropertyObjectBase(aOther)
  {
  }

// create form
  static PropertyObject<std::string> create(const char* aPath, const std::string& aValue)
  {
    PropertyObject<std::string> p(aPath);
    p = aValue;
    return p;
  }
  
  static PropertyObject<std::string> create(SGPropertyNode* aNode, const std::string& aValue)
  {
    PropertyObject<std::string> p(aNode);
    p = aValue;
    return p;
  }

  static PropertyObject<std::string> create(SGPropertyNode* aNode, const char* aChild, const std::string& aValue)
  {
    PropertyObject<std::string> p(aNode, aChild);
    p = aValue;
    return p;
  }
  
  
  operator std::string () const
  {
    return getOrThrow()->getStringValue();
  }
  
  const char* operator=(const char* aValue)
  {
    SGPropertyNode* n = PropertyObjectBase::node(true);
    if (!n) {
      return aValue;
    }
    
    n->setStringValue(aValue);
    return aValue;
  }
  
  std::string operator=(const std::string& aValue)
  {
    SGPropertyNode* n = PropertyObjectBase::node(true);
    if (!n) {
      return aValue;
    }
    
    n->setStringValue(aValue);
    return aValue;
  }
  
  bool operator==(const char* value) const
  {
    std::string s(*this);
    return (s == value);    
  }

  bool operator==(const std::string& value) const
  {
    std::string s(*this);
    return (s == value);    
  }

  SGPropertyNode* node(bool aCreate = false) const
  {
    return PropertyObjectBase::node(aCreate);
  }
private:
};

} // of namespace simgear

typedef simgear::PropertyObject<double> SGPropObjDouble;
typedef simgear::PropertyObject<bool> SGPropObjBool;
typedef simgear::PropertyObject<std::string> SGPropObjString;
typedef simgear::PropertyObject<long> SGPropObjInt;

#endif
