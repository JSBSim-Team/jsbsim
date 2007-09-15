// Template Numerical Toolkit (TNT) for Linear Algebra
//
// BETA VERSION INCOMPLETE AND SUBJECT TO CHANGE
// Please see http://math.nist.gov/tnt for updates
//
// R. Pozo
// Mathematical and Computational Sciences Division
// National Institute of Standards and Technology


// Basic TNT  numerical vector (0-based [i] AND 1-based (i) indexing )
//

//Chris Siefert's modified namespace-free version - 6/8/99
//adding l2norm capabilities.
//made the dot product the overloaded *
//component-based multiply is now compmult.
//added Scalar * Vector, and Vector * Scalar
//added overloaded == and != operators

//COMPLETE OPERATOR LIST
// Vector<T>& newsize(Subscript N)
// Vector<T>& operator=(const Vector<T> &A)
// Vector<T>& operator=(const T& scalar)
// Subscript dim() const (also size())
// operator()
// operator[]
// ostream& operator<<(ostream &s, const Vector<T> &A)
// istream & operator>>(istream &s, Vector<T> &A)
// vector + vector
// vector - vector
// compmult(vector, vector) - componant-wise multiplication, used to be *

// cmsief
// friend bool operator==(const Vector<T>&A, const Vector<T>& B)
// friend bool isnear(const Vector<T>&A, const Vector<T>& B, const T tolerance)
// friend bool operator!=(const Vector<T>&A, const Vector<T>& B)
// double l2norm()
// double l2norm_sqr()
// scalar * vector, vector * scalar (also scalmult)
// vector * vector - dot product, uset to be dotprod


#ifndef VEC_H
#define VEC_H

// #define TNT_BOUNDS_CHECK 1 if you want bound check

#ifndef DOLD_ALLOC
#include <new>
#endif

//#include "subscrpt.h"
#if defined(sgi) && !defined(__GNUC__)
# include <math.h>
# include <stdlib.h>
# include <assert.h>
#else
# include <cmath> /*for l2norms*/
# include <cstdlib>
# include <cassert>
#endif
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
//#include <strstream.h> deprecated

#define D_PRECISION 16

using namespace std;
//namespace TNT
//{

typedef long Subscript;

template <class T>
class Vector 
{


  public:

    typedef Subscript   size_type;
    typedef         T   value_type;
    typedef         T   element_type;
    typedef         T*  pointer;
    typedef         T*  iterator;
    typedef         T&  reference;
    typedef const   T*  const_iterator;
    typedef const   T&  const_reference;

    Subscript lbound() const { return 1;}
 
  protected:
    T* v_;                  
    T* vm1_;        // pointer adjustment for optimzied 1-offset indexing
    Subscript n_;

    // internal helper function to create the array
    // of row pointers

    void initialize(Subscript N)
    {
        // adjust pointers so that they are 1-offset:
        // v_[] is the internal contiguous array, it is still 0-offset
        //
        assert(v_ == NULL);
	
#ifdef DOLD_ALLOC
        v_ = new T[N];
        assert(v_  != NULL);
#else
	try{
	  v_ = new T[N];
	} //try	
	//catch ( bad_alloc exception ) {      
	catch ( bad_alloc ) {      
	  cerr << "Memory allocation failed in file vec.h, method initialize()."
	       << "Exiting with value 1.\n";
	  exit(1);
	} //catch
#endif
        vm1_ = v_-1;
        n_ = N;
    }
   
    void copy(const T*  v)
    {
        Subscript N = n_;
        Subscript i;

#ifdef TNT_UNROLL_LOOPS
        Subscript Nmod4 = N & 3;
        Subscript N4 = N - Nmod4;

        for (i=0; i<N4; i+=4)
        {
            v_[i] = v[i];
            v_[i+1] = v[i+1];
            v_[i+2] = v[i+2];
            v_[i+3] = v[i+3];
        }

        for (i=N4; i< N; i++)
            v_[i] = v[i];
#else

        for (i=0; i< N; i++)
            v_[i] = v[i];
#endif      
    }

    void set(const T& val)
    {
        Subscript N = n_;
        Subscript i;

#ifdef TNT_UNROLL_LOOPS
        Subscript Nmod4 = N & 3;
        Subscript N4 = N - Nmod4;

        for (i=0; i<N4; i+=4)
        {
            v_[i] = val;
            v_[i+1] = val;
            v_[i+2] = val;
            v_[i+3] = val; 
        }

        for (i=N4; i< N; i++)
            v_[i] = val;
#else

        for (i=0; i< N; i++)
            v_[i] = val;
        
#endif      
    }
    


    void destroy()
    {     
        if (v_ == NULL) return ;

        delete [] (v_);
        v_ = NULL;
        vm1_ = NULL;
    }


  public:

    // access

