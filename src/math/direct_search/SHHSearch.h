/*SHHSearch.h
 *declarations of Spendley, Hext and Himsworth Simplex Search
 *Adam Gurson College of William & Mary 1999
 *
 * directSearch version,
 * Modified by P.L.(Anne) Shepherd, 2000 - 2001
 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

 */

#ifndef _SHHSearch_
#define _SHHSearch_
#ifndef DEBUG
#define DEBUG 0
#endif


#include <math/direct_search/SimplexSearch.h>

/** The SHHSearch class implements the Spendley, Hext and Himsworth
    Simplex Search.

    @author Adam Gurson College of William & Mary 1999,
      revised by P.L. Shepherd, 2000 - 2001 */
class SHHSearch : public SimplexSearch 
{ 

 public:

  /**@name Constructors and destructor */
  //@{ 
  /**Constructor.  This constructor has two parameters.
     Other data members will be set to default values, as follows. 
     This is of course in addition to assignments and 
     initializations made in DirectSearch and SimplexSearch classes.
    
    \begin{itemize} 
    \item[]  simplexAges = NULL
    \item[] memory is allocated for field reflectionPt
    \item[] IDnumber = 3100
    \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search. This will be
       initialized as the minPoint.
  */
   SHHSearch(long dim, Vector<double> &startPoint);
   // default constructor


   /**  Constructor which allows shrinking coefficient initialization. 
	sigma will be set to the value of sig by the SimplexSearch constructor
        with the same signature.  Other fields will be assigned
	as in SHHSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint the start point for the search.
          This will be initialized as the minPoint.
	@param sig the user-defined value for sigma, the
          shrinking coefficient. */
   SHHSearch(long dim, Vector<double> &startPoint, double sig);


   /**  Constructor which allows shrinking coefficient
	initialization and specification of starting_edgeLengths. 
	sigma will be set to the value of sig by SimplexSearch
        constructor with the same signature, and  
	starting_edgeLengths will be initialized from lengths.
        Other fields will be assigned
	as in SHHSearch(long dim, Vector<double> &startPoint).

	@param dim the dimension of the problem.
	@param startpoint the start point for the search. This will
          be initialized as the minPoint.
	@param sig the user-defined value for sigma, the shrinking
          coefficient.
        @param lengths reference to a Vector of doubles representing
          the desired edgelengths of the simplex.  */
   SHHSearch(long dim, Vector<double> &startPoint, 
	     double sig, Vector<double> &lengths );

  /** Copy constructor
	@param Original a reference to the object from which to
          construct *this. */
   SHHSearch(const SHHSearch& Original);
   // deep copy constructor


  /**Special constructor using void function and object pointers.
     The objective function can be sent in as the fourth 
     parameter here; the last parameter is used for any other
     information that may be necessary; it is used in MAPS,
     for example, to send in an object from an outside class.
     Usually, though, the final parameter will simply be 
     set to NULL.
     
  This constructor takes six input parameters. All are sent
  to the SimplexSearch constructor with the same signature.
   Other fields are set by default as in
  SHHSearch(long dim, Vector<double> &startPoint).

  @param dim the dimension of the problem
  @param startPoint a Vector of doubles, the starting point for the search
  @param startStep will be used as the edge length of a fixed-length
    right simplex.
  @param stopStep the stopping step length for the search
  @param objective a pointer to the function to be minimized
  @param input_obj used to send additional data as needed--will
    normally be set to NULL.
  */ 
   SHHSearch(long dim, 
	     Vector<double> &startPoint,
	     double startStep,
	     double stopStep,
	     void (*objective)(long vars, Vector<double> &x, 
			       double & func, bool& flag,
                               void* an_obj),
	     void * input_obj);

   /** Destructor  */
   virtual ~SHHSearch();

   //@} //constructors and destructor

   /** @name Other public methods */
   //@{

   /** Overloaded assignment operator 
       @param A the search to be assigned to *this.
       @return  SHHSearch& */
   SHHSearch& operator=(const SHHSearch& A); 

   /** Begins the actual search, using the algorithm described by 
       Spendley, Hext and Himsworth.
       @return void  */
   void BeginSearch();


   /** Allows the user to choose a regular simplex for the search, rather
       than the default fixed length right simplex.
       @return void */   
   virtual void ChooseRegularSimplex();


