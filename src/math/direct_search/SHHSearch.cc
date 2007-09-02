/*SHHSearch.cc
 *class implementation of Spendley, Hext and Himsworth Simplex Search
 *Adam Gurson College of William & Mary 1999
 *
 * Modified by Anne Shepherd, W&M, 8/2000
 */

#include <math/direct_search/SHHSearch.h>

// constructors & destructors

SHHSearch::SHHSearch(long dim, Vector<double> &startPoint )
  : SimplexSearch(dim, startPoint)
{
  simplexAges = NULL;
  new_Vector(reflectionPt, dimension);
  IDnumber = 3100;

} // SHHSearch() (default)


SHHSearch::SHHSearch(long dim, Vector<double> &startPoint, double sig)
  : SimplexSearch(dim, startPoint, sig)
{
  simplexAges = NULL;
  new_Vector(reflectionPt, dimension);
  IDnumber = 3100;
} // SHHSearch() (special)

SHHSearch::SHHSearch(long dim, Vector<double> &startPoint,
		     double sig, Vector<double> &lengths )
  : SimplexSearch(dim, startPoint, sig, lengths)
{
  simplexAges = NULL;
  new_Vector(reflectionPt, dimension);
  IDnumber = 3100;
} // SHHSearch() (special with edgelengths)


SHHSearch::SHHSearch(const SHHSearch& Original) : SimplexSearch(Original)
{
  Original.GetCurrentSimplexAges(simplexAges);
  new_Vector_Init(reflectionPt, *(Original.reflectionPt));
  reflectionPtValue = Original.reflectionPtValue;
  IDnumber = Original.IDnumber;
} // SHHSearch() (copy constructor)

SHHSearch::SHHSearch(long dim, 
		     Vector<double> &startPoint,
		     double startStep,
		     double stopStep,
		     void (*objective)(long vars, Vector<double> &x, 
				       double & func, bool& flag,
                                       void* an_obj),
		     void * input_obj)
  : SimplexSearch(dim, startPoint, startStep, stopStep, objective, input_obj)
{
  simplexAges = NULL;
  new_Vector(reflectionPt, dimension);
  IDnumber = 3100;

} // special constructor using void function and object pointers

SHHSearch::~SHHSearch()
{
  delete reflectionPt;
  if(simplexAges != NULL){
    delete[] simplexAges;
  }
   //NOTE: Matrix and Vector classes have their own destructors
} // ~SHHSearch

SHHSearch& SHHSearch::operator=(const SHHSearch& A)
{
  SimplexSearch::CopySearch(A);
  return(*this);
}
void SHHSearch::BeginSearch()
{
  if(SimplexSpecified == false) {
    ChooseRightSimplex();
  } //if
  ExploratoryMoves();
} //BeginSHHSearch()

void SHHSearch::ChooseRegularSimplex()
{ 
  InitRegSimplex();
}
  

void SHHSearch::ChooseRightSimplex()
{ 
  InitRightSimplex();
}
  

void SHHSearch::ReadInFile(istream& fp)
{
   if(fp == NULL) {
      cerr<<"No Input Stream in ReadSimplexFile()!\n";
      return; // There's no file handle!!
   }

   Matrix<double> *plex = NULL;
   new_Matrix(plex, dimension+1, dimension);
   for( long i = 0; i <= dimension; i++ ) {
      for ( long j = 0; j < dimension; j++ ) {
         fp >> (*plex)(i,j);
      } // inner for
   } // outer for
   InitGeneralSimplex(plex);
   delete plex;
   SimplexSpecified = true;
} // ReadSimplexFile()

// Query functions


void SHHSearch::GetCurrentSimplexAges(long* &simAges) const
{
  if(simplexAges  != NULL) {
    new_array(simAges, dimension + 1);
    for( long i = 0; i <= dimension; i++ ) {
      simAges[i] = simplexAges[i];
    } // for
  } //if
} // GetCurrentSimplexAges()

// private functions

void SHHSearch::ExploratoryMoves()
{
  const long DEFAULT_FLAG = -1;
  toleranceHit = 0;
  
  //Get rid of these "magic numbers." (the -1's)
  SetReplacementIndex(DEFAULT_FLAG);
  do {
    FindMinReplacementIndices(GetReplacementIndex());
    if(DEBUG) PrintDesign();
    
     // If any point has been here for a significantly long
     // time, the simplex is most likely circling a local
     // minimum, so shrink the simplex. For further information
     // regarding the rationale behind this heuristic, see
     // Gurson, "Simplex Search Behavior in Nonlinear Optimization"
     //   See design documentation bibliography.
    if( AgesTooOld() ) {
      ShrinkSimplex();
      ResetAges();
      FindMinReplacementIndices(DEFAULT_FLAG);
      if(DEBUG) PrintDesign();

       // stop if at maximum function calls
       if ((maxCalls != NO_MAX) && (functionCalls >= maxCalls)) {
          FindMinReplacementIndices(DEFAULT_FLAG);
          break;
            // return;
      } //inner if
    } // if
    if(BreakOnExact()) break;
    FindCentroid();
    FindReflectionPt();
    ReplaceSimplexPoint(replacementIndex, *reflectionPt);
    simplexValues[replacementIndex] = reflectionPtValue;
    UpdateAges(replacementIndex);
  } while (!Stop());   // while stopping criteria is not satisfied
  FindMinReplacementIndices(DEFAULT_FLAG); 
} // ExploratoryMoves()


