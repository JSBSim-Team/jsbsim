/*NMSearch.h
 *declarations of Nelder Mead Simplex Search
 *Adam Gurson College of William & Mary 1999
 *Modified by P.L. (Anne) Shepherd, 1/2001
 */

#ifndef _NMSearch_
#define _NMSearch_

#include <math/direct_search/SimplexSearch.h>

#ifndef DEBUG
#define DEBUG 0
#endif


/**NMSearch class. A simplex search using the method described by
Nelder and Mead.

 @author Adam Gurson College of William & Mary 1999, revised
    by P.L. Shepherd, 1/2001
 */
class NMSearch : public SimplexSearch
{ 
 public:

  /** @name constructors and destructors */
  //@{

  /**Constructor.  This constructor has two parameters.
     Other data members will be set to default values, as
     follows.  This is of course in addition to assignments and 
     initializations made in DirectSearch and SimplexSearch classes.
    \begin{itemize}
    \item[]  alpha = 1.0; 
    \item[]  beta = 0.5;  
    \item[]  gamma = 2.0; 
    \item[]  memory is allocated for fields reflectionPt, expansionPt,
       and contractionPt.
    \item[] IDnumber = 3200
    \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search. This will
        be initialized as the minPoint.
  */
   NMSearch(long dim, Vector<double> &startPoint );


   /**  Constructor which allows shrinking coefficient
	initialization and specification of starting_edgeLengths. 
	sigma will be set to the value of sig by the SimplexSearch
        constructor with the same signature, and  
	starting_edgeLengths will be initialized from lengths.
        Other fields will be assigned
	as in NMSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint the start point for the search. This will
           be initialized as the minPoint.
	@param sig the user-defined value for sigma, the shrinking
           coefficient.
        @param lengths reference to a Vector of doubles representing
           the desired edgelengths of the simplex.  */
   NMSearch(long dim, Vector<double> &startPoint, 
	    double sig, Vector<double> &lengths); 


   /**  Constructor which allows initialization of all four coefficients.
	sigma will be set to the value of NewSigma by the SimplexSearch
        constructor with the same signature, and the other coefficients will 
	be initialized here. Other fields will be assigned
	as in NMSearch(long dim, Vector<double> &startPoint).

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
   NMSearch(long dim, Vector<double> &startPoint,
            double NewSigma, double NewAlpha, 
	    double NewBeta, double NewGamma);


  /** Copy constructor
	@param Original a reference to the object from which to construct *this */
   NMSearch(const NMSearch& Original);

 
  /**Special constructor using void function and object pointers.
     The objective function can be sent in as the fourth 
     parameter here; the last parameter is used for any other
     information that may be necessary; it is used in MAPS,
     for example, to send in an object from an outside class.
     Usually, though, the final parameter will simply be 
     set to NULL.
     
  This constructor takes ten input parameters. The arguments dim,
  startPoint, startStep, stopStep, objective, and input_obj
  are sent to one of the SimplexSearch constructors for assignment; 
  NewSigma, NewAlpha, NewBeta, and NewGamma are assigned in this
  constructor.Other fields are set by default as in
  SHHSearch(long dim, Vector<double> &startPoint).

  @param dim the dimension of the problem.
  @param startPoint a Vector of doubles, the starting point for the
    search.
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
  @param objective a pointer to the function to be minimized
  @param input_obj used to send additional data as needed--will
    normally be set to NULL.
  */   
   NMSearch(long dim, 
	    Vector<double> &startPoint,
	    double NewSigma, double NewAlpha,
	    double NewBeta, double NewGamma,
	    double startStep,
	    double stopStep,
	    void (*objective)(long vars, Vector<double> &x, 
			      double & func, bool& flag,
                              void* an_obj),
	    void * input_obj);

   /** Destructor */
   ~NMSearch();

   //@} //constructors and destructor

   /** @name other public methods */
   //@{

   /** Overloaded assignment operator 
       @param A the search to be assigned to *this.
       @return  NMSearch&
   */
   NMSearch& operator= (const NMSearch& A);

   /** May also pass cin as input stream if desired.
    input the values of each trial point.
    NOTE: THIS FUNCTION WILL ONLY ACCEPT n+1 POINTS
   
    functionCalls is reset to 0 and ALL FUNCTION VALUES ARE CALCULATED.
     @param fp reference to an input stream.
     @return void  */
   void ReadInFile(istream& fp);

   /** Specifies that a right simplex will be used.
     @return void  */
   void ChooseRightSimplex();

   /** Specifies that a regular simplex will be used.
     @return void  */
   void ChooseRegularSimplex();

    /**This method begins the actual searching. 
      @return void */
   void BeginSearch();

