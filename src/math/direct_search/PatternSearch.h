/* PatternSearch.h
 * declarations of the PatternSearch base class member 
 * functions and data for optimization
 * Liz Dolan College of William and Mary 1999
 *                                                                 
 *  The author of this software is Elizabeth  D. Dolan.                     
 *  Permission to use, copy, modify, and distribute this software  
 *  for any purpose without fee is hereby granted, provided that   
 *  this entire notice is included in all copies of any software   
 *  which is or includes a copy or modification of this software   
 *  and in all copies of the supporting documentation for such     
 *  software.  THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT    
 *  ANY EXPRESS OR IMPLIED WARRANTY.  IN PARTICULAR, THE AUTHOR    
 *  OFFERS NO REPRESENTATION OR WARRANTY OF ANY KIND                   
 *  CONCERNING THE MERCHANTABILITY OF THIS SOFTWARE OR ITS    
 *  FITNESS FOR ANY PARTICULAR PURPOSE. 
 *
 *  DirectSearch version, modified by P.L. Shepherd, 5/00-01/01
 *                         
 */                                                                 


#ifndef _PatternSearch_
#define _PatternSearch_

#include <math/direct_search/DirectSearch.h>

typedef char file[32];
/**The PatternSearch class is derived from the DirectSearch class.
   PatternSearch is 
   an abstract base class.  */
class PatternSearch : public DirectSearch
{
 public:
   
  /**@name Constructors & Destructor */  
  //@{

  /**Constructor for class PatternSearch.  This constructor has two parameters.  
     Other data members will be set
     to default values, as follows (in addition to data members set by
     the DirectSearch constructor):
     \begin{itemize}
     \item[] patternLength = 0
     \item[] initialStepLength = .25
     \item[] delta = initialStepLength
     \item[] IDnumber = 2000
     \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search
     @see DirectSearch::DirectSearch(long dim, Vector<double> &startPoint)
  */
  PatternSearch(long dim, Vector<double> &startPoint);

  /** Deep copy constructor for class PatternSearch
      @param Original a reference to the object to be copied.  Note that
      this will in ordinary practice actually be a member of a concrete
      class derived from PatternSearch. */
  PatternSearch(const PatternSearch & Original);


  /**Special constructor using void function and object pointers.
     The objective function can be sent in as the fifth parameter here;
     the last parameter is used for any other
     information that may be necessary; it is used in MAPS,
     for example, to send in an  object from an outside class.
     Usually, though, the final parameter will simply be 
     set to NULL.
  This constructor takes six input parameters.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  startStep the beginning delta, or lattice step length 
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will
      normally be set to NULL.
  */
  PatternSearch(long dim, 
	       Vector<double> &startPoint,
	       double startStep,
	       double stopStep,
	       void (*objective)(long vars, Vector<double> &x, 
				 double & func, bool& flag, void* an_obj),
	       void * input_obj);

  /** Destructor  */
  virtual ~PatternSearch();

  //@}  //constructors and destructor
  /** @name Other Initialization Methods*/
  //@{

  /** Overloaded assignment operator.
      note that because PatternSearch is an abstract base class,
      the object actually returned will be of a type corresponding
      to one of the  concrete classes.
      @param A reference to a const PatternSearch object
      @return  PatternSearch&
   */
  virtual PatternSearch& operator=(const PatternSearch& A);

  /** Reinitializes values in order to reuse the same search object.
      This version of {\bf CleanSlate} takes two parameters. Other
      data members are set by default as in the constructor
      PatternSearch(long, Vector<double>&).
      @param dim the dimension of the problem
      @param startPoint the new starting point for the search
      @return void
      @see CleanSlate(long dim, Vector<double> &startpoint,
                          double* startStep, double stopStep,
			  void (*objective)(long vars, Vector<double> &x, 
					    double & func, bool& flag, void* an_obj),
			  void * input_obj)
  */
  virtual void CleanSlate(long dim, Vector<double> &startpoint);


