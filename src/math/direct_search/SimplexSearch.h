/*SimplexSearch.h
  Abstract Base Class for Simplex Searches.  

  Adam Gurson, P.L. Shepehrd
  College of William and Mary
 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

 */

#ifndef _SimplexSearch_
#define _SimplexSearch_

#include <iostream>
#include <fstream>
#if defined(sgi) && !defined(__GNUC__)
# include <math.h>
# include <stdlib.h>
#else
# include <cmath>
# include <cstdlib>
#endif
#include <math/direct_search/objective.h>
#include <math/direct_search/vec.h>
#include <math/direct_search/cppmat.h>
#include <math/direct_search/DirectSearch.h>


#ifndef DEBUG
#define DEBUG 0
#endif

/**Abstract Base Class for the simplex searches.  
   @author Adam Gurson and P.L. Shepherd, College of William and Mary */
class SimplexSearch : public DirectSearch
{ 
 public:
  /**Default edgelength for the constructors  */
   static const int def_Length = 2;

   /** @name Constructors and destructor */
   //@{


  /**Constructor.  This constructor has two parameters.
     Other data members will be set to default values,
     as follows. This is of course in addition to assignments and 
     initializations made in DirectSearch class.
    \begin{itemize}
    \item[] sigma = 0.5
    \item[] simplexValues = NULL
    \item[] minIndex = NULL
    \item[] SimplexSpecified = false
    \item[] memory is allocated for fields centroid, scratch, and scratch2
    \item[] starting_edgeLengths is initialized as a Vector of
        length dimension, all of whose entries are equal to the
        default length of 2.0.
    \item[] IDnumber = 3000
    \end{itemize}

     @param dim the dimension of the problem
     @param startpoint the start point for the search. This will be initialized 
         as the minPoint.
  */ 
   SimplexSearch(long dim, Vector<double> &startPoint);

    /** Copy constructor
    @param Original a reference to the object from which to initialize */
   SimplexSearch(const SimplexSearch& Original);

   /**  Constructor which allows shrinking coefficient initialization. 
    sigma will be set to the value of sig.  Other fields will be assigned
    as in SimplexSearch(long dim, Vector<double> &startPoint).

    @param dim the dimension of the problem.
    @param startpoint the start point for the search.
           This will be initialized as the minPoint.
    @param sig the user-defined value for sigma, the
           shrinking coefficient. */
   SimplexSearch(long dim, Vector<double> &startPoint, double sig);


   /**  Constructor which allows shrinking coefficient
    initialization and specification of starting_edgeLengths. 
    sigma will be set to the value of sig.  
    starting_edgeLengths will be initialized from lengths.
        Other fields will be assigned
    as in SimplexSearch(long dim, Vector<double> &startPoint).

    @param dim the dimension of the problem.
    @param startpoint the start point for the search. This will
          be initialized as the minPoint.
    @param sig the user-defined value for sigma, the shrinking
          coefficient.
        @param lengths reference to a Vector of doubles
          representing the desired edgelengths of the simplex.  */
   SimplexSearch(long dim, Vector<double> &startPoint, 
         double sig, const Vector<double> &lengths);



  /**Special constructor using void function and object pointers.
     The objective function can be sent in as the fourth
     parameter here; the last parameter is used for any other
     information that may be necessary; it is used in MAPS, for
     example, to send in an object from an outside class.
     Usually, though, the final parameter will simply be 
     set to NULL.
     
  This constructor takes five input parameters.
  Other fields are set by default as in
  SimplexSearch(long dim, Vector<double> &startPoint).

  @param dim the dimension of the problem
  @param startPoint a Vector of doubles, the starting point for the search
  @param startStep will be used as the edge length of a fixed-length
     right simplex.
  @param stopStep the stopping step length for the search
  @param objective a pointer to the function to be minimized
  @param input_obj used to send additional data as needed--will
     normally be set to NULL.
  */ 
   SimplexSearch(long dim, 
         Vector<double> &startPoint,
         double startStep,
         double stopStep,
         void (*objective)(long vars, Vector<double> &x, 
                           double & func, bool& flag,
                           void* an_obj),
         void * input_obj);

   virtual ~SimplexSearch();
   // destructor

   //@} //constructors and destructor

   /** @name Other public methods */
   //@{  //other public methods