  /** Deletes any existing simplex and replaces it 
    with a right-angle simplex in the following manner:
   
    minPoint points to the point that will be the "origin" or
    base point of the simplex points (it will be a part of the simplex)
    starting_edgeLengths points to a vector of n doubles, where n is the
    dimension of the given search. x_1 will then be located
    a distance of starting_edgeLengths[0] away from the base point along
    the the x_1 axis, x_2 is edgeLengths[1] away on the x_2 axis, etc.
   
    functionCalls is reset to 0.
   
    @return void  */
   virtual void ChooseRightSimplex();

   /** 
    May also pass cin as input stream if desired.
    Input the values of each trial point
    NOTE: THIS FUNCTION WILL ONLY ACCEPT n+1 POINTS
   
    functionCalls is reset to 0 and ALL FUNCTION VALUES ARE CALCULATED.
    @param fp reference to an input stream
    @return void  */
   void ReadInFile(istream& fp); // Query functions


   /** Performs a deep copy of the simplexAges array to a double pointer
    points to a newly allocated chunk of memory upon return
    USER SHOULD PASS JUST A NULL POINTER, WITHOUT PREALLOCATED MEMORY
      @param simAges reference to a pointer to an array of doubles,
        should be NULL going in.
      @return void  */
   void GetCurrentSimplexAges(long* &simAges) const;


   /** Prints out the simplex points by row, their corresponding f(x)
       values, and the number of function calls thus far.
       @return void  */
   void PrintDesign() const;

   //@} //other public methods

   protected :

   /** @name  methods */
   //@{

   /** This method does the actual work of the search.  It uses
       the Spendley, Hext and Himsworth algorithm  to find function
       minimum.
       @return void  */
   virtual void ExploratoryMoves();


   /** Returns true if the stopping criteria have been satisfied. 
       @return bool  */
   virtual bool Stop();

 // Simplex-altering functions

   /** Inititializes a regular simplex,  following an algorithm
       given by Jacoby, Kowalik, and Pizzo in
       "Iterative Methods for Nonlinear Optimization 
       Problems," Prentice-Hall (1972).  This algorithm also appears in 
       Spendley, Hext, and Himsworth, "Sequential Application of Simplex 
       Designs in Optimisation and Evolutionary Operation," Technometrics, 
       Vol. 4, No. 4, November 1962, pages 441--461.
       Note that Input matrix plex must be of the correct dimensions.

       @param plex pointer to a Matrix of doubles, of appropriate size.
       @return void  */
   virtual void InitRegSimplex();


   /** Inititializes a right simplex. Input matrix plex must be of
       the correct dimensions.
       @param plex pointer to a Matrix of doubles, of appropriate size.
       @return void  */
   virtual void InitRightSimplex();

  /** Deletes any existing design matrix and replaces it with one pointed
      to by plex.  functionCalls is set to 0 and ALL FUNCTION VALUES
      ARE CALCULATED.
       @param plex pointer to a Matrix of doubles, from which
         design will be initialized.
       @return void  */
   virtual void InitGeneralSimplex(Matrix<double> *plex);

   /**  Sets minIndex to the simplex index of the point which generates
	the lowest value of f(x)
	sets replacementIndex to the simplex index of the point which 
	generates the highest value of f(x) excluding the point at
	replacementSkipIndex 
	if replacementSkipIndex is set to a valid simplex index, the
	replacement search will skip over that index during its search
	this is used to prevent the simplex from getting stuck
	in a "back and forth" infinite loop.
	When the min replacement index is found, minPoint is reset as well, 
	to the appropriate values.
	@param replacementSkipIndex the index to skip over
	@return void  */
   void FindMinReplacementIndices(long replacementSkipIndex);


   /** Finds the reflection point and sets its f(x) value.
       @return void */
   void FindReflectionPt();


   /**  Determines when to shrink the simplex based on
      how old the individual simplex points are
      returns 1 if true and a shrink should occur, 0 otherwise.
      @return int  */
   int AgesTooOld();

   /** Increments the ages of all simplex points EXCEPT the point
       with index newIndex, which gets an age of 1.
       @param newIndex the index of the new point, whose age will
         not be incremented.
       @return void */
   void UpdateAges(long newIndex);

   
   /** Resets all simplex point ages to 1. 
       @return void */
   void ResetAges();
   //@} //private methods

   /** @name protected fields */
   //@{

   /** Will point to an array of longs that corresponds to the
       simplex ages. */
   long *simplexAges;  

   /** The reflection point of the simplex */
   Vector<double> *reflectionPt; 

   /** The value of f(reflectionPt) */
   double reflectionPtValue;   

   //@} //private fields
};


#endif