bool SHHSearch::Stop()
    /* I changed the stopping criteria to accommodate the choice between
       exact and inexact function-call counting.  --pls 8/7/00
    */
{   
    bool stopBool= false;

    if (Stop_on_std) {
        double mean = 0.0;
        
        for( long i = 0; i <= dimension; i++) {
            if( i != minIndex ) {
                mean += simplexValues[i];
            } // if
        } //for 

        /* I'm not doing a try block for the divide-by-0 here because I
         * check for dimension == 0 in every constructor, and  the only
         * way to set the dimension is in a constructor. */
        mean /= static_cast<double>(dimension);

        // Test for the suggested Nelder-Mead stopping criteria
        double total = 0.0;
        double holder = 0.0;
        for( long i = 0; i <= dimension; i++ ) {
            holder = (simplexValues[i] - mean);
            total += (holder * holder);
        } //for
        total /= ((static_cast<double>(dimension)) + 1.0);
        total = sqrt(total);

        if(total < stoppingStepLength) {
            toleranceHit = 1;
            stopBool = true;
        }
    }

    else {
        stopBool =  delta < stoppingStepLength;
        toleranceHit = int(stopBool);
    }   
    // } // outer if
   return stopBool;
} // Stop()



void SHHSearch::InitRegSimplex()
{
  Matrix<double> *plex = NULL;
  new_Matrix(plex, dimension+1,dimension);
  SimplexSpecified = true;
  SimplexSearch::Initialize_Regular(plex);
  InitGeneralSimplex(plex); 
  if(plex != NULL) delete plex;
  plex = NULL; 
} // InitRegularTriangularSimplex()


void SHHSearch::InitRightSimplex()
{
  Matrix<double> *plex = NULL;
  new_Matrix(plex, dimension+1,dimension);
  SimplexSearch::Initialize_Right(plex);
  InitGeneralSimplex(plex);  
  if(plex != NULL) delete plex;
  plex = NULL;
} // InitRegularTriangularSimplex()


void SHHSearch::InitGeneralSimplex(Matrix<double> *plex)
{
   SimplexSearch::InitGeneralSimplex(plex);
   new_array(simplexAges, dimension+1);
   ResetAges();
   //Note "magic number---pls 7/00
   FindMinReplacementIndices(-1);
} // InitGeneralSimplex()


void SHHSearch::FindMinReplacementIndices(long replacementSkipIndex)
{
  //  printSimplex();
   if(simplexValues == NULL) {
      cerr << "Error in FindMinReplacementIndices() - "
           << "The vector of simplexValues is NULL!!\n";
      return;
   }
   long newMinIndex = 0;
   replacementIndex = 0;
   double min = simplexValues[0];  
   double replaceVal = simplexValues[0];
   if (replacementSkipIndex == 0) {
     replacementIndex = 1;
     replaceVal = simplexValues[1];
   }
   for( long i = 1; i <= dimension; i++ ) {
      if( simplexValues[i] < min ) {
         min = simplexValues[i];
         newMinIndex = i;
      } // if
      if( (i != replacementSkipIndex) && (simplexValues[i] > replaceVal) ) {
         replaceVal = simplexValues[i];
         replacementIndex = i;
      } // if
   } // for
   if (simplexValues[newMinIndex] < simplexValues[minIndex]) {
     minIndex = newMinIndex;
     ResetAges();
   }
   Vector<double> tempVec((*design).row(minIndex));
   (*minPoint) = tempVec;
   // SetMinPoint(tempVec);
   minValue = (simplexValues[minIndex]);
} // FindMinReplacementIndices()


void SHHSearch::FindReflectionPt()
{ 
   (*reflectionPt) = 0.0;
   (*reflectionPt) = ( (*centroid) * 2.0 ) - (*design).row(replacementIndex);
   bool success;
   fcnCall(dimension, (*reflectionPt), reflectionPtValue, success, some_object);
   if(!success) {
      cerr << "Error finding f(x) for reflection point at"
           << "function call #" << functionCalls << ".\n";
   } //if{
} // FindReflectionPt()


int SHHSearch::AgesTooOld()
{
  if( simplexAges[minIndex] > (dimension+1) )
    return 1;
  else
    return 0;
} // AgesTooOld()

void SHHSearch::UpdateAges(long newIndex)
{
  for( long i = 0; i <= dimension; i++ ) {
    if( i == newIndex )
      simplexAges[i] = 1;
    else
      simplexAges[i]++;
  } // for
} // ResetAges()

void SHHSearch::ResetAges()
{
   for( long i = 0; i <= dimension; i++ ) 
      simplexAges[i] = 1;
} // ResetAges()

void SHHSearch::PrintDesign() const
{
  SimplexSearch::PrintDesign();
  for( long i = 0; i <= dimension; i++ ) {
    cout << "   Age: " << simplexAges[i] << "\n";
  } // for

  cout << "\nFCalls: " << functionCalls << "\n\n";
}
