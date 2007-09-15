// Template Numerical Toolkit (TNT) for Linear Algebra
//
// BETA VERSION INCOMPLETE AND SUBJECT TO CHANGE
// Please see http://math.nist.gov/tnt for updates
//
// R. Pozo
// Mathematical and Computational Sciences Division
// National Institute of Standards and Technology

// Modified 6/2003 to make it work with new compilers,
//   and renamed.
//      P.L. (Anne) Shepherd

// ***********************************************************
// ****IMPORTANT NOTE:
//     This file is a child of our old cmat.h file, ***BUT***
//     I have modified this code so the overloaded reference
//         operator () has C-type 0-based indexing.  This is 
//         a CHANGE from the original, which uses Fortran-type
//         1-based indexing.  Any program that calls the operator
//         in this version of cppmat.h expecting 1-based indexing,
//         or uses an old version of cppmat.h and expects 0-based
//         indexing, will get BAD RESULTS.  Please BE SURE
//         you know which version of cppmat.h you are using and 
//         which one you need to be using.
//           ----Anne Shepherd, 6/17/03   ********************
// ***********************************************************

// C compatible matrix: row-oriented, 0-based [i][j] and  (i,j) indexing
//
// Chris Siefert's modified namespace free version - 5/26/99
// function (row) has been added.  
//    It returns the vector that contains that row.
// function (col) has been added.  
//    It returns the vector that contains that column.
// Also, support for multiplication by a scalar has been added.
// Support for Vector * Matrix acting as if the vector was a row 
//    vector is added.
//
// Added line for namespace std 6/2003 Anne Shepherd

//Since Mr. Pozo never did it, here is the complete catalog of outside
//accessable Matrix operations:
//
// Matrix<T>& newsize(Subscript M, Subscript N) - resize matrix
// operator T**(){ return  row_;} - ???
// Subscript size() const { return mn_; }
// operator = Matrix
// operator = scalar - assigns all elements to the scalar.
// Subscript dim(Subscript d)
// num_rows() 
// num_cols() 
// operator[](Subscript i)
// operator()(Subscript i)
// operator()(Subscript i, Subscript j)
// ostream << Matrix
// istream >> Matrix
// Region operator()(const Index1D &I, const Index1D &J) - ???
// Matrix + Matrix
// Matrix - Matrix
// mult_element(A,B) - element by element multiplication
// transpose(A) - matrix transposition
// Matrix * Matrix (also matmult)
// matmult(C, B, A) - result stored in C
// Matrix * Vector (also matmult)

// Chris Siefert Additions:
// row(i) - returns a Vector containing the ith row.
// col(i) - returns a Vector containing the ith column.
// Matrix * Scalar, Scalar * Matrix (also scalmult, 1st form only).
// Vector * Matrix (also matmult).  Pretends vector is a row vector.
//
// Modified 7/00 by Anne Shepherd to allow for different implementations
// of the operators new and new[].  If your compiler does not support the
// ANSI Standard try-catch syntax, compile with -DDOLD_ALLOC flag or just 
// #define DOLD_ALLOC in your file before including our files.

#ifndef CMAT_cms_pls_H
#define CMAT_cms_pls_H

#ifndef DOLD_ALLOC
#include <new>
#endif

#include <math/direct_search/vec.h>
#if defined(sgi) && !defined(__GNUC__)
# include <stdlib.h>
# include <assert.h>
#else
# include <cstdlib>
# include <cassert>
#endif

#include <iostream>
#ifdef TNT_USE_REGIONS
#include "region2d.h"
#endif

#include <iomanip>
#define D_PRECISION 16


//namespace TNT
//{
typedef long Subscript;

