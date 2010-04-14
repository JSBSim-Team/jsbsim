/* -*-c++-*-
 *
 * Copyright (C) 2005-2006 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef SGSharedPtr_HXX
#define SGSharedPtr_HXX

#include "SGReferenced.hxx"

/// This class is a pointer proxy doing reference counting on the object
/// it is pointing to.
/// The SGSharedPtr class handles reference counting and possible
/// destruction if no nore references are in use automatically.
/// Classes derived from @SGReferenced can be handled with SGSharedPtr.
/// Once you have a SGSharedPtr available you can use it just like
/// a usual pointer with the exception that you don't need to delete it.
/// Such a reference is initialized by zero if not initialized with a
/// class pointer.
/// One thing you need to avoid are cyclic loops with such pointers.
/// As long as such a cyclic loop exists the reference count never drops
/// to zero and consequently the objects will never be destroyed.
/// Always try to use directed graphs where the references away from the
/// top node are made with SGSharedPtr's and the back references are done with
/// ordinary pointers.
/// There is a very good description of OpenSceneGraphs ref_ptr which is
/// pretty much the same than this one at
/// http://dburns.dhs.org/OSG/Articles/RefPointers/RefPointers.html

template<typename T>
class SGSharedPtr {
public:
  SGSharedPtr(void) : _ptr(0)
  {}
  SGSharedPtr(T* ptr) : _ptr(ptr)
  { get(_ptr); }
  SGSharedPtr(const SGSharedPtr& p) : _ptr(p.ptr())
  { get(_ptr); }
  template<typename U>
  SGSharedPtr(const SGSharedPtr<U>& p) : _ptr(p.ptr())
  { get(_ptr); }
  ~SGSharedPtr(void)
  { put(); }
  
  SGSharedPtr& operator=(const SGSharedPtr& p)
  { assign(p.ptr()); return *this; }
  template<typename U>
  SGSharedPtr& operator=(const SGSharedPtr<U>& p)
  { assign(p.ptr()); return *this; }
  template<typename U>
  SGSharedPtr& operator=(U* p)
  { assign(p); return *this; }

  T* operator->(void) const
  { return _ptr; }
  T& operator*(void) const
  { return *_ptr; }
  operator T*(void) const
  { return _ptr; }
  T* ptr(void) const
  { return _ptr; }

  bool isShared(void) const
  { return SGReferenced::shared(_ptr); }
  unsigned getNumRefs(void) const
  { return SGReferenced::count(_ptr); }

  bool valid(void) const
  { return _ptr; }

private:
  void assign(T* p)
  { get(p); put(); _ptr = p; }

  void get(const T* p) const
  { SGReferenced::get(p); }
  void put(void)
  { if (!SGReferenced::put(_ptr)) { delete _ptr; _ptr = 0; } }
  
  // The reference itself.
  T* _ptr;
};

#endif