   /** Sets the reflection coefficient
       @param newAlpha the new value for alpha 
       @return void */
   void SetAlpha(double newAlpha);

    /** Returns the value of alpha, the reflection coefficient
       @return double */
   double GetAlpha() {return alpha;}
       
   /** Sets the contraction coefficient
       @param newBeta the new value for Beta 
       @return void */
   void SetBeta(double newBeta);
   
   /** Returns the value of beta, the contraction coefficient
       @return double */   
   double GetBeta(){ return beta;}


   /** Sets the expansion coefficient 
       @param newGamma the new value for Gamma 
       @return void */
   void SetGamma(double newGamma);
   
   /** Returns the value of gamma, the expansion coefficient
       @return double */    
   double GetGamma() {return gamma;}

   /** Overloaded to return NMdelta, in this search overloaded to equal
       the mean of all the edgelengths of the simplex
       @return double
   */
   double GetDelta();

   //@} //other public methods

 protected:
 

   /** This method does the actual work of the search.  It uses the 
       Nelder Mead algorithm  to find the function minimum.
       @return void */
   void ExploratoryMoves();

   

   /** Deletes any existing simplex and replaces it with a regular
       triangular simplex.
   
    minPoint points to a point that will be the "origin" of the
    simplex points (it will be a part of the simplex)
    starting_edgeLengths[0] is the length of each edge of the "triangle."
 
    functionCalls is reset to 0 and ALL FUNCTION VALUES ARE CALCULATED.
      @param plex pointer to a Matrix of doubles representing a simplex.
      @return void  */
   void InitRegSimplex();


   /**  Deletes any existing simplex and replaces it with a right-angle
    simplex.
   
    starting_edgeLengths points to an array of n doubles, where n is the
       dimension of the given search. x_1 will then be located
       a distance of edgeLengths[0] away from the basepoint along the
       the x_1 axis, x_2 is edgeLengths[1] away on the x_2 axis, etc.
   
    functionCalls is reset to 0 and ALL FUNCTION VALUES ARE CALCULATED.
      @param plex pointer to a Matrix of doubles representing a simplex
      @return void  
   */
   void InitRightSimplex();

   /** Used to implement all simplex initializations.  
       @param plex pointer to a Matrix of doubles representing a simplex
       @return void */
   void InitGeneralSimplex(const Matrix<double> *plex);

   /** Returns true if the stopping criteria have been satisfied. 
       @return bool */
   bool Stop();

   /** Calculates NMdelta, the mean of the lengths
       of the edges of the simplex.
       @return void
   */
   void CalculateNMDelta();
   
   /** Used to implement the overloaded assignment operator
       @param Original reference to the search to be copied
       @return void  */
   void CopySearch(const NMSearch & Original);

   /** Sets minIndex to the simplex index of the point which generates
    the lowest value of f(x) and sets maxIndex to the simplex index
    of the point which generates the highest value of f(x). 
       @return void 
   */
   void FindMinMaxIndices();


   /** Returns simplex index of the point which
       generates the second highest value of f(x).
       @return long */
   long SecondHighestPtIndex();


   /** Finds the centroid 
       @return void  */
   void FindCentroid();


   /** Finds the reflection point and sets its f(x) value.
       @return void */
   void FindReflectionPt();

   /**  Finds the expansion point and sets its f(x) value. 
	@return void */
   void FindExpansionPt();
  
   /**  Finds the contraction point and sets its f(x) value.
	@return void */
   void FindContractionPt();
  

   /**The reflection coefficient */ 
   double alpha;                 

   /** The contraction coefficient */
   double beta;                 

   /** The expansion coefficient */
   double gamma;                  

   /** The index of point generating max f(x) */
   long maxIndex;                  

   /** The reflection point */
   Vector<double> *reflectionPt;   

   /** The value of f(reflectionPt) */
   double reflectionPtValue;     

   /** The expansion point */ 
   Vector<double> *expansionPt;  

   /** The value of f(expansionPt) */
   double expansionPtValue;      

   /** The contraction point */
   Vector<double> *contractionPt;

   /** The value of f(contractionPt) */
   double contractionPtValue;    

   /** min(f(maxIndexPoint),reflectionPtValue) */ 
   double maxPrimePtValue;      

   /**  Set by FindContractionPt() and used in ExploratoryMoves() to 
	branch in possibility 3. */
   long maxPrimePtId;

   /** Analogous to delta in the other searches, but here it is
       the mean of the lengths of all the edges of the simplex.
       Used to decide whether to stop the search, unless Stop_on_std
       is set to true.  Because it is expensive to calculate, we
       do so only when necessary. */
   double NMdelta;

};


#endif

