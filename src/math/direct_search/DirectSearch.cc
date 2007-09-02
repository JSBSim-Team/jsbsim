/** 
    DirectSearch.cc

    Implementation File for the abstract base class DirectSearch.

    Elizabeth Dolan, Anne Shepherd,
      and some help from Chris Seifert and Erin Parker
    College of William and Mary

*/ 


#include <math/direct_search/DirectSearch.h>

//constructors & destructor

DirectSearch::DirectSearch(long dim, Vector<double> &startPoint)
  {
#ifndef _changed_f_indexing_to_c_pls
    cerr<<"\nERROR:  using outdated version of cppmat.h---this will";
    cerr<<"\ngive incorrect results.  Use the updated version, which";
    cerr<<"\nchanges fortran-style indexing to C-style in all cases.\n\n";
    exit(1); 
#endif
    if (dim <= 0) {
      cerr << "\nIn default constructor: ";
      cerr << "\nDimension cannot be zero or less. ";
      cerr << " Cannot continue with constructor. ";
      cerr << "\nExiting with value 1.\n";
      exit(1);
    } //if
    dimension = dim;
    functionCalls = 0;
    design = NULL;
    minPoint = NULL;
    new_Vector_Init(minPoint, startPoint);
    minValue = HUGE_VAL;
    maxCalls = NO_MAX;
    stoppingStepLength = 10e-8;
    some_object = NULL;
    fcn_name = fcn;
    exact_count = false;
    IDnumber = 1000;

#if defined(AGO_DIRECTSEARCH)
    ofile = 0L;
#endif

  }//constructor initializes private data members to user defined starting values

  DirectSearch::DirectSearch(const DirectSearch & Original)
  {
    dimension = Original.GetDimension();
    design = NULL;
    InitializeDesign(Original.design);
    functionCalls = Original.functionCalls;
    minPoint = NULL;
    new_Vector_Init(minPoint,*(Original.minPoint));
    minValue = Original.minValue; 
    maxCalls = Original.maxCalls; 
    stoppingStepLength = Original.stoppingStepLength;  
    some_object = Original.some_object;
    fcn_name = Original.fcn_name;
    exact_count = Original.exact_count; 
    IDnumber = Original.IDnumber;

#if defined(AGO_DIRECTSEARCH)
    ofile = Original.ofile;
#endif

#ifndef _changed_f_indexing_to_c_pls
    cerr<<"\nERROR:  using outdated version of cppmat.h---this will";
    cerr<<"\ngive incorrect results.  Use the updated version, which";
    cerr<<"\nchanges fortran-style indexing to C-style in all cases.\n\n";
    exit(1); 
#endif
  }//deep copy constructor



DirectSearch::DirectSearch(long dim,
                           Vector<double> &startPoint,
                           double stopStep, 
                           void (*objective)(long vars, Vector<double> &x, 
                                             double & func, bool& flag, void* an_obj),
                           void * input_obj
                          )
{
#ifndef _changed_f_indexing_to_c_pls
    cerr<<"\nERROR:  using outdated version of cppmat.h---this will";
    cerr<<"\ngive incorrect results.  Use the updated version, which";
    cerr<<"\nchanges fortran-style indexing to C-style in all cases.\n\n";
    exit(1); 
#endif
    if (dim <= 0) {
      cerr << "\nIn special constructor: ";
      cerr << "\nDimension cannot be zero or less. ";
      cerr << " Cannot continue with constructor. ";
      cerr << "\nExiting with value 1.\n";
      exit(1);
    } //if
    dimension = dim;
    minPoint = NULL;
    functionCalls = 0;
    design = NULL;
    new_Vector_Init(minPoint, startPoint);
    //minPoint = new Vector<double>(startPoint);
    minValue = HUGE_VAL;
    maxCalls = NO_MAX;
    stoppingStepLength = stopStep;
    fcn_name = objective;
    some_object = input_obj;
    exact_count = false;
    IDnumber = 1000;

#if defined(AGO_DIRECTSEARCH)
    ofile = 0L;
#endif

}


DirectSearch::~DirectSearch()
  {
    if(design != NULL) {
      delete design;
      design = NULL;
    }
    if(minPoint != NULL) {
      delete minPoint;
      minPoint = NULL;
    }
    //note: matrix and vector classes have their own destructors
  }//destructor

  void DirectSearch::CopySearch(const DirectSearch & Original)
  {
    if(this != &Original) { 
      if(design != NULL) {
    delete design;
    design = NULL;
      } //if
      if(minPoint != NULL) {
    delete minPoint;
    minPoint = NULL;
      } // if
      dimension = Original.GetDimension();
      //CleanSlate(dimension);
      InitializeDesign(Original.design);
      functionCalls = Original.functionCalls;
      //     minPoint = new Vector<double>(*(Original.minPoint));
      new_Vector_Init(minPoint,*(Original.minPoint) );
      minValue = Original.minValue; 
      maxCalls = Original.maxCalls; 
      stoppingStepLength = Original.stoppingStepLength; 
      some_object = Original.some_object;
      fcn_name = Original.fcn_name;
      exact_count = Original.exact_count;
      IDnumber = Original.IDnumber;

//#endif

    } // outer if
  }//deep copy  


  DirectSearch& DirectSearch::operator=(const DirectSearch& A)
  {
      CopySearch(A);
      return(*this);
  }

  void DirectSearch::CleanSlate(long dim, Vector<double> &startPoint)
  //reinitialize all values
  {
    if (dim <= 0) {
      cerr << "\nDimension cannot be zero or less. ";
      cerr << " Cannot continue with constructor. ";
      cerr << "\nExiting with value 1.\n";
      exit(1);
    } //if
    dimension = dim;
    functionCalls = 0;
    if(design != NULL) {
      delete design;
      design = NULL;
    }
    if(minPoint != NULL) {
      delete minPoint;
      minPoint = NULL;
    }
    // minPoint = new Vector<double>(startPoint); 
    new_Vector_Init(minPoint, startPoint );
    if(minPoint == NULL) {
      cerr << "\nProblem with startpoint in Clean Slate--uninitialized.  Exiting.\n";
      exit(1);
    }
    stoppingStepLength = 10e-8;
    some_object = NULL;
    fcn_name = fcn;
    exact_count = false;

//#endif

  }//reinitialize all search values to appropriate user-defined start values

