/* NLessSearch.h 
 * header file for a regular simplex search based on 
 * the PatternSearch class
 * The NLessSearch searches about a regular simplex (i.e. 
 * minimal positive basis) until finding improvement in the
 * objective function value.  Then the search relocates to the
 * improving point and begins again.
 * Liz Dolan, The College of William & Mary, 1999
 *
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
 *  DirectSearch version, modified by Anne Shepherd,
 *  College of William and Mary, 2000-2001
 *
 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

 */                                                                 

#ifndef _NLessSearch_
#define _NLessSearch_

#include <math/direct_search/PatternSearch.h>

/** NlessSearch is derived from PatternSearch.  It searches using a 
    search lattice based on a simplex.
    
    @author Elizabeth Dolan, modified by P.L.(Anne) Shepherd, 2000-2001
*/  
class NLessSearch: public PatternSearch
{
  public :

  /**@name Constructors & Destructor */  
  //@{

  /**Constructor for NLessSearch.  This constructor has two parameters.
     All it does is call the PatternSearch constructor with
     the same signature, then change the IDnumber to 2500.

     @param dim the dimension of the problem
     @param startpoint the start point for the search
     @see PatternSearch::PatternSearch(long dim, Vector<double> &startPoint)
  */
  NLessSearch(long numberOfVariables, Vector<double> &startPoint);


  /**Special NLessSearch constructor using void function and
     object pointers. This constructor
     merely calls the PatternSearch constructor with the same signature,
     then changes the IDnumber to 2500.
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
					    double & func,
                                            bool& flag, void* an_obj),
			  void * input_obj) */  
  NLessSearch(long dim, 
	      Vector<double> &startPoint,
	      double startStep,
	      double stopStep,
	      void (*objective)(long vars, Vector<double> &x, 
				double & func, bool& flag,
                                void* an_obj),
	      void * input_obj);
 
  /** Destructor */ 
  ~NLessSearch();
  //@} // constructors and destructor

  
  NLessSearch& operator=(const NLessSearch &A);

  /** @name Search Method */
  //@{
  
  /**BeginSearch will call the methods that implement 
     the actual NLessSearch algorithm. 
     @return void */
  void BeginSearch();
  //@}  // search method

  private:
  /** @name Private methods */
  //@{

  /** Exploratory Moves does the main work of the algorithm.
      @return void */
  void ExploratoryMoves();

  /** CreatePattern() builds a regular simplex
     @return void*/
  void CreatePattern();


  /** Calls the PatternSearch method ScalePattern to scale the 
      trial steps by half.
      @return void */
  void UpdatePattern();

  /** Rescales the simplex to the desired size 
      @param dimens the dimension of the matrix 
      @param pat a reference to a design Matrix.
      @param size the desired size
      @return void */      
  void SizePattern(long dimens, Matrix<double> & pat, double size);
};//class NLessSearch
//@} //private methods
#endif