   /**Overloaded assignment operator
      @param A the search to be copied
      @return SimplexSearch&  */
   virtual SimplexSearch& operator= (const SimplexSearch& A);

   /**This method begins the actual searching in the concrete classes.  It is
      pure virtual in this abstract base class. 
      @return void */
   virtual void BeginSearch() = 0;

   /** Allows the user to choose a regular simplex for the search. Pure
       virtual in this abstract base class.
       @return void */   
   virtual void ChooseRegularSimplex()= 0;


  /** Virtual void function, unimplemented in this absstract base class.
    In concrete classes, will delete any existing simplex and replaces it 
    with a right-angle simplex in the following manner:
   
    minPoint points to a point that will be the "origin" or base point of the
    simplex points (it will be a part of the simplex)
    starting_edgeLengths points to a vector of n doubles, where n is the
    dimension of the given search. x_1 will then be located
    a distance of starting_edgeLengths[0] away from the base point along the
    the x_1 axis, x_2 is edgeLengths[1] away on the x_2 axis, etc.
   
    functionCalls is reset to 0.
   
    @return virtual void  */
  virtual void ChooseRightSimplex() = 0;


  /** Pure virtual in this abstract base class.  
    May also pass cin as input stream if desired.
    Input the values of each trial point
    NOTE: THIS FUNCTION WILL ONLY ACCEPT n+1 POINTS
   
    functionCalls is reset to 0.
    @param fp reference to an input stream
    @return virtual void
  */
  virtual void ReadInFile(istream& fp) = 0;

   /** Prints out the simplex points by row, their corresponding f(x)
       values, and the number of function calls thus far.
       @return void */
   virtual void PrintDesign() const;

#if defined(AGO_DIRECTSEARCH)

   virtual void PrintfMin() const;

#endif

   //@} //other public methods

  /** @name Accessor and mutator methods */
  //@{

   /** Returns replacementIndex.
       @return long  */
   long GetReplacementIndex() const;

   /** Sets starting_edgeLengths to equal lengths.  If you wish to use a
       regular dimplex or a fixed-length right simplex, all entries of 
       the input Vector lengths should be the same.  If you wish to search
       using a variable-length right simplex, then the entries of the input
       parameter lengths should be the desired edgelengths of the 
       right simplex.
       @param lengths a reference to a Vector of doubles representing the
         edgelengths of the simplex.
       @return void  */
       
   void SetStartingEdgeLengths(const Vector<double> & lengths);

   /** Assigns the input Vector lengths the values of the member field 
       starting_edgeLengths.  
       NOTE:  the input Vector lengths must not be NULL and 
       should be of length dimension.  
 
       @param lengths reference to a Vector of doubles, of length dimension.
       @return void  */
   void GetStartingEdgeLengths(Vector<double> & lengths);


   /** Allows the user to set a new value for
       the shrinking coefficient.
       @param newSigma new value for Sigma (the shrinking coefficient)
       @return void  */
   void SetSigma(double newSigma);

    /** Returns the value of sigma.        
        @return double  */
     
   double GetSigma();


   /** performs a deep copy of the simplexValues array to a double pointer
    points to a newly allocated chunk of memory upon return
    USER SHOULD PASS JUST A NULL POINTER, WITHOUT PREALLOCATED MEMORY
    @param simValues reference to a NULL double pointer going in,
      will be initialized to simplexValues on return.
    @return void */
   void GetCurrentSimplexValues(double* &simValues) const;

   /** Returns toleranceHit, which indicates whether the simplex
       size has refined down to the value of stoppingStepLength. 
       @return int */
   int GetTolHit() const;

   /** Returns minIndex.
        @return long  */
   long GetMinIndex() const;

   
   /** Upon return the input Vector* troid will point to a newly allocated
       Vector of doubles representing the centroid of the simplex.
       User should pass just a null pointer.
       @param troid a pointer to a Vector of doubles: it should be
         NULL going in.
       @return void */
   void GetCentroid(Vector<double>* &troid);

   /** Returns delta, the length of the longest simplex edge.
       @return double
   */
   double GetDelta();

   /** Returns value of the Stop_on_std field
       @return bool
    */
   bool Is_Stop_on_std() const;

