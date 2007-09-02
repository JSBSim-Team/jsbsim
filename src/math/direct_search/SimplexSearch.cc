/*SimplexSearch.cc
 *  Abstract Base class for Simplex Searches
 *
 * 
 *  Anne Shepherd, Summer 2000, based on work by
 *   Adam Gurson, College of William & Mary, 1999
 *
 */

#include <math/direct_search/SimplexSearch.h>
#include <iostream>
#include <iomanip>

// constructors & destructors


SimplexSearch::SimplexSearch(long dim, Vector<double> &startPoint )
  : DirectSearch(dim, startPoint)
{
  sigma = 0.5;
  simplexValues = NULL;
  SimplexSpecified = false;
  new_Vector(centroid, dim);
  new_Vector(scratch, dim);
  new_Vector(scratch2, dim);
  Vector<double> tempVec(dim, def_Length );
  new_Vector_Init(starting_edgeLengths, tempVec);
  minIndex = 0;
  toleranceHit = 0;
  IDnumber = 3000;
  Stop_on_std = false;
  delta = -1;
} // SimplexSearch() (default)


SimplexSearch::SimplexSearch(long dim, Vector<double> &startPoint, 
                             double sig, const Vector<double> &lengths)
  : DirectSearch(dim, startPoint)
{
  sigma = sig;
  simplexValues = NULL;
  SimplexSpecified = false;
  new_Vector(centroid, dim);
  new_Vector(scratch, dim);
  new_Vector(scratch2, dim);
  new_Vector_Init(starting_edgeLengths, lengths);
  minIndex = 0;
  toleranceHit = 0;
  IDnumber = 3000;
  Stop_on_std = false;
  delta = -1;
} //SimplexSearch(special)

SimplexSearch::SimplexSearch(long dim, Vector<double> &startPoint, double sig)
  : DirectSearch(dim, startPoint)
{
  minIndex = 0;
  sigma = sig;
  simplexValues = NULL;
  SimplexSpecified = false;
  new_Vector(centroid, dim);
  new_Vector(scratch, dim);
  new_Vector(scratch2, dim);
  Vector<double> tempVec(dim,  def_Length);
  new_Vector_Init(starting_edgeLengths, tempVec);
  toleranceHit = 0;
  IDnumber = 3000;
  Stop_on_std = false;
  delta = -1;
} // SimplexSearch() (special)

SimplexSearch::SimplexSearch(const SimplexSearch& Original)
  : DirectSearch(Original)
{
   simplexValues = NULL;
   Original.GetCurrentSimplexValues(simplexValues);
   minIndex = Original.minIndex;
   sigma = Original.sigma;
   //+++check this flag!!!+++   ---pls 7/12/00
   SimplexSpecified = Original.SimplexSpecified;
   //SimplexSpecified = false;
   replacementIndex = Original.replacementIndex;
   centroid = NULL;
   new_Vector_Init(centroid, *(Original.centroid));
   starting_edgeLengths = NULL;
   new_Vector(scratch, dimension);
   new_Vector(scratch2, dimension);
   new_Vector_Init(starting_edgeLengths, *(Original.starting_edgeLengths));
   toleranceHit = Original.toleranceHit;
   IDnumber = Original.IDnumber;
   Stop_on_std = Original.Stop_on_std;
   delta = Original.delta;
} // SimplexSearch() (copy constructor)


  /**special constructor using void function and object pointers.  The
   * objective function can be sent in as the fourth parameter here;
   * the last parameter is used for any other information that may be
   * necessary;
   * it is used in MAPS, for example, to send in an object from an
   * outside
   * class. Usually, though, the final parameter will simply be 
   *  set to NULL.
   */
  SimplexSearch::SimplexSearch(long dim, 
                               Vector<double> &startPoint,
                               //note that startStep here is the edge
                               //lengths of a regular right simplex
                               double startStep,
                               double stopStep,
                               void (*objective)(long vars, Vector<double> &x, 
                                                 double & func, bool& flag,
                                                 void* an_obj),
                               void * input_obj)
   : DirectSearch(dim, startPoint, stopStep, objective, input_obj)
{
  sigma = 0.5;
  simplexValues = NULL;
  new_Vector(centroid, dim);
  new_Vector(scratch, dim);
  new_Vector(scratch2, dim);  
  Vector<double> tempVec(dim, startStep);
  new_Vector_Init(starting_edgeLengths, tempVec);
  minIndex = 0;
  SimplexSpecified = false;
  IDnumber = 3000;
  delta = -1;
  Stop_on_std = false;
} //special constructor for MAPS


SimplexSearch::~SimplexSearch()
{
   if(simplexValues != NULL) delete [] simplexValues;
   if(centroid != NULL) delete centroid;
   if(scratch != NULL) delete scratch;
   if(scratch2 != NULL) delete scratch2;
   if(starting_edgeLengths != NULL) delete starting_edgeLengths;
   //NOTE: Matrix and Vector classes have their own destructors
} // ~SimplexSearch


