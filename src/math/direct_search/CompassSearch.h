/* CompassSearch.h 
 * header file for a search based on the class PatternSearch.
 * A compass search checks the positive and negative quardinate vectors for
 * each dimension until improvement in the function values is found.  
 * The search then relocates to the improving point and begins again.
 * Liz Dolan, The College of William and Mary, 1999
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
 *  Direct Search Version,
 *  Modified by Anne Shepherd, College of William and Mary,
 *  2001
 */      
                                      

#ifndef _CompassSearch_
#define _CompassSearch_

#include <math/direct_search/PatternSearch.h>

/** Used in CompassSearch::UpdatePattern() for scaling the pattern */
const double SCALE_FACTOR = 0.5;
          
/**The CompassSearch class is derived from the  PatternSearch class.
  A compass search checks the positive and negative quardinate vectors for
  each dimension until improvement in the function values is found.  
  The search then relocates to the improving point and begins again.

  @author Liz Dolan, The College of William and Mary, 1999,
    modified by P.L. Shepherd, 7/00
*/                  
class CompassSearch: public PatternSearch
{

  public : 
   
  /**@name Constructors & Destructor */  
  //@{

  /**Constructor.  This constructor has two parameters.  All it does is call
     the PatternSearch constructor with the same signature, then change
     its ID number to 2200.
     
     @param dim the dimension of the problem
     @param startpoint the start point for the search
     @see PatternSearch::PatternSearch(long dim, Vector<double> &startPoint)
  */
  CompassSearch(long numberOfVariables, Vector<double> &startPoint); 


  /**Special constructor using void function and object pointers. This
     constructor merely calls the PatternSearch constructor with the
     same signature, then change its ID number to 2200.
     
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
					    double & func, bool& flag, void* an_obj),
			  void * input_obj) */

  CompassSearch(long dim, 
	      Vector<double> &startPoint,
	      double startStep,
	      double stopStep,
	      void (*objective)(long vars, Vector<double> &x, 
				double & func, bool& flag, void* an_obj),
	      void * input_obj);
  
  /** Destructor */
  virtual  ~CompassSearch();

  //@}  // constructor and destructor

  /** @name Other public methods */
  //@{

  /** Overloaded assignment operator 
       @param A the search to be assigned to *this.
       @return  CompassSearch& */
  CompassSearch& operator=(const CompassSearch &A);

  /** BeginSearch() will call the methods that implement the actual 
     compass search algorithm. 
     @return void */
  void BeginSearch();

  //@} //search method

  protected :


  /**Exploratory Moves does the real work of the compass search.
     @return void */
  void ExploratoryMoves();

  /**Initializes the pattern to one that contains one positive 
  and one negative unit vector in each of the compass directions.
  @return void */
  void CreatePattern();

  /** Calls the PatternSearch method {\bf ScalePattern()} to scale
      the trial steps by SCALE_FACTOR. SCALE_FACTOR is typically 0.5,
      so the steplength halves itself.
      @return void */
  void UpdatePattern();
};//class CompassSearch

#endif








