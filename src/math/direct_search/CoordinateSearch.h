 /* CoordinateSearch.h 
 * header file for a search based on the class PatternSearch.
 * A Coordinate search checks the positive and negative quardinate vectors for
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
 *  DirectSearch version, modified by Anne Shepherd,
 *  College of William and Mary, 2001
 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

 */                                                                 

#ifndef _CoordinateSearch_
#define _CoordinateSearch_

#include <math/direct_search/PatternSearch.h>

/**CoordinateSearch class.
   A Coordinate search checks the positive and negative coordinate
   vectors for each dimension until improvement in the function values
   is found.  The search then relocates to the improving point and
   begins again.
   @author Liz Dolan, The College of William and Mary, 1999;
      modified by P.L. Shepherd 7/00
*/
class CoordinateSearch: public PatternSearch
{

  public :

  /**@name Constructors & Destructor */  
  //@{

  /**Constructor.  This constructor has two parameters.  All it does is call
     the PatternSearch constructor with the same signature, then change
     its ID number to 2100.

     @param dim the dimension of the problem
     @param startpoint the start point for the search
     @see PatternSearch::PatternSearch(long dim, Vector<double> &startPoint)
  */
  CoordinateSearch(long numberOfVariables, Vector<double> &startPoint); 



  /**Special constructor using void function and object pointers.
     This constructor merely calls the PatternSearch constructor
     with the same signature, then change
     its ID number to 2100.
  This constructor takes six input parameters.
  @param  dim the dimension of the problem
  @param  startPoint a Vector of doubles, the starting point for the search
  @param  startStep the beginning delta, or lattice step length 
  @param  stopStep the stopping step length for the search
  @param  objective a pointer to the function to be minimized
  @param  input_obj used to send additional data as needed--will normally be set to NULL.
  @see  PatternSearch(long dim, Vector<double> &startPoint,
                          double startStep,  double stopStep,
			  void (*objective)(long vars, Vector<double> &x, 
					    double & func, bool& flag,
                                            void* an_obj),
			  void * input_obj) */
  CoordinateSearch(long dim, 
		   Vector<double> &startPoint,
		   double startStep,
		   double stopStep,
		   void (*objective)(long vars, Vector<double> &x, 
				     double & func, bool& flag, void* an_obj),
		   void * input_obj);
 
  /** Destructor */
  virtual ~CoordinateSearch();
  //@} // constructors and destructor

  /** @name Other public methods */
  //@{

  /** Overloaded assignment operator 
       @param A the search to be assigned to *this.
       @return  CoordinateSearch& */
  CoordinateSearch& operator=(const CoordinateSearch &A);

  /** BeginSearch() will call the methods that implement the actual 
     coordinate search algorithm. 
     @return void */
  void BeginSearch();
  //@} //other public methods

  protected :

  /**Exploratory Moves does the real work of the search, using the
     Coordinate Search algorithm to find the function minimum.
     @return void */
  void ExploratoryMoves();

  /** Initializes the pattern to one that contains one positive 
  and one negative unit vector in each of the Coordinate directions.
     @return void */
  void CreatePattern();


  /** Scales the pattern trial steps to search half as far in each
      direction
     @return void */
  void UpdatePattern();

};//class CoordinateSearch


#endif