template <class T>
class Matrix 
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
    Subscript m_;
    Subscript n_;
    Subscript mn_;      // total size
    T* v_;                  
    T** row_;           
    T* vm1_ ;       // these point to the same data, but are 1-based 
    T** rowm1_;

    // internal helper function to create the array
    // of row pointers

    void initialize(Subscript M, Subscript N)
    {
        mn_ = M*N;
        m_ = M;
        n_ = N;

#ifdef DOLD_ALLOC
        v_ = new T[mn_]; 
        row_ = new T*[M];
        rowm1_ = new T*[M];

        assert(v_  != NULL);
        assert(row_  != NULL);
        assert(rowm1_ != NULL);
#else
	try{
	  v_ = new T[mn_]; 
	} //try
	//catch ( bad_alloc exception ) {      
	catch ( bad_alloc ) {      
	  cerr << "Memory allocation failed in file cppmat.h, method initialize()."
	       << "Exiting with value 1.\n";
	  exit(1);
	} //catch

	try{
	  row_ = new T*[M];
	} //try
	//catch (bad_alloc exception) {      
	catch (bad_alloc) {      
	  cerr << "Memory allocation failed in file cppmat.h, method initialize()."
	       << "Exiting with value 1.\n";
	  exit(1);
	} //catch

	try{
	  rowm1_ = new T*[M];
	} //try
	//catch ( bad_alloc exception ) {      
	catch ( bad_alloc ) {      
	  cerr << "Memory allocation failed in file cppmat.h, method initialize()."
	       << "Exiting with value 1.\n";
	  exit(1);
	} //catch
#endif

        T* p = v_;              
        vm1_ = v_ - 1;
        for (Subscript i=0; i<M; i++)
        {
            row_[i] = p;
            rowm1_[i] = p-1;
            p += N ;
            
        }

        rowm1_ -- ;     // compensate for 1-based offset
    }
   
    void copy(const T*  v)
    {
        Subscript N = m_ * n_;
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
        Subscript N = m_ * n_;
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
        /* do nothing, if no memory has been previously allocated */
        if (v_ == NULL) return ;

        /* if we are here, then matrix was previously allocated */
        if (v_ != NULL) delete [] (v_);     
        if (row_ != NULL) delete [] (row_);

        /* return rowm1_ back to original value */
        rowm1_ ++;
        if (rowm1_ != NULL ) delete [] (rowm1_);
    }


  public:

    operator T**(){ return  row_; }
    operator T**() const { return row_; }


    Subscript size() const { return mn_; }

    // constructors

    Matrix() : m_(0), n_(0), mn_(0), v_(0), row_(0), vm1_(0), rowm1_(0) {};

    Matrix(const Matrix<T> &A)
    {
        initialize(A.m_, A.n_);
        copy(A.v_);
    }

    Matrix(Subscript M, Subscript N, const T& value = T(0))
    {
        initialize(M,N);
        set(value);
    }

    Matrix(Subscript M, Subscript N, const T* v)
    {
        initialize(M,N);
        copy(v);
    }

    Matrix(Subscript M, Subscript N, char *s)
    {
        initialize(M,N);   

        istringstream ins(s);        
        Subscript i, j;

        for (i=0; i<M; i++)
            for (j=0; j<N; j++)
                ins >> row_[i][j];
    }

    // destructor
    //
    ~Matrix()
    {
        destroy();
    }


    // reallocating
    //
    Matrix<T>& newsize(Subscript M, Subscript N)
    {
        if (num_rows() == M && num_cols() == N)
            return *this;

        destroy();
        initialize(M,N);
        
        return *this;
    }




    // assignments
    //
    Matrix<T>& operator=(const Matrix<T> &A)
    {
        if (v_ == A.v_)
            return *this;

        if (m_ == A.m_  && n_ == A.n_)      // no need to re-alloc
            copy(A.v_);

        else
        {
            destroy();
            initialize(A.m_, A.n_);
            copy(A.v_);
        }

        return *this;
    }
        
    Matrix<T>& operator=(const T& scalar)
    { 
        set(scalar); 
        return *this;
    }


    Subscript dim(Subscript d) const 
    {
#ifdef TNT_BOUNDS_CHECK
       assert( d >= 1);
        assert( d <= 2);
#endif
        return (d==1) ? m_ : ((d==2) ? n_ : 0); 
    }

    Subscript num_rows() const { return m_; }
    Subscript num_cols() const { return n_; }


  inline T* operator[](Subscript i)
    {
#ifdef TNT_BOUNDS_CHECK
        assert(0<=i);
        assert(i < m_) ;
#endif
        return row_[i];
    }

/*START - cmsief************************/
  
  /*This is a Chris Siefert original that attempts to return a vector*/
  inline Vector<T> row (Subscript i) const
    {
#ifdef TNT_BOUNDS_CHECK
     assert(0<=i);
     assert(i < m_) ;
#endif
     Vector<T>temp(n_,row_[i]);
     return (temp);     
    }

  inline Vector<T> col (Subscript i) const
    {
#ifdef TNT_BOUNDS_CHECK
     assert(0<=i);
     assert(i < n_) ;
#endif  
     Vector<T>temp(m_);
     for(long f=0;f<m_;f++) temp[f]=row_[f][i];
     return (temp);
    }