SimplexSearch& SimplexSearch::operator=(const SimplexSearch& A)
{
  CopySearch(A);
  return(*this);
}

void SimplexSearch::ReplaceSimplexPoint(long index,
                                        const Vector<double>& newPoint)
{
   for( long i = 0; i < dimension; i++ ) {
      (*design)[index][i] = newPoint[i];
   } // for
} // ReplaceSimplexPoint()

void SimplexSearch::CalculateFunctionValue(long index)
{
   *scratch = (*design).row(index);
   bool success;
   fcnCall(dimension, (*scratch), simplexValues[index], success, some_object);
   if(!success) cerr<<"Error calculating point at index "
                    << index << "in CalculateFunctionValue().\n";
} // CalculateFunctionValue()


void SimplexSearch::SetSigma(double newSigma)
{
   sigma = newSigma;
} // SetSigma()


double SimplexSearch::GetSigma()
{
    return sigma;
}

bool SimplexSearch::Stop()  
{
    bool stopBool = false;
    stopBool = (maxCalls != NO_MAX && functionCalls >= maxCalls);
    return stopBool;
} //Stop

// Simplex-altering functions 
// Query functions

//Note that simValues should point to NULL!!!
void SimplexSearch::GetCurrentSimplexValues(double* &simValues) const
{
  if(simplexValues != NULL) {
    new_array(simValues,(dimension+1));
    for( long i = 0; i <= dimension; i++ ) {
      simValues[i] = simplexValues[i];
    } // for
  } //if
} // GetCurrentSimplexValues()


void SimplexSearch::SetReplacementIndex(long newIndex){
  replacementIndex = newIndex;
}

long SimplexSearch::GetReplacementIndex() const 
{
  return replacementIndex;
} // GetReplacementIndex()

void SimplexSearch::SetStartingEdgeLengths(const Vector<double> & lengths)
{ 
  (*starting_edgeLengths) = lengths;
} //SetEdgeLengths

void SimplexSearch::GetStartingEdgeLengths(Vector<double> & lengths)
{ 
  lengths = (*starting_edgeLengths);
} //GetEdgeLengths

int SimplexSearch::GetTolHit() const
{
   return toleranceHit;
} // GetTolHit()

long SimplexSearch::GetMinIndex() const
{
  return minIndex;
} //GetMinIndex

void SimplexSearch::GetCentroid(Vector<double>* &troid) 
{
  if(troid != centroid) {
    if (troid != NULL) delete troid;
    new_Vector_Init(troid, (*centroid) );
  }
} // GetCentroid()
 
double SimplexSearch::GetDelta()
{
    return delta;
}

bool SimplexSearch::Is_Stop_on_std() const
{
    return Stop_on_std;
}

void SimplexSearch::Set_Stop_on_std()
{
    Stop_on_std = true;
}
   
void SimplexSearch::Set_Stop_on_delta()
{
    Stop_on_std = false;
}

// private functions


void SimplexSearch::CopySearch(const SimplexSearch & Original)
{
   DirectSearch::CopySearch(Original);
   if(simplexValues != NULL) {
     delete [] simplexValues;
     simplexValues = NULL;
   } //if
   Original.GetCurrentSimplexValues(simplexValues);
   minIndex = Original.minIndex;
   sigma = Original.sigma;
   SimplexSpecified = Original.SimplexSpecified;
   if(starting_edgeLengths != NULL) {
     delete starting_edgeLengths;
     starting_edgeLengths = NULL;
   } //if   
   new_Vector_Init(starting_edgeLengths, *(Original.starting_edgeLengths));
   replacementIndex = Original.replacementIndex;
   if(centroid != NULL) {
     delete centroid;
     centroid = NULL;
   } //if
   new_Vector_Init(centroid, *(Original.centroid));
}//deep copy  

void SimplexSearch::Initialize_Regular(Matrix<double> *plex)
{
    /*  This routine constructs a regular simplex (i.e., one in which all of 
     *  the edges are of equal length) following an algorithm given by Jacoby,
     *  Kowalik, and Pizzo in "Iterative Methods for Nonlinear Optimization 
     *  Problems," Prentice-Hall (1972).  This algorithm also appears in 
     *  Spendley, Hext, and Himsworth, "Sequential Application of Simplex 
     *  Designs in Optimisation and Evolutionary Operation," Technometrics, 
     *  Vol. 4, No. 4, November 1962, pages 441--461.
     */

   long i,j;
   double p, q, temp;
   double dim = static_cast<double>(dimension);
   double simplexEdge = (*starting_edgeLengths)[0];
   const double ROOT_2 = sqrt(2.0);
   for( long col = 0; col < dimension; col++ ) {
     // (*plex)[0][col] = (*minPoint)[col];
     (*plex)(0,col) = (*minPoint)[col];
   }

   temp = dim + 1.0;
   q = ((sqrt(temp) - 1.0) / (dim * ROOT_2)) * simplexEdge;
   p = q + ((1.0 / ROOT_2) * simplexEdge);

   for(i = 1; i <= dimension; i++) { 
      for(j = 0; j <= i-2; j++) {
	//(*plex)[i][j] = (*plex)[0][j] + q;
	(*plex)(i,j) = (*plex)(0,j) + q;
      } // inner for 1
      j = i - 1;
      // (*plex)[i][j] = (*plex)[0][j] + p;
      (*plex)(i,j) = (*plex)(0,j) + p;
      for(j = i; j < dimension; j++) {
	// (*plex)[i][j] = (*plex)[0][j] + q;
	(*plex)(i,j) = (*plex)(0,j) + q;
      } // inner for 2
   } // outer for
} // InitRegularTriangularSimplex()

