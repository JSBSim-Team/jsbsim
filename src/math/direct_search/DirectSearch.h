#ifndef _DirectSearch_
#define _DirectSearch_

#include <iostream>
#include <fstream>
//#include <istream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <math/direct_search/objective.h>  // user's file
#include <math/direct_search/vec.h>        // vector and matrix classes
#include <math/direct_search/cppmat.h>
#include <math/direct_search/Dyn_alloc.h>

/**    DirectSearch is the abstract  base class for the PatternSearch and 
       SimplexSearch classes.  It contains:
       \begin{itemize}
       \item[]--constructors,
       \item[]--a virtual destructor, 
       \item[]--data members that will be used throughout all the classes, 
       \item[] --accessors for these data members,
       \item[] --virtual void placeholders for the algorithmic member functions to
          be implemented in the concrete classes, and
       \item[] --a few utility functions.
       \end{itemize}
         @author Elizabeth Dolan, Adam Gurson, Anne Shepherd, 
         College of William and Mary  
*/

using namespace std;
class DirectSearch
{
 public:
  /** If NO_MAX is used to initialize maxCalls (as it is by default), the search
      will terminate based only on refinement of the design. */
  static const long NO_MAX = -1;

 /**@name Constructors and destructor */
  //@{
 //constructors & destructor

  /**Constructor.  This constructor has two parameters.  Other data members will be set
     to default values, as follows.  This is of course in addition to assignments and 
     initializations made in derived classes. Note that fcn_name is set to fcn, the function
     found in objective.cc and objective.h.  The user is responsible for putting his or her
     objective function into objective.cc in a form that will work with this constructor.
    \begin{itemize}
    \item[] functionCalls = 0
    \item[] design = NULL
    \item[] minValue = HUGE_VAL
    \item[] maxCalls = NO_MAX
    \item[] stoppingStepLength = 10e-8
    \item[] some_object = NULL
    \item[] fcn_name = fcn (found in objective.cc)
    \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search
  */ 
  DirectSearch(long dim, Vector<double> &startPoint);


    /** Copy constructor
    @param Original a reference to the object from which to initialize */
  DirectSearch(const DirectSearch & Original);

  /**special constructor using void function and object pointers.  The objective function 
     can be sent in as the fourth parameter here; the last parameter is used for any other
     information that may be necessary; it is used in MAPS, for example, to send in an
     object from an outside class. Usually, though, the final parameter will simply be 
     set to NULL.
     
  This constructor takes five input parameters.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will normally be set to NULL.
  */ 
DirectSearch(long dim, 
             Vector<double> &startPoint,
             double stopStep,
             void (*objective)(long vars, Vector<double> &x, 
                               double &func, bool &flag, void* an_obj),
             void * input_obj);

  /** Destructor */
  virtual ~DirectSearch();
  
  /** Overloaded assignment operator.
      note that because DirectSearch is an abstract base class,
      the object actually returned will be of a type corresponding
      to one of the  concrete classes.
      @param A reference to a const DirectSearch object
      @return  DirectSearch&
   */
  DirectSearch& operator=(const DirectSearch& A);

  //@}  //constructors and destructor
 /**@name Other initialization methods */
  //@{

  /** Reads the design in from a file supplied by the user.
      User may also pass cin as the input stream as desired. 
      This method is unimplemented in this abstract base class; it
      will be implemented at a lower level in the hierarchy. */
  virtual void ReadInFile(istream & fp) = 0;


  /**  Reinitializes values in order to reuse the same search object.
      This version of {\bf CleanSlate} takes two parameters. Other data members
      are set by default as in the constructor {\bf DirectSearch(long, Vector<double>&)}.
      @param dim the dimension of the problem
      @param startPoint the new starting point for the search
      @return void
      @see CleanSlate(long dim, Vector<double> &startpoint, double stopStep,
              void (*objective)(long vars, Vector<double> &x, 
                        double & func, bool& flag, void* an_obj),
              void * input_obj)
  */
  virtual void CleanSlate(long dim, Vector<double> & startPoint);