void DirectSearch::CleanSlate(long dim, 
                  Vector<double> &startPoint,
                  double stopStep,
                  void (*objective)(long vars, Vector<double> &x, 
                        double & func, bool& flag, void* an_obj),
                  void * input_obj)
//reinitialize all values--overloaded for use with MAPS
{
  CleanSlate(dim, startPoint);  
  stoppingStepLength = stopStep;
  fcn_name = objective;
  some_object = input_obj;   
} //special constructor for MAPS
  
  ///indirection of function call for purposes of keeping an accurate 
  ///tally of the number of function calls
  void DirectSearch::fcnCall(long n, Vector<double> &x, 
                 double & f, bool& flag, void * nothing)
  {
    // This is to make sure the call does not change the value of
    //  x --an unintended "feature" in our objective code is that
    //  it allows that change to be made.
    Vector<double> myx = x;
    fcn_name(n, myx, f, flag, nothing);
    functionCalls++;
  } //fcnCall

//We may want to change this so the user has to do the new
  void DirectSearch::CopyDesign(Matrix<double> * &designPtr) const
  //user should pass just a pointer, without preallocated memory
  {
    if(design != NULL)
      new_Matrix_Init(designPtr, (*design)); 

  }//copyDesign

  void DirectSearch::InitializeDesign(const Matrix<double> *designPtr)
  {
    if (design != NULL && design != designPtr) {
        delete design;
        design = NULL;
    } //if
    if(designPtr != NULL)
      new_Matrix_Init(design, (*designPtr));
  }//InitializeDesign


  bool DirectSearch::Stop()  
  {
    return (maxCalls != NO_MAX && functionCalls >= maxCalls);
  } //Stop


  long  DirectSearch::GetFunctionCalls() const { 
    return functionCalls; 
  } // GetFunctionCalls

  void DirectSearch:: SetFunctionCalls(long newCalls) {
    functionCalls = newCalls;
  }

  int DirectSearch::GetID() const
  {
    return IDnumber;
  } //GetID

  ///Retrieves the minimum point and assigns it to minPt
  ///The vector minimum should be of the correct dimension

  void  DirectSearch::GetMinPoint(Vector<double> & minimum) const
  {
    minimum = (*minPoint);
  } //GetMinPoint

  ///resets minPoint
  void DirectSearch::SetMinPoint(Vector<double> & minimum) const 
  {
    (*minPoint) = minimum;
  } //SetMinPoint

  ///best objective function value found thus far

  void  DirectSearch::GetMinVal(double & value) const
  {
    value = minValue; 
  } // GetMinVal

  void  DirectSearch::SetMinVal(double & value)
  {
    minValue = value; 
  } // SetMinVal

//+++make sure this works--I changed it+++ --pls
  Matrix<double> * DirectSearch::GetDesign()
  {
    Matrix<double> *temp = NULL;
    new_Matrix_Init(temp, (*design));
    return temp;
  } //GetDesign

  ///returns the number of dimensions
  long  DirectSearch::GetDimension() const
  {
    return dimension;
  } //GetDimension

  /// Accessor for maxCalls
  long  DirectSearch::GetMaxCalls() const
  {
    return maxCalls;
  } //GetMaxCalls;


  void DirectSearch::SetMaxCalls(long calls)
  {
      if (calls < 0) {
          maxCalls = NO_MAX;
      }
      else {
          maxCalls = calls;
      } //end if-else
  } //SetMaxCalls


  void DirectSearch::SetMaxCallsExact(long calls)
  {
    SetMaxCalls(calls);
    exact_count = true;
  }

  double DirectSearch::GetStoppingStepLength() const
  {
    return stoppingStepLength;
  } //getStoppingStepLength


  void DirectSearch::SetStoppingStepLength(double len)
  {
    if (len > 0)
      stoppingStepLength = len;
    else {
      cout << "\nIllegal input--StoppingStepLength must be greater than zero.\n";
      cout << "Using defualt value instead.\n";
    } //else
  } //getStoppingStepLength

  void DirectSearch::SetExact() 
  {
    exact_count = true;
  }

  void DirectSearch::SetInexact() 
  {
    exact_count = false;
  }

  bool DirectSearch::IsExact()
  {
    return exact_count;
  }


 bool DirectSearch::BreakOnExact()
  {
      return(exact_count == true
             && maxCalls != NO_MAX
             && functionCalls >= maxCalls);
  }