    iterator begin() { return v_;}
    iterator end()   { return v_ + n_; }
    const iterator begin() const { return v_;}
    const iterator end() const  { return v_ + n_; }

    // destructor

    ~Vector() 
    {
        destroy();
    }

    // constructors

    Vector() : v_(0), vm1_(0), n_(0)  {};

    Vector(const Vector<T> &A) : v_(0), vm1_(0), n_(0)
    {
        initialize(A.n_);
        copy(A.v_);
    }

    Vector(Subscript N, const T& value = T(0)) :  v_(0), vm1_(0), n_(0)
    {
        initialize(N);
        set(value);
    }

    Vector(Subscript N, const T* v) :  v_(0), vm1_(0), n_(0)
    {
        initialize(N);
        copy(v);
    }

    Vector(Subscript N, char *s) :  v_(0), vm1_(0), n_(0)
    {
        initialize(N);
	istringstream ins(s);     

        Subscript i;

        for (i=0; i<N; i++)
                ins >> v_[i];
    }


    // methods
    // 
    Vector<T>& newsize(Subscript N)
    {
        if (n_ == N) return *this;

        destroy();
        initialize(N);

        return *this;
    }

  
    // assignments
    //
    Vector<T>& operator=(const Vector<T> &A)
    {
        if (v_ == A.v_)
            return *this;

        if (n_ == A.n_)         // no need to re-alloc
            copy(A.v_);

        else
        {
            destroy();
            initialize(A.n_);
            copy(A.v_);
        }

        return *this;
    }
        
    Vector<T>& operator=(const T& scalar)
    { 
        set(scalar);  
        return *this;
    }

    Subscript dim() const 
    {
        return  n_; 
    }

    Subscript size() const 
    {
        return  n_; 
    }

  /*Equivalence Operators -cmsief*/

  friend bool isnear(const Vector<T>&A, const Vector<T>& B, const T tolerance) {
    bool s=true;
    if(A.n_!=B.n_ || tolerance<0) return false;

    for(Subscript i=0;s&&i<A.n_;i++){
      if (fabs(A.v_[i]-B.v_[i]) > tolerance ) s=false;
    }
    return s;    
  }/*end isnear*/
    
  friend bool operator==(const Vector<T>&A, const Vector<T>& B) {
    bool s=true;
    if(A.n_!=B.n_) return false;

    for(Subscript i=0;s&&i<A.n_;i++){
      if (A.v_[i]!=B.v_[i]) s=false;
    }
    return s;    
  }
  
  friend bool operator!=(const Vector<T>&A, const Vector<T>& B) {
    return !(A==B);
  }
    
  /*end cmsief*/

  
    inline reference operator()(Subscript i)
    { 
#ifdef TNT_BOUNDS_CHECK
        assert(1<=i);
        assert(i <= n_) ;
#endif
        return vm1_[i]; 
    }

    inline const_reference operator() (Subscript i) const
    {
#ifdef TNT_BOUNDS_CHECK
        assert(1<=i);
        assert(i <= n_) ;
#endif
        return vm1_[i]; 
    }

    inline reference operator[](Subscript i)
    { 
#ifdef TNT_BOUNDS_CHECK
        assert(0<=i);
        assert(i < n_) ;
#endif
        return v_[i]; 
    }

    inline const_reference operator[](Subscript i) const
    {
#ifdef TNT_BOUNDS_CHECK
        assert(0<=i);
        assert(i < n_) ;
#endif
        return v_[i]; 
    }


  //  friend std::istream & operator>>(std::istream &s, Vector<T> &A);
#ifdef OLD_LIBC
  friend istream & operator>>(istream &s, Vector<T> &A);
#else
  //  template<class T>
  friend istream & operator>><>(istream &s, Vector<T> &A);
#endif
  
// *******************[ basic norm algorithms ]***********************cmsief

  double l2norm() {

    /*This algorithm is drawn from the f2c'd CLAPACK from netlib.
      translated by f2c (version 19940927).
      Modified on 14-October-1993 to inline the call to DLASSQ.   
      Sven Hammarling, Nag Ltd.   
      Modified on 25-May-1999 to act in a C++ manner and work with R. Pozo's Vector.
      Chris Siefert, College of William and Mary.
      This returns the l2norm of the vector.
    */
    
    double d__1, scale, absxi, ssq;
    
    if (n_ < 1) return (0.0);
    else if (n_ == 1) return(fabs((double)v_[0]));
    else {
      scale = 0.0;
      ssq = 1.0;
      
      for (Subscript ix = 0; ix < n_; ix++ ) {        
        if (v_[ix] != 0.0) {          
          absxi = (d__1 = (double) v_[ix], fabs(d__1));            
          if (scale < absxi) {
            /* Computing 2nd power */
            d__1 = scale / absxi;
            ssq = ssq * (d__1 * d__1) + 1.0;
            scale = absxi;
          }/*end if*/
          else {
            /* Computing 2nd power */
            d__1 = absxi / scale;
            ssq += d__1 * d__1;
          }/*end else*/
        }/*end if*/
        /* L10: */
      }/*end for*/
      return(scale * sqrt(ssq));
    }/*end else*/
  }/*end l2norm - cmsief*/