  /**overloaded version of CleanSlate using void function and object pointers.  This
  version of CleanSlate takes six parameters.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will normally be set to NULL.
  @return void
  @see  CleanSlate(long dim, Vector<double> &startpoint) */
  virtual void CleanSlate(long dim, 
              Vector<double> &startPoint,
              double stopStep,
              void (*objective)(long vars, Vector<double> &x, 
                        double &func, bool &flag, void* an_obj),
              void * input_obj);

 
  /** Deletes any existing pattern and replaces it with the one pointed to by designPtr
     @param patternSize the number of "columns", i.e. trial points, in design matrix
     @param pat a pointer to a design matrix 
     @return void */  
  void InitializeDesign(const Matrix<double> *designPtr);

  /** Prints out search information
    @return void */
  virtual void PrintDesign() const = 0;


  //@}  //other init routines

 //algorithmic routines

  /** @name Search method */
  //@{
  /**{\bf BeginSearch()} is an unimplemented virtual void method in the abstract base 
     classes. It is to be implemented in the concrete 
     classes.  There, BeginSearch() will call the methods that implement the actual 
     search algorithms.  */
  virtual void BeginSearch() = 0;
  //@} //Search method


 /**@name Accessors and Mutators */
  //@{

  /** Returns number of objective function evaluations so far
      @return long */
  long GetFunctionCalls() const;

  /** Sets number of function calls already used--useful for times
      when you are using multiple searches for the same optimization;
      for example, if you wanted to do 100 function calls worth of
      SMDSearch followed by 100 calls (max) of HJSearch.
      @param newCalls the new value to which you wish to set functioCalls.
      @return void
  */
  void SetFunctionCalls(long newCalls);

  /** Returns the ID Number of the search.
      @return int
  */
  int GetID() const;

  
#if defined(AGO_DIRECTSEARCH)

  /**Sets the pointer to the output file stream for logging purposes
     @param outputf a pointer to ofstream
      @return void  */
  void SetOutputFile(ofstream * outputf) { ofile = outputf; }

  /**Retrieves the pointer to the object output file stream for logging purposes
     @param outputf address of ofstream
      @return ofstream*  */
  ofstream* GetOutputFile() const { return ofile; }

#endif

  /**Retrieves the minimum point and assigns it to minimum
  The vector minimum should be of the correct dimension.
    @param minimum a reference to a vector of the correct dimension, which will be assigned the value of minPoint.
    @return void */
  void GetMinPoint(Vector<double> & minimum) const;


  /**Resets minPoint to the point represented by the input parameter minimum
     @param minimum the new minimum point, whose value will be assigned to minPoint
      @return void  */
  void SetMinPoint(Vector<double> & minimum) const;

  /** Retrieves the best objective function value found thus far. The input parameter 
      will be set to the value of minValue.
        @param value a reference to a double, which will be given the value of minValue
        @return void */
  void GetMinVal(double & value) const; 

  /**Resets the minimum value to the value of the input parameter.
     @param value a reference to a double, to whose vale minValue will be set
     @return void */
  void SetMinVal(double & value); 

  
  /** Returns the dimension of the problem
      @return long */
  long GetDimension() const;

  /**  Returns the number of function calls in the budget; if the
   value is equal to NO_MAX, will terminate based only on refinement
   of the design
   @return long*/
  long GetMaxCalls() const;

  /** Use to set a function call budget; by default set to the 
   value of NO_MAX, which indicates no budget, in which case the search will
   terminate based only upon grid refinement.
     @param calls the number of function calls to which maxCalls will be set
     @return void */
  void SetMaxCalls(long calls);

  /**Sets maxCalls to the value of calls, and sets exact_count to true,
     so search will terminate precisely when the call budget runs out.
     @param calls the number of calls in the call budget
     @return void  */
  void SetMaxCallsExact(long calls);

  /**  Returns a pointer to a matrix initialized from the design matrix.
      @return Matrix<double>*  */
  Matrix<double> * GetDesign(); 

  /** Deep copy of the pattern to a Matrix pointer. 
      Points to a newly allocated chunk of memory upon return.
      The user should pass just a pointer, without preallocated memory.
      The user has the responsibility of using {\bf delete} to free the 
      memory thus allocated when it is no longer needed.
       @param designPtr a reference to a Matrix object.  Should be void going in.
       @return void*/
  void CopyDesign(Matrix<double>* &designPtr) const; 

  /** Returns stoppingStepLength, the smallest step length allowed in the search.
      @return double */
  double GetStoppingStepLength() const;

