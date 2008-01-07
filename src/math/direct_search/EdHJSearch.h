/*  EdHJSearch.h 
 *  header file for an edited Hooke and Jeeves search based 
 *  on the class PatternSearch.
 *  Includes a minor modification to the basic Hooke and Jeeves strategy to
 *  avoid making pattern steps directly after pattern contractions (which mostly cover
 *  the same ground that was already covered in the search step preceding 
 *  the contraction.
 *  For a good description of the Hooke and Jeeves search algorithm
 *  I recommend Non-Linear Optimization Techniques by Box, Davies,
 *  and Swann, 1969. 
 *  Liz Dolan, The College of William and Mary, 1999
 *
 *                                                                 
 *  The author of this software is Elizabeth  D. Dolan.                     
 *  Permission to use, copy, modify, and distribute this software  
 *  for any purpose without fee is hereby granted, provided that   
 *  this entire notice is included in all copies of any software   
 *  which is or includes a copy or modification of this software   
 *  and in all copies of the supporting documentation for such     
 *  software.  THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT    
 *  ANY EXPRESS OR IMPLIED WARRANTY.
 *
 *  DirectSearch version, modified by Anne Shepherd,
 *  College of William and Mary, 2000-2001     
 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

 */                                                                 

#ifndef _EdHJSearch_
#define _EdHJSearch_

#include <math/direct_search/PatternSearch.h>


/**  Liz Dolan's Edited Hooke and Jeeves search based on the class
     PatternSearch
     For a good description of the Hooke and Jeeves search algorithm
     I recommend Non-Linear Optimization Techniques by Box, Davies,
     and Swann, 1969.
 
 @author  Liz Dolan, The College of William and Mary, 1999,
  modified by P.L.(Anne) Shepherd, 2000-2001.
*/                                             
class EdHJSearch: public PatternSearch
{

  public :

  /**@name Constructors & Destructor */  
  //@{

  /**Constructor.  This constructor has two parameters.  It calls
     the PatternSearch constructor with the same signature, then
     initializes three additional member fields with default values as
     follows:
   
     step = initialStepLength;  
     factor = 0.5;
     IDnumber = 2400;

     @param dim the dimension of the problem
     @param startpoint the start point for the search
     @see PatternSearch::PatternSearch(long dim,
          Vector<double> &startPoint)
  */
  EdHJSearch(long numberOfVariables, Vector<double> &startPoint); 

  /** Copy constructor.  Makes a deep copy of the search by calling the
      PatternSearch copy constructor and then initializing the additional
      two fields.  The new search will have the same values as those of
      the input parameter Original. 
        @param Original the search to be copied
  */
  EdHJSearch(const EdHJSearch & Original); 
  /**Special constructor using void function and object pointers.
     This constructor calls the PatternSearch constructor
     with the same signature, then initializes
     the three additional member fields as in 
     EdHJSearch(long numberOfVariables, Vector<double> &startPoint); 

  This constructor takes six input parameters.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  startStep the beginning delta, or lattice step length 
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will
      normally be set to NULL.
  @see  PatternSearch(long dim, Vector<double> &startPoint,
                          double startStep,  double stopStep,
			  void (*objective)(long vars, Vector<double> &x, 
					    double & func, bool& flag,
                                            void* an_obj),
			  void * input_obj) */
  EdHJSearch(long dim, 
	     Vector<double> &startPoint,
	     double startStep,
	     double stopStep,
	     void (*objective)(long vars, Vector<double> &x, 
			       double & func, bool& flag, void* an_obj),
	     void * input_obj);

  /** Destructor */
  ~EdHJSearch();

  //@} //constructors and destructors
  /** @name Other public methods */
  //@{

  /**Overloaded assignment operator.
      @param A the search to be copied
      @return EdHJSearch&  */ 
  virtual EdHJSearch& operator=(const EdHJSearch& A); 
 
  /** BeginSearch() will call the methods that implement the actual 
     compass search algorithm. 
     @return void */
  void BeginSearch();

  /** Overrides the PatternSearch version with an acurate length. 
      @return double */
  double GetDelta();


  /** Reinitializes values in order to reuse the same search object.
      This version of CleanSlate takes two parameters. It calls the
      PatternSearch version of CleanSlate with the same signature, and
      Other data members are set by default as in the constructor 
      EdHJSearch(long, Vector<double>&).
      @param dim the dimension of the problem
      @param startPoint the new starting point for the search
      @return void
      @see CleanSlate(long dim, Vector<double> &startpoint,
                      double* startStep, double stopStep,
     		      void (*objective)(long vars, Vector<double> &x, 
					double & func, bool& flag,
                                        void* an_obj),
		      void * input_obj)
  */
  void CleanSlate(long dimensions, Vector<double> &startPoint);

  /**Overloaded version of CleanSlate using void function
    and object pointers. 
    This version of CleanSlate takes six parameters. It calls the 
    PatternSearch method with the same signature and sets additional
    fields to default values as in the constructors.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  startStep the beginning delta, or lattice step length 
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will
       normally be set to NULL.
  @return void
  @see CleanSlate(long dim, Vector<double> &startpoint) */
  void CleanSlate(long dim, 
		  Vector<double> &startPoint,
		  double startStep,
		  double stopStep,
		  void (*objective)(long vars, Vector<double> &x, double & func, 
				    bool& flag, void* an_obj),
		  void * input_obj);
  //@} //other public methods

 protected :
  
  /** Used to implement the overloaded assignment operator.
      @param Original a reference to an EdHJSearch object
      @return void */
  void CopySearch(const EdHJSearch & Original);

 
  /**Exploratory Moves does the real work of the search. It uses
     Liz Dolan's  modified Hooke and Jeeves algorithm to minimize
     the objective function.
     @return void */
 void ExploratoryMoves();

 /** Makes certain search has not already exceeded maxCalls or 
  is stepping at less than stoppingStepLength.
     @return stop */
  bool Stop();

  /**step length */
   double step; 

   /** Factor by which the step is reduced */ 
   double factor;
};//class EdHJSearch


#endif










