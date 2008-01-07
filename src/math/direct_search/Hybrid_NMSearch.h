/* Hybrid_NMSearch.h

    Header file for class Hybrid_NMSearch, a hybrid
    Nelder-Mead/Compass search.

    Anne Shepherd, College of William and Mary, 2000
 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

*/


#ifndef _HY_Search_
#define _HY_Search_

#include <math/direct_search/NMSearch.h>
#include <math/direct_search/EdHJSearch.h>
#include <iostream>
#include <fstream>
#if defined(sgi) && !defined(__GNUC__)
# include <math.h>
# include <stdlib.h>
#else
# include <cmath>
# include <cstdlib>
#endif

#ifndef DEBUG
#define DEBUG 0
#endif


/** Hybrid_NMSearch class. Comprises a Nelder-Mead search (NMSearch) up
 *  to the point where the standard deviation test is satisfied.  Then it
 *  switches over to an EdHJSearch. This avoids the expense of finding
 *  NMdelta (See discussion in design document and user's manual.)
 *
 *
 *    @author P.L. Shepherd, 1/2001
 */
class Hybrid_NMSearch : public NMSearch
{
public:
    
//constructors and destructor
    
  /** @name constructors and destructors */
  //@{

  /**Constructor.  This constructor has two parameters.
     Other data members will be set to default values, as
     follows.  This is of course in addition to assignments and 
     initializations made in DirectSearch, SimplexSearch, and
     NMSearch classes.
    \begin{itemize}
    \item ESearchStoplength = 10e-8;
    \item NSearchCalls = 0;
    \item ESearchCalls = 0;
    \item TotalCalls = 0;
    \item Stop_on_std = true;
    \item[] IDnumber = 3210
    \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search. This will
        be initialized as the minPoint.
  */
    Hybrid_NMSearch(long dim, Vector<double> &startPoint );



   /**  Constructor which allows shrinking coefficient
    * 	initialization and specification of starting_edgeLengths
        (for the NMSearch portion of the search). These arguments are
        sent to the NMSearch constructor.
        Other fields will be assigned
	as in HybridNMSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint the start point for the search. This will
           be initialized as the minPoint.
	@param sig the user-defined value for sigma, the shrinking
           coefficient.
        @param lengths reference to a Vector of doubles representing
           the desired edgelengths of the simplex.  */
    
    Hybrid_NMSearch(long dim, Vector<double> &startPoint, 
             double sig, Vector<double> &lengths);


    
   /**  Constructor which allows initialization of all four coefficients
        used in the NMSearch portion of the search.
        Again, most of the work is done by the NMSearch constructor.
        Other fields will be assigned
	as in Hybrid_NMSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint the start point for the search. This will
          be initialized as the minPoint.
	@param NewSigma the user-defined value for sigma, the shrinking
          coefficient.
        @param NewAlpha the user-defined value for alpha, the reflection
          coefficient.
        @param NewBeta the user-defined value for beta, the contraction
          coefficient.
        @param NewGamma the user-defined value for gamma, the expansion
          coefficient.
   */
    Hybrid_NMSearch(long dim, Vector<double> &startPoint,
                    double NewSigma, double NewAlpha, 
                    double NewBeta, double NewGamma);
    
    Hybrid_NMSearch(const Hybrid_NMSearch& Original);

    /** Adds one more parameter to the long NMSearch constructor,
        for the EdHJSearch stopping step length.
	@param dim the dimension of the problem.
	@param startpoint the start point for the search. This will
          be initialized as the minPoint.
	@param NewSigma the user-defined value for sigma, the shrinking
          coefficient.
        @param NewAlpha the user-defined value for alpha, the reflection
          coefficient.
        @param NewBeta the user-defined value for beta, the contraction
          coefficient.
        @param NewGamma the user-defined value for gamma, the expansion
          coefficient.
        @param startStep will be used as the edge length of a fixed-length
          right simplex.
        @param stopStep the stopping step length for the search
        @param EdHJStop the stopping step length forthe EdHJSearch
        @param objective a pointer to the function to be minimized
        @param input_obj used to send additional data as needed--will
          normally be set to NULL.
    */
    Hybrid_NMSearch(long dim, 
                    Vector<double> &startPoint,
                    double NewSigma, double NewAlpha,
                    double NewBeta, double NewGamma,
                    double startStep,
                    double stopStep,
                    double EdHJStop,
                    void (*objective)(long vars, Vector<double> &x, 
                                      double & func, bool& flag,
                                      void* an_obj),
                    void * input_obj);

   
   /** Destructor */
   ~Hybrid_NMSearch();

   //@} //constructors and destructor

   
   /** @name other public methods */
   //@{

   
   /**This method starts the actual searching. 
      @return void */
   void BeginSearch();

   /**This is overloaded to return TotalCalls.      
      @return long
   */
   long GetFunctionCalls() const{return TotalCalls;}

   /**Returns the value of NSearchCalls.    
      @return long
   */      
   long GetNSearchCalls(){return NSearchCalls;}
   
   /**Returns the value of ESearchCalls.    
      @return long
   */      
   long GetESearchCalls(){return ESearchCalls;}

   /**Returns the value of TotalCalls.      
      @return long
   */      
   long GetTotalCalls(){return TotalCalls;}
   
   //@} //other public methods
     
protected:
 
   /**stoppingStepLength for the EdHJSearch phase */
   double ESearchStoplength;
   
   /**the number of calls made during the NMSearch phase */
   long NSearchCalls;
   
   /**the number of calls made during the EdHJSearch phase */
   long ESearchCalls;
 
   /**the total number of calls made */ 
   long TotalCalls;
   
}; //class Hybrid_NMSearch
#endif