  /** Sets stoppingStepLength, the smallest step length allowed in the search, to 
      the value of the input parameter.
        @param len  the new value for stoppingStepLength
    @return void */
  void SetStoppingStepLength(double len);
  
  /** Allows the user to specify that the search will terminate precicely
      upon completion of maxCalls number of function calls. 
      NOTE:  If you wish to set "exact", you MUST specify maxCalls; otherwise,
      if the default of -1 is set for maxCalls, the program will take you at your
      word and exit the search before any evaluations are made. 
      @return bool */
  virtual void SetExact();

 /** Allows the user to specify that the search will terminate upon completion
      of roughly maxCalls number of function calls, but only
      after a pattern decrease or "shrink" step.

      @return bool */
  virtual void SetInexact();

  /** Returns the value of exact_count(q.v.)
      @return bool */
  virtual bool IsExact();

  // agodemar hack
  /** Allows the user to pass a different function pointer with respect to the one
      used in the costructor.
      @param name  the function pointer. The function pointed to has the signature
	             void f(long, Vector<double> &, double & , bool &, void*)
      @return void */
  void SetFcnName( void(*name)(long, Vector<double> &, double & , bool &, void*) ){ fcn_name=name;}

  //@}  //accessors/mutators 

protected:


  /**Exploratory Moves is an unimplemented virtual void function in the abstract
     base classes.  It will be implemented in the concrete classes as the workhorse
     for the algorithmic implementations */
  virtual void ExploratoryMoves() = 0;
  

  /**  Gives default stopping criteria based on maxCalls and stoppingStepLength 
      @return bool */
  virtual bool Stop();

  /** Indirection of the call to the objective function for purposes of keeping an accurate 
  tally of the number of function calls
    @param n the dimension of the problem
    @param x the point at which to evaluate the function
    @param f upon return will hold the function value at the point x
    @param flag indicates whether the evaluation was possible
    @param nothing a pointer to be used in cases where extra information is needed. Will usually be set to NULL. 
    @return void  */
  void fcnCall(long n, Vector<double> &x, double & f, bool& flag, void * nothing);

  /**Used to implement the overloaded assignment operator.
     @param Original a reference to a DirectSearch object.  Note that ordinarily this will actually be an object of a type of one of the concrete classes derived from DirectSearch.
     @return void  */
  virtual void CopySearch(const DirectSearch & Original);

  /** A utility method used to implement the exact_count choice.
      @return bool
  */
  bool BreakOnExact();

  /**A pointer to a design matrix, with each trial direction a column */
  Matrix<double> * design;  

  /** the dimension of the search; i.e. the number of variables in
   the problem */
  long dimension;             

  /** the point having the lowest value so far */
  Vector<double> * minPoint;  
 
  /** best objective function value calculated thus far */
  double minValue;  
          
  /** tally of the number of function call */
  long functionCalls;   
  
  /**budget for number of function calls.  Set by default to -1.
   If not reset this means the program will terminate only based
   on grid refinement. */    
  long maxCalls;   

  /** the smallest steplength that will be allowed in the search.  When the
      step length reaches this value, the search should end. */
  double stoppingStepLength;

  /** a void pointer, to be used in cases where additional information must be
      handled by the search. Set by default to NULL */
  void* some_object;
 
  /**  The function to be minimized.  Note that it must be formulated--or wrapped in 
       some enclosing function formulated--to take the parameters as declared.  
       Ordinarily the last parameter would be set to NULL.  
        @see void* some_object  */ 
  void(*fcn_name)(long dim, Vector<double> &x, double & function, bool & success, void* an_object);
 
  /** A flag that determines whether to let the function call budget
      go slightly over so the search will wait until it has ordered a
      decrease in delta. Useful mostly forthe pattern searches.
      See Dolan, Lewis, and Torczon, "On the Local convergence of
      Pattern Search."
      Default is true in DirectSearch class, but this may be overwritten
      in subordinate a.b.c.'s, e.g. PatternSearch class.
  */
  bool exact_count;

  /** The ID number of the search.  It can be used, as in MAPS,
      for identifying what sort a search a pointer is pointing to.
  */
  int IDnumber;

#if defined(AGO_DIRECTSEARCH)

  ofstream * ofile;

#endif

};
#endif