/*END - cmsief************************/
  
  inline const T* operator[](Subscript i) const
    {
#ifdef TNT_BOUNDS_CHECK
      assert(0<=i);
      assert(i < m_) ;
#endif
      return row_[i];
    }

    /*  Changed all this from being 1-based to good ol' C-type
     *  0-based indexing.
     *     pls, 5/15/03
     */

  // this is for a sanity check later---using an old version of this
  // file will result in off-by-1 errors and give incorrect results.
#define _changed_f_indexing_to_c_pls

    inline reference operator()(Subscript i)
    { 
#ifdef TNT_BOUNDS_CHECK
        assert(1<=i);
        assert(i <= mn_) ;
#endif
        //return vm1_[i];
        return v_[i]; 
    }

    inline const_reference operator()(Subscript i) const
    { 
#ifdef TNT_BOUNDS_CHECK
        assert(1<=i);
        assert(i <= mn_) ;
#endif
        //return vm1_[i]; 
        return v_[i]; 
    }


    inline reference operator()(Subscript i, Subscript j)
    { 
#ifdef TNT_BOUNDS_CHECK
        assert(1<=i);
        assert(i <= m_) ;
        assert(1<=j);
        assert(j <= n_);
#endif
        //return  rowm1_[i][j]; 
        return  row_[i][j]; 
    }


    
    inline const_reference operator() (Subscript i, Subscript j) const
    {
#ifdef TNT_BOUNDS_CHECK
        assert(1<=i);
        assert(i <= m_) ;
        assert(1<=j);
        assert(j <= n_);
#endif
        //return  rowm1_[i][j]; 
        return  row_[i][j]; 
    }

    /*  end P.L. Shepherd hacks to C-ize the () operators */

#ifdef OLD_LIBC
  friend istream & operator>>(istream &s, Matrix<T> &A);
#else
  //  template<class T>
  friend istream & operator>><>(istream &s, Matrix<T> &A);
#endif  
  //        friend std::istream & operator>>(std::istream &s, Matrix<T> &A);

#ifdef TNT_USE_REGIONS

    typedef Region2D<Matrix<T> > Region;
    

    Region operator()(const Index1D &I, const Index1D &J)
    {
        return Region(*this, I,J);
    }


    typedef const_Region2D< Matrix<T> > const_Region;
    const_Region operator()(const Index1D &I, const Index1D &J) const
    {
        return const_Region(*this, I,J);
    }

#endif


};


/* ***************************  I/O  ********************************/
//std::ostream& operator<<(std::ostream &s, const Matrix<T> &A)
template <class T>
ostream& operator<<(ostream &s, const Matrix<T> &A)
{
    Subscript M=A.num_rows();
    Subscript N=A.num_cols();

    s << M << " " << N << "\n";

    for (Subscript i=0; i<M; i++)
    {
        for (Subscript j=0; j<N; j++)
        {
          s <<setprecision(D_PRECISION)<< A[i][j] << " ";
        }
        s << "\n";
    }


    return s;
}
//std::istream& operator>>(std::istream &s, Matrix<T> &A)
template <class T>
istream& operator>>(istream &s, Matrix<T> &A)
{

    Subscript M, N;

    s >> M >> N;

    if ( !(M == A.m_ && N == A.n_) )
    {
        A.destroy();
        A.initialize(M,N);
    }


    for (Subscript i=0; i<M; i++)
        for (Subscript j=0; j<N; j++)
        {
            s >>  A[i][j];
        }


    return s;
}

// *******************[ basic matrix algorithms ]***************************


template <class T>
Matrix<T> operator+(const Matrix<T> &A, 
    const Matrix<T> &B)
{
    Subscript M = A.num_rows();
    Subscript N = A.num_cols();

    assert(M==B.num_rows());
    assert(N==B.num_cols());

    Matrix<T> tmp(M,N);
    Subscript i,j;

    for (i=0; i<M; i++)
        for (j=0; j<N; j++)
            tmp[i][j] = A[i][j] + B[i][j];

    return tmp;
}

template <class T>
Matrix<T> operator-(const Matrix<T> &A, 
    const Matrix<T> &B)
{
    Subscript M = A.num_rows();
    Subscript N = A.num_cols();

    assert(M==B.num_rows());
    assert(N==B.num_cols());

    Matrix<T> tmp(M,N);
    Subscript i,j;

    for (i=0; i<M; i++)
        for (j=0; j<N; j++)
            tmp[i][j] = A[i][j] - B[i][j];

    return tmp;
}

template <class T>
Matrix<T> mult_element(const Matrix<T> &A, 
    const Matrix<T> &B)
{
    Subscript M = A.num_rows();
    Subscript N = A.num_cols();

    assert(M==B.num_rows());
    assert(N==B.num_cols());

    Matrix<T> tmp(M,N);
    Subscript i,j;

    for (i=0; i<M; i++)
        for (j=0; j<N; j++)
            tmp[i][j] = A[i][j] * B[i][j];

    return tmp;
}