  /**overloaded version of {\bf CleanSlate} using void function
     and object pointers.  This version of CleanSlate takes six parameters.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  startStep the beginning delta, or lattice step length 
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will
       normally be set to NULL.
  @return void
  @see CleanSlate(long dim, Vector<double> &startpoint) */
  virtual void CleanSlate(long dim, Vector<double> &startpoint, double startStep,  double stopStep,
			  void (*objective)(long vars, Vector<double> &x, 
					    double & func, bool& flag, void* an_obj),
			  void * input_obj);

  /**Deletes any existing pattern and replaces it with the one pointed
     to by designPtr.
     Calls DesignSearch::InitializeDesign() and then initializes patternLength.
     @param patternSize the number of "columns", i.e. trial points, in design matrix
     @param pat a pointer to a design matrix 
     @return void */  
  void InitializeDesign(long patternSize, const Matrix<double> * designPtr);

 
  /**Reads the design in from a file. You may also pass cin as
     the input stream as desired.
     Input first the pattern length and then the values of each trial vector
     (i.e. input the pattern by column)  
     @param fp the filestream --or cin may be used
     @return void */
  void ReadInFile(istream & fp);

  /** Prints out useful information about the search */
  void PrintDesign() const;

  //@} // init methods
  /**@name Search method */
  //@{
  /**{\bf BeginSearch()} is an unimplemented virtual void method
     in the abstract base classes. It is to be implemented in the concrete 
     classes.
     There, BeginSearch() will call the methods that implement the actual 
     search algorithms.  */
  virtual void BeginSearch() = 0;

  //@} //search method


  /** @name Accessors */
  //@{
  
  
  /** Returns the number of "columns" of the pattern matrix
     @param pattern a long provided by the user; will be assigned
     the value of patternLength
      @return void */
  void GetPatternLength(long & pattern) const; 

  /**  Returns delta, the lattice step length
      @return double */
  double GetDelta() const;

  /**Returns initialStepLength, the initial value of delta.
      @param stepLen a reference to a double: will be assigned
      the value of initialStepLength
      @return void */
  void GetInitialStepLength(double & stepLen);

  /** Assigns the value of steplen to initialStepLength.
      @param stepLen a reference to a double:initialStepLength
      will be assigned this value.
      @return void */
  void SetInitialStepLength(double & stepLen);

  

  //@} //accessors and mutators

 protected:

 
  /** Whether nor not to stop, based either on maxCalls or lattice resolution
     @return bool */
  virtual bool Stop(); 

  /**PatternSearch::Exploratory Moves is an unimplemented virtual void function.
     It will be implemented in the concrete classes as the workhorse
     for the algorithmic implementations */
  virtual void ExploratoryMoves() = 0;

 
  /** Used to implement the overloaded assignment operator.
      @param Original a reference to a PatternSearch object
      @return void */
  virtual void CopySearch(const PatternSearch & Original);

  /**NextPoint() calculates the next trial point by adding the
     design matrix column at index to the current vector.
     Returns the prospect in nextPoint.
    @param index the index of a column of the design matrix
    @param currentPoint a reference to the current vector
    @param nextPoint a reference to a vector, which will be assigned
        the value of the appropriate point to be evaluated next.
    @return void */
  virtual void NextPoint(long index, const Vector<double> & currentPoint,
			 Vector<double> & nextPoint);

 
  /** Replaces the minimizer & the minimum objective function value.
     @param newPoint the point that will be assigned as the new minPoint
     @param newValue the value that will be assigned to minValue
     @return void */
  virtual void ReplaceMinimum(Vector<double> & newPoint, double  newValue); 

 
  /** Scale lattice step length by scalar 
      @param scalar the value by which to scale the pattern
      @return void */
  virtual void ScalePattern(double scalar);

  /** The number of "columns" in design matrix, i.e. trial points */
  long patternLength; 
 
  /** The density of the underlying lattice  */  
  double delta; 

  /** The initial setting for delta */           
  double initialStepLength;
 
};
#endif




