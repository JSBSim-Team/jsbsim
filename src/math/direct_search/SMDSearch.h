/*SMDSearch.h
 *Declarations of Sequential version of Torczon's Multi-Directional Search
 *Adam Gurson College of William & Mary 2000
 *
 * DirectSearch version, modified by
 * P.L. (Anne) Shepherd, 1/2001
 */

#ifndef _SMDSearch_
#define _SMDSearch_

#include <math/direct_search/SimplexSearch.h>


#ifndef DEBUG
#define DEBUG 0
#endif


/** Declarations of Sequential version of Torczon's Multi-Directional Search
  @author Adam Gurson College of William & Mary 2000, modified by
    P.L. Shepherd 1/01
*/
class SMDSearch : public SimplexSearch
{ 
 public:
   
    static const long NOT_YET_INIT = -1;

   /** @name Constructors and destructor */
   //@{


  /**Constructor.  This constructor has two parameters.  Other data
     members will be set to default values, as follows.
      This is of course in addition to assignments and 
     initializations made in DirectSearch and SimplexSearch classses.
    \begin{itemize}
    \item[] delta = -1.0
    \item[] memory is allocated for fields simplexVBits, refSimplex,
        refSimplexValues, and refSimplexVBits.
    \item[] IDnumber = 3300
    \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search. This will be
        initialized as the minPoint.
  */ 
   SMDSearch(long dim, Vector<double> &startPoint);

    /** Copy constructor
	@param Original a reference to the object from which to
           construct *this */
   SMDSearch(const SMDSearch& Original);


   /**  Constructor which allows shrinking coefficient initialization. 
	sigma will be set to the value of sig by the SimplexSearch
        constructor with the same signature.  Other fields will be 
	assigned as in SMDSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint the start point for the search. This will be
          initialized as the minPoint.
	@param NewSigma the user-defined value for sigma, the shrinking
          coefficient. */
   SMDSearch(long dim, Vector<double> &startPoint, double NewSigma);

 /**  Constructor which allows shrinking coefficient
	initialization and specification of starting_edgeLengths. 
	sigma will be set to the value of sig by SimplexSearch
        constructor with the same signature, and  
	starting_edgeLengths will be initialized from lengths.
        Other fields will be assigned
	as in SMDSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint reference to a Vector representing the start point 
           for the search. This will be initialized as the minPoint.
	@param NewSigma the user-defined value for sigma, the shrinking
           coefficient.
        @param lengths reference to a Vector of doubles representing the
           desired edgelengths of the simplex.  */ 
   SMDSearch(long dim, Vector<double> &startPoint, double NewSigma, Vector<double> &lengths);



  /**Special constructor using void function and object pointers.
     The objective function can be sent in as the fourth parameter 
     here; the last parameter is used for any other
     information that may be necessary; it is used in MAPS,
     for example, to send in an object from an outside class
      Usually, though, the final parameter will simply be 
     set to NULL.
     
  This constructor takes six input parameters. All are sent to
  the SimplexSearch constructor with the same signature.
   Other fields are set by default as in
  SMDSearch(long dim, Vector<double> &startPoint).

  @param dim the dimension of the problem
  @param startPoint a Vector of doubles, the starting point for the search
  @param startStep will be used as the edge length of a fixed-length
     right simplex.
  @param stopStep the stopping step length for the search
  @param objective a pointer to the function to be minimized
  @param input_obj used to send additional data as needed--will
     normally be set to NULL.
  */ 
   SMDSearch(long dim, 
         Vector<double> &startPoint,
         double startStep,
         double stopStep,
         void (*objective)(long vars, Vector<double> &x, 
                           double & func, bool& flag,
                           void* an_obj),
         void * input_obj);

   /** Destructor.  Note that Matrix and Vector classes have
       their own destructors. */
   virtual ~SMDSearch();
   //@} // constructors and destructor

   /** @name Other public methods */
   //@{

   /** Overloaded assigment operator.
       @param A search to assign to *this.
       @return  SMDSearch& 
   */
   SMDSearch& operator= (const SMDSearch& A);

   /**Sets the search in motion.  
      @return void */
   void BeginSearch();

   /** Deletes any existing simplex and replaces it with a regular
    triangular simplex.  
   
    edgelengths[] represents  the length of each edge of the "triangle":
    in this case, all entries of edgelengths[] should be equal.
   
    functionCalls is reset to 0
    delta is set to edgeLengths[0]
    @return void  */
   
   void ChooseRegularSimplex();