   /** Allows user to specify that the standard Nelder-Mead
       stopping criterion will be used.  We recommend the user
       keep the defaults we have set for all searches.  There is
       a chance that an SHH or NM search can hang up on a
       point that is not a minimum when using this criterion.

       @return void
   */
   void Set_Stop_on_std();

   /** Allows users to specify that the search will terminated
       based on delta. (This is the default for most searches)
       
       @return void
   */
   void Set_Stop_on_delta();

   //@} //accessors

protected :

     /** @name methods */
     //@{
   /** Pure virtual in this abstract base class. Does the actual work of the
       search algorithm in the concrete classes.
       @return void  */
   virtual void ExploratoryMoves() = 0;
 
   /**Finds the f(x) value for the simplex point indexed at index and
      replaces the proper value in simplexValues.
      @param index the row of the design matrix holding the  point
         to be evaluated
      @return void  */
   virtual void CalculateFunctionValue(long index);

   /** returns true if the stopping criteria have been satisfied
      @return bool */
   virtual bool Stop();

   /** Used to implement the overloaded assignment operator.
       @param Original the search to be copied
       @return void   */
   virtual void CopySearch(const SimplexSearch & Original);

   /** Inititializes a right simplex. Input matrix plex must be of
       the correct dimensions.
       @param plex pointer to a Matrix of doubles, of appropriate size.
       @return void  */
   virtual void Initialize_Right(Matrix<double> *plex);

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
   virtual void Initialize_Regular(Matrix<double> *plex);

   /** Deletes any existing design matrix and replaces it with one pointed to
       by plex.  functionCalls is set to 0.
       @param plex pointer to a Matrix of doubles, from which design
         will be initialized.
       @return void  */
   virtual void InitGeneralSimplex(const Matrix<double> *plex);

  
   /** Sets replacementIndex to the value of newIndex. 
      @param newIndex the desired value of replacementIndex
      @return void  */
   virtual void SetReplacementIndex(long newIndex);
 
   /** Finds the centroid and assigns the value to the member field centroid.
       @return void  */
   virtual void FindCentroid();

   /**  Goes through the simplex and reduces the
    lengths of the edges adjacent to the best vertex
    @return void */
   virtual void ShrinkSimplex();

   /** Replaces simplex point indexed at index with parameter newPoint.
       @param index index (in the design matrix) of the point to be replaced
       @param newPoint the point with which to replace the one at index. */
   virtual void ReplaceSimplexPoint(long index, const Vector<double>& newPoint);

   //@} //protected methods
   /** @name Protected fields */
   //@{

   /** f(x) values of the points corresponding to the rows of the
        design matrix */
   double *simplexValues;        
   
   /** The edgelengths of the simplex that will be represented by
       the design matrix. If you wish to use either the
       default--a fixed-length right simplex-- or
       a regular simplex, all entries of this vector should be equal.
       If you plan to choose a variable length right
       simplex, then the entries of the vector
       should be the desired edgelengths of the simplex. */
   Vector<double> *starting_edgeLengths;    
  
   /** The shrinking coefficient */  
   double sigma;   

   /** Index of point generating min f(x) */
   long minIndex;
 
   /** Index of point to be replaced */            
   long replacementIndex;     

   /** The current centroid */
   Vector<double> *centroid;  
//++++++++do it this way in DirectSearch uberclass??  
//Why not make it a bool?+++++++++++   --pls 6/6/00
   
   /** Flag to indicate the reason for stopping the search.
       Will be 1 if stopped due to tolerance, 0 if funcCalls */
   int toleranceHit;            

   /** Flag to indicate whether a specific simplex type has been
       chosen.If false, the search will be initialized
       with a right simplex. */
   bool SimplexSpecified;

   /** Flag to indicate that the user has chosen (where available) to
       use the standard-deviation (i.e. Nelder-Mead type) stopping
       criterion.  Set by defaultto false.
   */
   bool Stop_on_std;

   /** Named to recall the "delta" of the pattern searches,
       this is generally the length of (at the start) the longest
       edge of the simplex; it is used for the stopping test
       where Stop_on_std is false (as it is by default).
   */
   double delta;

   /** The two scratch vectors are simply extra storage space
    to be used by methods that require a vector of
    size = dimensions */
   Vector<double> *scratch, *scratch2;
   //@} //Protected fields
};//SimplexSearch


#endif


