/** Dyn_alloc

This handles dynamic memory allocation.  
If your compiler does not support the new 
try/catch syntax, you should compile with a -DDOLD_ALLOC flag, 
or else just define DOLD_ALLOC
in your file before you include any of our files

If your compiler does support try/catch, you just need to 
include <new> and not worry about
anything.

Anne Shepherd, College of William and Mary
revised by Anthony Padula, 7/2000
*/


#ifdef DOLD_ALLOC
#ifndef DS_ALLOC_OLD
#define DS_ALLOC_OLD

/** Old Style allocators:  Compile with flag -DOLD_ALLOC

    Use this version ONLY if your compiler does not support the new 
    try/catch syntax. Note that this will probably not work with newer 
    compilers because the ANSI C++ standard does not require the new 
    operator to return a null pointer if it fails.

    Anne Shepherd, College of William and Mary.

*/

#include <iostream>
#include "vec.h"
#include "cppmat.h"
#include <cstdlib>
#include <string>
//#include <new>


template <class T>
void new_array(T *&myptr, int size) {
  myptr = new T[size]; 
  if (myptr == NULL) {
    cerr << "New Array memory allocation failed."
	 << "Exiting with value 1.\n";
    exit(1);
  } // if
  //bzero(myptr, sizeof(T));
} // new_array


template <class T>
void new_Vector(Vector<T> *& myptr, long dim) {
  myptr = new Vector<T>(dim, T(0.0));
  if (myptr == NULL) {
    cerr << "New Vector memory allocation failed."
	 << "Exiting with value 1.\n";
    exit(1);
  } // if
} //new_Vector

template <class T>
void new_Vector_Init(Vector<T> *& myptr, const Vector<T> & Holder) {
  myptr = new Vector<T>(Holder);
  if (myptr == NULL) {
    cerr << "New Vector memory allocation failed."
	 << "Exiting with value 1.\n";
    exit(1);
  } // if
} //new_Vector


template <class T>
void new_Matrix(Matrix<T> *& myptr, long dim1, long dim2) {
  myptr = new Matrix<T>(dim1, dim2, T(0.0));
  if (myptr == NULL) {
    cerr << "New Matrix memory allocation failed."
	 << "Exiting with value 1.\n";
    exit(1);
  } // if
} //new_Matrix

template <class T>
void new_Matrix_Init(Matrix<T> *& myptr, const Matrix<T> & Holder) {
  myptr = new Matrix<T>(Holder);
  if (myptr == NULL) {
    cerr << "New Matrix memory allocation failed."
	 << "Exiting with value 1.\n";
    exit(1);
  } // if
} //new_Matrix

#endif
#else
/*  Allocators for ANSI Standard-compliant compilers.   
 
    Contains templates for dynamic allocation of Vectors, Matrices, and arrays.
    For use with DirectSearch and its derived classes.  Note that the methods
    defined in this header file use the try/catch syntax of the new ANSI C++ standard. 
    Note that the array allocator method allocates an uninitialized array.
    new_Vector_Init and new_Matrix_Init allocate memory and initialize
    it to user-specified values.

    If you are using an old compiler that does not support this standard, change 
    #include "Dyn_alloc.h" to #include "Dyn_alloc_old.h" and compile with 
    Dyn_alloc_old.h.
    
    Anne Shepherd, College of William and Mary

*/

#ifndef DS_NEW_ALLOC
#define DS_NEW_ALLOC

#include <iostream>
#include "vec.h"
#include "cppmat.h"
#include <new>

/**@name Dyn_alloc.h

    Dyn_alloc.h 
 
    Contains templates for dynamic allocation of Vectors, Matrices, and arrays.
    For use with DirectSearch and its derived classes.  Note that the methods
    defined in this header file use the try/catch syntax of the new ANSI C++ standard. 
    If you are using an old compiler that does not support this syntax, change 
     \##include "Dyn_alloc.h"# to  \##include "Dyn_alloc_old.h"# and compile with 
    #Dyn_alloc_old.h#.
    
    Anne Shepherd, College of William and Mary

*/
//@{
/**Template new_array allocates a new array of type T, length size.  
   The user should send in myptr as just a null pointer.  
     @param myptr pointer to an object of type T
     @param size the desired length of the new array
     @return void
*/
   
template <class T>
void new_array(T *&myptr, long size) {
    try{
      myptr = new T[size]; 
    } //try
    //catch ( bad_alloc exception ) {      
    catch ( bad_alloc ) {      
   	cerr << "Memory allocation failed in new_Array."
	     << "Exiting with value 1.\n";
	exit(1);
    } //catch
    for(long i = 0; i < size; i++) {
      myptr[i] = T(0.0);
    } // for
} // new_array

/**Template new_Vector allocates a new Vector of type T, length dim, and
   initializes all entries to zero.
   The user should send in myptr as just a null pointer.  
     @param myptr pointer to an object of type T
     @param dim the desired length of the new Vector
     @return void
*/
template <class T>
void new_Vector(Vector<T> *& myptr, long dim) {
  try{
    myptr = new Vector<T>(dim, T(0.0));
  } //try
  //catch ( bad_alloc exception ) {
  catch ( bad_alloc ) {
    cerr << "Memory allocation failed in new_Vector."
	 << "Exiting with value 1.\n";
    exit(1);
  } //catch
} //new_Vector

/**Template new_Vector_Init allocates a new Vector of type T, length dim, and
   initializes all entries to the corresponding values in Holder.
   The user should send in myptr as just a null pointer.  
     @param myptr pointer to an object of type T
     @param Holder the Vector from which to initialize the newly allocated Vector
     @return void
*/
template <class T>
void new_Vector_Init(Vector<T> *& myptr, const Vector<T> & Holder) {
  try{
    myptr = new Vector<T>(Holder);
  } //try
  //catch ( bad_alloc exception ) {
  catch ( bad_alloc ) {
    cerr << "Memory allocation failed in new_Vector_Init."
	 << "Exiting with value 1.\n";
    exit(1);
  } //catch
} //new_Vector


/**Template new_Matrix allocates a new Matrix of type T, of dimensions
   dim1 by dim2, and initializes all entries to zero.
   The user should send in myptr as just a null pointer.  
     @param myptr pointer to an object of type T
     @param dim1 number of rows in the new Matrix
     @param dim2 number of columns in the new Matrix
     @return void
*/
template <class T>
void new_Matrix(Matrix<T> *& myptr, long dim1, long dim2) {
  try{
    myptr = new Matrix<T>(dim1, dim2, T(0.0));
  } //try
  //catch ( bad_alloc exception ) {      
  catch ( bad_alloc ) {      
    cerr << "Memory allocation failed in new_Matrix."
	 << "Exiting with value 1.\n";
    exit(1);
  } //catch
} //new_Matrix

/**Template new_Matrix_Init allocates a new Matrix of type T. The
   new Matrix will have the same dimensions and the same entries as Holder.
   The user should send in myptr as just a null pointer.  
     @param myptr pointer to an object of type T
     @param Holder the Vector from which to initialize the newly allocated Vector
     @return void
*/
template <class T>
void new_Matrix_Init(Matrix<T> *& myptr, const Matrix<T> & Holder) {
  try{
    myptr = new Matrix<T>(Holder);
  } //try
  //catch ( bad_alloc exception ) {      
  catch ( bad_alloc ) {      
    cerr << "Memory allocation failed in new_Matrix_Init."
	 << "Exiting with value 1.\n";
    exit(1);
  } //catch
} //new_Matrix
//@} // end Dyn_alloc.h
#endif
#endif