  double l2norm_sqr() {
    
    /*This algorithm is drawn from the f2c'd CLAPACK from netlib.
      translated by f2c (version 19940927).
      Modified on 14-October-1993 to inline the call to DLASSQ.   
      Sven Hammarling, Nag Ltd.   
      Modified on 25-May-1999 to act in a C++ manner and work with R. Pozo's Vector.
      Chris Siefert, College of William and Mary.
      This returns the square of the l2norm.
    */
    
    double d__1, scale, absxi, ssq;
    
    if (n_ < 1) return (0.0);
    else if (n_ == 1) return(fabs((double)v_[0]*v_[0]));
    else {
      scale = 0.0;
      ssq = 1.0;
      
      for (Subscript ix = 0; ix < n_; ix++ ) {        
        if (v_[ix] != 0.0) {          
          absxi = (d__1 = (double) v_[ix], fabs(d__1));            
          if (scale < absxi) {
            /* Computing 2nd power */
            d__1 = scale / absxi;
            ssq = ssq * (d__1 * d__1) + 1.0;
            scale = absxi;
          }/*end if*/
          else {
            /* Computing 2nd power */
            d__1 = absxi / scale;
            ssq += d__1 * d__1;
          }/*end else*/
        }/*end if*/
        /* L10: */
      }/*end for*/
      return(scale * scale * ssq);
    }/*end else*/
  }/*end l2norm_sqr - cmsief*/
  
};/*end class*/


/* ***************************  I/O  ********************************/
//std::ostream& operator<<(std::ostream &s, const Vector<T> &A)
template <class T>
ostream& operator<<(ostream &s, const Vector<T> &A)
{
    Subscript N=A.dim();

    s <<  N << endl;

    for (Subscript i=0; i<N; i++)
      s  <<setprecision(D_PRECISION) << A[i] << " " << endl;
    s << endl;

    return s;
}
//std::istream & operator>>(std::istream &s, Vector<T> &A)
template <class T>
istream & operator>>(istream &s, Vector<T> &A)
{

    Subscript N;

    s >> N;

    if ( !(N == A.n_) )
    {
        A.destroy();
        A.initialize(N);
    }


    for (Subscript i=0; i<N; i++)
            s >>  A[i];


    return s;
}


// *******************[ basic matrix algorithms ]***************************

/****cmsief****/
template <class T>
Vector<T> scalmult(const Vector<T> &A, const T &B)
{
    Subscript N = A.dim(); 
    Vector<T> tmp(N);
    Subscript i;
    for (i=0; i<N; i++)
            tmp[i] = A[i] *B;
    return tmp; 
} 


template <class T>
Vector<T> operator*(const Vector<T> &A, const T &B)
{
  return scalmult(A,B);
}

template <class T>
Vector<T> operator*(const T &B, const Vector<T> &A)
{
  return scalmult(A,B);
}

/****end cmsief*****/

template <class T>
Vector<T> operator+(const Vector<T> &A, 
    const Vector<T> &B)
{
    Subscript N = A.dim();

    assert(N==B.dim());

    Vector<T> tmp(N);
    Subscript i;

    for (i=0; i<N; i++)
            tmp[i] = A[i] + B[i];

    return tmp;
}

template <class T>
Vector<T> operator-(const Vector<T> &A, 
    const Vector<T> &B)
{
    Subscript N = A.dim();

    assert(N==B.dim());

    Vector<T> tmp(N);
    Subscript i;

    for (i=0; i<N; i++)
            tmp[i] = A[i] - B[i];

    return tmp;
}

//Vector<T> operator*(const Vector<T> &A, const Vector<T> &B)
template <class T>
Vector<T> compmult(const Vector<T> &A, const Vector<T> &B)
{
    Subscript N = A.dim();

    assert(N==B.dim());

    Vector<T> tmp(N);
    Subscript i;

    for (i=0; i<N; i++)
            tmp[i] = A[i] * B[i];

    return tmp;
}

//T dot_prod(const Vector<T> &A, const Vector<T> &B)
template <class T>
T operator* (const Vector<T> &A, const Vector<T> &B)
{
    Subscript N = A.dim();
    assert(N == B.dim());

    Subscript i;
    T sum = 0;

    for (i=0; i<N; i++)
        sum += A[i] * B[i];

    return sum;
}

//}   /* namespace TNT */

#endif
// VEC_H