template <class T>
Matrix<T> transpose(const Matrix<T> &A)
{
    Subscript M = A.num_rows();
    Subscript N = A.num_cols();

    Matrix<T> S(N,M);
    Subscript i, j;

    for (i=0; i<M; i++)
        for (j=0; j<N; j++)
            S[j][i] = A[i][j];

    return S;
}


    
template <class T>
inline Matrix<T> matmult(const Matrix<T>  &A, 
    const Matrix<T> &B)
{

#ifdef TNT_BOUNDS_CHECK
    assert(A.num_cols() == B.num_rows());
#endif

    Subscript M = A.num_rows();
    Subscript N = A.num_cols();
    Subscript K = B.num_cols();

    Matrix<T> tmp(M,K);
    T sum;

    for (Subscript i=0; i<M; i++)
    for (Subscript k=0; k<K; k++)
    {
        sum = 0;
        for (Subscript j=0; j<N; j++)
            sum = sum +  A[i][j] * B[j][k];

        tmp[i][k] = sum; 
    }

    return tmp;
}

template <class T>
inline Matrix<T> operator*(const Matrix<T>  &A, 
    const Matrix<T> &B)
{
  return matmult(A,B);
}

/*More Chris Siefert additions*/

template <class T>
inline Matrix<T> scalmult(const Matrix<T>  &A, 
    const T &x)
{
  Matrix<T> tmp=A;
  Subscript M = A.num_rows();
  Subscript N = A.num_cols();

  for(Subscript i=0;i<M;i++)
    for(Subscript j=0;j<N;j++)
      tmp[i][j]*=x;
  return tmp;
}

template <class T>
inline Matrix<T> operator*(const Matrix<T>  &A, 
    const T &x)
{
  return scalmult(A,x);
}

template <class T>
inline Matrix<T> operator*(const T &x, 
    const Matrix<T> &A)
{
  return scalmult(A,x);
}

template <class T>
Vector<T> matmult(const Vector<T> &x, const Matrix<T> &A) {
  /*pretends that x is a row-vector*/
#ifdef TNT_BOUNDS_CHECK
    assert(A.num_rows() == x.dim());
#endif

    Subscript M = A.num_rows();
    Subscript N = A.num_cols();

    Vector<T> tmp(N);
    T sum;

    for (Subscript i=0; i<N; i++)
    {
        sum = 0;
        //        Vector<T> coli=A.col(i);
        for (Subscript j=0; j<M; j++)
            sum = sum +  A[j][i] * x[j];

        tmp[i] = sum; 
    }

    return tmp;

}/*end matmult*/

template <class T>
inline Vector<T> operator*(const Vector<T> &x, 
                           const Matrix<T> &A)
{
  return matmult(x,A);
}


/*end Chris Siefert additions*/

template <class T>
inline int matmult(Matrix<T>& C, const Matrix<T>  &A, 
    const Matrix<T> &B)
{

    assert(A.num_cols() == B.num_rows());

    Subscript M = A.num_rows();
    Subscript N = A.num_cols();
    Subscript K = B.num_cols();

    C.newsize(M,K);

    T sum;

    const T* row_i;
    const T* col_k;

    for (Subscript i=0; i<M; i++)
    for (Subscript k=0; k<K; k++)
    {
        row_i  = &(A[i][0]);
        col_k  = &(B[0][k]);
        sum = 0;
        for (Subscript j=0; j<N; j++)
        {
            sum  += *row_i * *col_k;
            row_i++;
            col_k += K;
        }
        C[i][k] = sum; 
    }

    return 0;
}


template <class T>
Vector<T> matmult(const Matrix<T>  &A, const Vector<T> &x)
{

#ifdef TNT_BOUNDS_CHECK
    assert(A.num_cols() == x.dim());
#endif

    Subscript M = A.num_rows();
    Subscript N = A.num_cols();

    Vector<T> tmp(M);
    T sum;

    for (Subscript i=0; i<M; i++)
    {
        sum = 0;
        const T* rowi = A[i];
        for (Subscript j=0; j<N; j++)
            sum = sum +  rowi[j] * x[j];

        tmp[i] = sum; 
    }

    return tmp;
}

template <class T>
inline Vector<T> operator*(const Matrix<T>  &A, const Vector<T> &x)
{
    return matmult(A,x);
}

//} // namespace TNT

#endif
// CMAT_H