void SimplexSearch::Initialize_Right(Matrix<double> *plex)
{
   for( long i = 0; i < dimension; i++ ) {
      // we're building the minPoint component-by-component into
      // the (n+1)st row
      //(*plex)[dimension][i] = (*minPoint)[i];
      (*plex)(dimension,i) = (*minPoint)[i];

      // now fill in the ith row with the proper point
      for( long j = 0; j < dimension; j++ ) {
        // (*plex)[i][j] = (*minPoint)[j];
         (*plex)(i,j) = (*minPoint)[j];
         if( i == j )
	   //(*plex)[i][j] += (*starting_edgeLengths)[i];
            (*plex)(i,j) += (*starting_edgeLengths)[i];
      }
   } // for
} // InitRightSimplex()

void SimplexSearch::InitGeneralSimplex(const Matrix<double> *plex)
{
   functionCalls = 0;
   delta = -1;
   if( design != NULL ) { delete design; }
   if( simplexValues != NULL ) { delete [] simplexValues;}
   new_Matrix_Init(design, (*plex));
   new_array(simplexValues, (dimension + 1)); 

   bool success;
   for( long i = 0; i <= dimension; i++ ) {
      *scratch = (*plex).row(i);
      fcnCall(dimension, (*scratch), simplexValues[i], success, some_object);
      if(!success) cerr<<"Error with point #"<<i<<" in initial simplex.\n";
   } // for
   (*minPoint) = (*scratch);
   minValue = simplexValues[dimension];
   minIndex = dimension;

   /* Now go through the simplex and
   define delta to be the length of the LONGEST simplex side.
   */
   double temp;
     for( long j = 0; j < dimension; j++ ) {
       for ( long k = j+1; k <= dimension; k++ ) {
         temp = ( ((*design).row(j)) - ((*design).row(k)) ).l2norm();
         if( temp > delta ) delta = temp;
       } // inner for
     } //for
//SimplexSpecified = true;
} // InitGeneralSimplex()


void SimplexSearch::FindCentroid()
{
   (*centroid) = 0.0;
   for( long i = 0; i <= dimension; i++ ) {
      if( i != replacementIndex ) {
         (*centroid) = (*centroid) + (*design).row(i);
      } // if
   } // for
   (*centroid) = (*centroid) * ( 1.0 / static_cast<double>(dimension) );
} // FindCentroid()


void SimplexSearch::ShrinkSimplex()
{
   // stop if at maximum function calls
   if (maxCalls != NO_MAX
       && functionCalls >= maxCalls) {return;}
   
   delta *= sigma;
   Vector<double> *lowestPt = scratch;
   *lowestPt = (*design).row(minIndex);
   Vector<double> *tempPt = scratch2;
   bool success;
   for( long i = 0; i <= dimension; i++ ) {
      if( i != minIndex ) {
         *tempPt = (*design).row(i);
         (*tempPt) = (*tempPt) + ( sigma * ( (*lowestPt)-(*tempPt) ) );
         for( long j = 0; j < dimension; j++ ) {
            (*design)[i][j] = (*tempPt)[j];
         } // inner for
         fcnCall(dimension,(*tempPt),simplexValues[i],success, some_object);
         if (!success) cerr << "Error shrinking the simplex.\n";
         
         // stop if at maximum function calls
         if (maxCalls != NO_MAX
             && functionCalls >= maxCalls) {return;}

      } // if
   } // outer for
} // ShrinkSimplex

void SimplexSearch::PrintDesign() const
{
  for( long i = 0; i <= dimension; i++ ) {
     cout << "\nPoint: ";
     for ( long j = 0; j < dimension; j++ ) {
       cout << (*design)[i][j] << " ";
     } // inner for
     cout << "\nValue: " << simplexValues[i];
  } // outer for
  cout << "\nFCalls: " << functionCalls << "\n\n";
} //printSimplex

#if defined(AGO_DIRECTSEARCH)

void SimplexSearch::PrintfMin() const
{
   if ( ofile ) {
     *(ofile) << functionCalls << ", " << minValue << ", " << delta;
     for ( long i = 0; i < dimension; i++ ) {
       *(ofile) << ", "<< (*minPoint)[i];
     }
     *(ofile) << "\n";
   }
}

#endif