   /**Deletes any existing simplex and replaces it with a right-angle
    simplex.
   
     The starting point will be the "origin" of the
     simplex points (it will be a part of the simplex and
     its function value is found here)
     edgeLengths points to an array of n doubles, where n is the
       dimension of the given search. x_1 will then be located
      a distance of edgeLengths[0] away from the basepoint along the
       the x_1 axis, x_2 is edgeLengths[1] away on the x_2 axis, etc.
   
   functionCalls is reset to 0
   delta is set to the largest value in edgeLengths[] 
	@return void */
   void ChooseRightSimplex();

   
   /**   May also pass cin as input stream if desired.
    Input the values of each trial point
    NOTE: THIS FUNCTION WILL ONLY ACCEPT n+1 POINTS
   
    NOTE: assumes that the basePoint(which will be initialized as
      the minPoint) is the last point entered
   
    functionCalls is reset to 0
    delta is set to the length of the longest simplex side.
      @param fp reference to an input stream
      @return void  
   */

   void ReadInFile(istream& fp);

   /**  Performs a deep copy of the simplexVBits array to a long pointer.
        Points to a newly allocated chunk of memory upon return
        USER SHOULD PASS JUST A NULL POINTER, WITHOUT PREALLOCATED MEMORY. 
	@param simVBits a reference to a pointer of type long int.
	@return void  */
   void GetCurrentSimplexVBits(long* &simVBits) const;


   /** Prints out the primary simplex points by row. Also  
    prints the corresponding f(x) values and validity status of each
    point, and the number of function calls thus far.
     @return void 
   */
   void PrintDesign() const;


   /** Prints out the reflection simplex points by row. Also  
    prints the corresponding f(x) values and validity status of each
    point, and the number of function calls thus far.
     @return void 
   */
   void printRefSimplex() const;
   //@} //other public methods

 protected:


   /** This method does the actual work of the search. It uses the 
       SMD algorithm to find the function minimum.
       @return void */
   void ExploratoryMoves();

   /** Returns true if the stopping criteria have been satisfied.
       @return bool  */
   bool Stop();


   /** Inititializes a regular simplex,  following an algorithm given
       by Jacoby, Kowalik, and Pizzo in
       "Iterative Methods for Nonlinear Optimization 
       Problems," Prentice-Hall (1972).  This algorithm also appears in 
       Spendley, Hext, and Himsworth, "Sequential Application of Simplex 
       Designs in Optimisation and Evolutionary Operation," Technometrics, 
       Vol. 4, No. 4, November 1962, pages 441--461.
       Note that Input matrix plex must be of the correct dimensions.

       @param plex pointer to a Matrix of doubles, of appropriate size.
       @return void  */
   void InitRegSimplex(); 


   /** Inititializes a right simplex. Input matrix plex must be of
       the correct dimensions.
       @param plex pointer to a Matrix of doubles, of appropriate size.
       @return void  */
   void InitRightSimplex();


  /** Deletes any existing design matrix and replaces it with one pointed
      to by plex.  functionCalls is set to 0.
      The basePoint is the minPoint and will be the "origin" of the
      simplex points (it will be a part of the simplex and
      its function value is calculated here).
       @param plex pointer to a Matrix of doubles, from which design
         will be initialized.
       @return void  */
   void InitGeneralSimplex(Matrix<double> *plex);

   /** Used to implement the overloaded assignment operator
       @param Original reference to the search to be copied
       @return void  */
   void CopySearch(const SMDSearch & Original);

   /** Creates a reflection simplex from the primary simplex.
       @return void */
   void CreateRefSimplex();

   /** Swaps the primary and reflection simplices. 
       This method removes the need to delete and
       reallocate memory by simply swapping pointers
       and using the same two "simplex memory slots"
       for the entire search.
       @return void */ 
   void SwitchSimplices();
  
   /**This function goes through the primary simplex and reduces the
    lengths of the edges adjacent to the best vertex.
    @return void */
   void ShrinkSimplex();

   /** This is used to find another INVALID point in the simplex.
    if all points are VALID, returns 0, otherwise 1.
    @param index a reference to a long, representing the current index
    @param validBits reference to a pointer to an array of longs, here 
    representing a validity array. */
   long GetAnotherIndex(long& index, long*& validBits);

   /** Like CalculateFunctionValue(), but for the Reflection Simplex
   (A user should not directly manipulate the reflection simplex,
   hence the private status of this function)
   @param index the index in refSimplexValues at which to store the
     function value.
   @return void 
   */
   void CalculateRefFunctionValue(long index);

   /**  Valid bits for the simplex values */
   long *simplexVBits; 
  
   /** Used to step through the arrays  */   
   long currentIndex;             

   /**  Used to step through the arrays */
   long refCurrentIndex;          

   /** The reflection simplex */
   Matrix<double> *refSimplex;      

   /** f(x) values corresponding to the simplex points. */
   double *refSimplexValues;  

   /**  Valid bits for the simplex values */   
   long *refSimplexVBits;          


};


#endif
