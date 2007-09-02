
/*NMSearch.cc
 * class implementation of Nelder Mead Simplex Search
 * Adam Gurson College of William & Mary 1999
 * modified by P.L. (Anne) Shepherd, 2000
 */

#include <math/direct_search/NMSearch.h>


// constructors & destructors
NMSearch::NMSearch(long dim, Vector<double> &startPoint) 
  : SimplexSearch(dim, startPoint)
{
   alpha = 1.0;
   beta = 0.5;
   gamma = 2.0;
   new_Vector(reflectionPt, dim);
   new_Vector(expansionPt, dim);
   new_Vector(contractionPt, dim);
   exact_count = true;
   Stop_on_std = true;
   NMdelta = delta;
   IDnumber = 3200;
} // NMSearch() (default)

NMSearch::NMSearch(long dim, Vector<double> &startPoint, 
		   double sig, Vector<double> &lengths) 
  : SimplexSearch(dim, startPoint, sig, lengths)
{
   alpha = 1.0;
   beta = 0.5;
   gamma = 2.0;
   new_Vector(reflectionPt, dim);
   new_Vector(expansionPt, dim);
   new_Vector(contractionPt, dim);
   exact_count = true;
   Stop_on_std = true;
   IDnumber = 3200;
   NMdelta = delta;
} // NMSearch() (default)

NMSearch::NMSearch(long dim, Vector<double> &startPoint,
                   double NewSigma, double NewAlpha, 
		   double NewBeta, double NewGamma)
  : SimplexSearch(dim, startPoint, NewSigma)
{
  
   alpha = NewAlpha;
   beta = NewBeta;
   gamma = NewGamma;
   new_Vector(reflectionPt, dim);
   new_Vector(expansionPt, dim);
   new_Vector(contractionPt, dim);
   exact_count = true;
   Stop_on_std = true;
   IDnumber = 3200;
   NMdelta = delta;
} // NMSearch() (special)


  NMSearch::NMSearch(long dim, 
		     Vector<double> &startPoint,
		     double NewSigma, double NewAlpha,
		     double NewBeta, double NewGamma,
		     double startStep,
		     double stopStep,
		     void (*objective)(long vars, Vector<double> &x, 
				       double & func, bool& flag,
                                       void* an_obj),
		     void * input_obj)
   : SimplexSearch(dim, startPoint, startStep, stopStep, objective,
                   input_obj)
{
   
   sigma = NewSigma;
   alpha = NewAlpha;
   beta = NewBeta;
   gamma = NewGamma;
   new_Vector(reflectionPt, dim);
   new_Vector(expansionPt, dim);
   new_Vector(contractionPt, dim);
   exact_count = true;
   Stop_on_std = true;
   IDnumber = 3200;
   NMdelta = delta;
} //special constructor using void pointers

NMSearch::NMSearch(const NMSearch& Original) 
   : SimplexSearch(Original)
{
  alpha = Original.alpha;
  beta = Original.beta;
  gamma = Original.gamma;
  maxIndex = Original.maxIndex;
  reflectionPt = NULL;
  new_Vector_Init( reflectionPt, (*(Original.reflectionPt)));
  reflectionPtValue = Original.reflectionPtValue;
  expansionPt = NULL;
  new_Vector_Init(expansionPt, (*(Original.expansionPt)));
  expansionPtValue = Original.expansionPtValue;
  contractionPt = NULL;
  new_Vector_Init( contractionPt, *(Original.contractionPt));
  NMdelta = Original.NMdelta;
} // NMSearch() (copy constructor)

NMSearch::~NMSearch()
{
  if(reflectionPt != NULL)
    delete reflectionPt;
  if(expansionPt != NULL) 
    delete expansionPt;
  if(contractionPt != NULL)
    delete contractionPt;   //NOTE: Matrix and Vector classes
                            // have their own destructors
} // ~NMSearch


NMSearch& NMSearch::operator=(const NMSearch& A)
{
  CopySearch(A);
  return(*this);
}

void NMSearch::ChooseRightSimplex()
{ 
  InitRightSimplex();
}

void NMSearch::ChooseRegularSimplex()
{
  InitRegSimplex();
} // InitRegularTriangularSimplex()

void NMSearch::BeginSearch()
{
    
  if(SimplexSpecified == false) {
    ChooseRightSimplex();
  } //if
  ExploratoryMoves();
} //BeginNMSearch


void NMSearch::SetAlpha(double newAlpha)
{
   alpha = newAlpha;
} // SetAlpha()

void NMSearch::SetBeta(double newBeta)
{
   beta = newBeta;
} // SetBeta()

void NMSearch::SetGamma(double newGamma)
{
   gamma = newGamma;
} // SetGamma()


double NMSearch::GetDelta()
{
    
    CalculateNMDelta();   
    return NMdelta;
} // NMSearch version of GetDelta


void NMSearch::ExploratoryMoves()
  //+++++There are a lot of nested ifs and if-elses here--maybe
  // we'll want to break it up later++++++++  --pls 6/12
{
   double secondHighestPtValue; // used for contraction/reflection decision
   toleranceHit = 0;

   FindMinMaxIndices();
   do {
      if(DEBUG) PrintDesign();

#if defined(AGO_DIRECTSEARCH)
     PrintfMin();
#endif

      FindCentroid();
      secondHighestPtValue = simplexValues[SecondHighestPtIndex()];
    // reflection step
      FindReflectionPt();

      // stop if at maximum function calls and update the simplex 
      if (maxCalls != NO_MAX && functionCalls >= maxCalls) {
         FindMinMaxIndices();
         ReplaceSimplexPoint(maxIndex, *reflectionPt);
         simplexValues[maxIndex] = reflectionPtValue;
         FindMinMaxIndices(); 
         return;
      }

    // possibility 1

      if(simplexValues[minIndex] > reflectionPtValue) {
	//+++++Shouldn't we call this whole thing ExpansionStep 
	//or something??? may be easier to read.  Think
	// about this later. ++++ --pls 6/12
         FindExpansionPt(); // expansion step

         if (reflectionPtValue > expansionPtValue) {
            ReplaceSimplexPoint(maxIndex, *expansionPt);
            simplexValues[maxIndex] = expansionPtValue;
         } // inner if
         else {
            ReplaceSimplexPoint(maxIndex, *reflectionPt);
            simplexValues[maxIndex] = reflectionPtValue;
         } // else         
      } // if for possibility 1

    // possibility 2

      else if( (secondHighestPtValue > reflectionPtValue        ) &&
               (   reflectionPtValue >= simplexValues[minIndex]) ) {
         ReplaceSimplexPoint(maxIndex, *reflectionPt);
         simplexValues[maxIndex] = reflectionPtValue;
      } // else if for possibility 2

    // possibility 3
      else if( reflectionPtValue >= secondHighestPtValue ) {
         FindContractionPt(); // contraction step
         if(maxPrimePtId == 0) {
           if( contractionPtValue > maxPrimePtValue ) {
             ShrinkSimplex();
             if (maxCalls != NO_MAX && functionCalls >= maxCalls) {
                 break;
             }
           } // inner if
           else {
             ReplaceSimplexPoint(maxIndex, *contractionPt);
             simplexValues[maxIndex] = contractionPtValue;
           } // inner else
         } // maxPrimePtId == 0
         else if(maxPrimePtId == 1) {
           if( contractionPtValue >= maxPrimePtValue ) {
             ShrinkSimplex();
           } // inner if
           else {
             ReplaceSimplexPoint(maxIndex, *contractionPt);
             simplexValues[maxIndex] = contractionPtValue;
           } // inner else
         } // maxPrimePtId == 1
      } // else if for possibility 3

    // if we haven't taken care of the current simplex, something's wrong
      else {
         cerr << "Error in ExploratoryMoves() - "
              << "Unaccounted for case.\nTerminating.\n";
         return;
      }
   FindMinMaxIndices();
   if(BreakOnExact()) break;
   } while (!Stop());   // while NM stopping criteria is not satisfied
} // ExploratoryMoves()

/*This is an expensive function.( O(n^3) including the underlying vector
 * operations.) Do we want to do this? 
 * Only when NM criterion is true?  --that's where it is now, 8/00 --pls
 */
void NMSearch::CalculateNMDelta()
{
  double holder = 0.0;
  double temp = 0.0;
  long counter = 0;
  
  for( long j = 0; j < dimension; j++ ) {
    for ( long k = j+1; k <= dimension; k++ ) {        
        temp = ( ((*design).row(j)) - ((*design).row(k)) ).l2norm();
      holder += temp;
      counter ++;
    } // inner for
  } // outer for

  //cout << "\ncounter = " << counter << endl;
  holder /= static_cast<double>(counter);
  NMdelta = holder;
} // FindDelta


/*QUESTION: If the simplex collapses, will this hang the program??
  ---------pls, 8/21/00
*/
bool NMSearch::Stop()
{
    bool stopBool = false;
    double mean = 0.0;

    // First, see if we're over our call budget.
    if(maxCalls != NO_MAX) {
        if(functionCalls >= maxCalls)
            stopBool = true;        
    } //if

    if(!stopBool)
    {
        for( long i = 0; i <= dimension; i++) {
            if( i != minIndex ) {
                mean += simplexValues[i];
            } // if
        } //for 

        mean /= (double)dimension;

        // Test for the suggested Nelder-Mead stopping criteria
        double total = 0.0;
        double holder = 0.0;
        for( long i = 0; i <= dimension; i++ ) {
            holder = simplexValues[i] - mean;
            total += (holder * holder);
        } //for
        total /= ((double)(dimension) + 1.0);
        total = sqrt(total);
 
        if(total < stoppingStepLength) {
            toleranceHit = 1;
            stopBool = true;
        }
        if (!Stop_on_std && stopBool) {
            CalculateNMDelta();
            stopBool =  NMdelta < stoppingStepLength;
            toleranceHit = int(stopBool);   
        }
    } // if(!stopBool) 
    return stopBool;
} // Stop()

void NMSearch::CopySearch(const NMSearch & Original)
{
   SimplexSearch::CopySearch(Original);
   alpha = Original.alpha;
   beta = Original.beta;
   gamma = Original.gamma;
   maxIndex = Original.maxIndex;
   if(reflectionPt != NULL) delete reflectionPt;
   new_Vector_Init( reflectionPt, (*(Original.reflectionPt)));
   reflectionPtValue = Original.reflectionPtValue;
   if(expansionPt != NULL) delete expansionPt;
   new_Vector_Init(expansionPt, (*(Original.expansionPt)));
   expansionPtValue = Original.expansionPtValue;
   if(contractionPt != NULL) delete contractionPt;
   new_Vector_Init( contractionPt, *(Original.contractionPt));
   contractionPtValue = Original.contractionPtValue;
}//deep copy  

// Simplex-altering functions

void NMSearch::InitRegSimplex()
{
  Matrix<double> *plex = NULL;
  new_Matrix(plex, dimension+1,dimension);
  SimplexSpecified = true;
  SimplexSearch::Initialize_Regular(plex);
  InitGeneralSimplex(plex);  
  if(plex != NULL) delete plex;
} // InitRegularTriangularSimplex()


void NMSearch::InitRightSimplex()
{
  
  Matrix<double> *plex = NULL;
  new_Matrix(plex, dimension+1,dimension);
  /*Note that we do NOT set SimplexSpecified to true here. This is
   * because if we did and the user wanted to use the same search over
   * and over in a loop, it would not reinitialize each time unless the
   * user called ChooseRightSimplex each time.  Because this is the
   * default, we decided to risk the occasional harmless
   * re-initialization rather than confuse the user. */
  SimplexSearch::Initialize_Right(plex);
  InitGeneralSimplex(plex);  
  if(plex != NULL) delete plex;
} // InitRegularTriangularSimplex()


void NMSearch::InitGeneralSimplex(const Matrix<double> *plex)
{
   SimplexSearch::InitGeneralSimplex(plex);   
   FindMinMaxIndices();
} // InitGeneralSimplex()


void NMSearch::ReadInFile(istream& fp)
{
   if(fp == NULL) {
      cerr<<"No Input Stream in ReadInFile()!\n";
      return; // There's no file handle!!
   }

   Matrix<double> *plex = NULL;
   new_Matrix(plex, dimension+1,dimension);
   for( long i = 0; i <= dimension; i++ ) {
      for ( long j = 0; j < dimension; j++ ) {
        fp >> (*plex)[i][j];
      } // inner for
   } // outer for
   InitGeneralSimplex(plex);
   SimplexSpecified = true;
   delete plex;
   plex = NULL;
 
   //cout << "exiting NMSearch::ReadInFile\n"; //agodemar
        
} // ReadInFile()

// Query functions

// private functions

void NMSearch::FindMinMaxIndices()
{
   minIndex = 0;
   maxIndex = dimension;
   double min = simplexValues[0];
   double max = simplexValues[dimension];
   for( long i = 1; i <= dimension; i++ ) {
      if( simplexValues[i] < min ) {
         min = simplexValues[i];
         minIndex = i;
      } // if
      if( simplexValues[dimension - i] > max ) {
         max = simplexValues[dimension - i];
         maxIndex = dimension - i;
      } // if
   } // for
   // Vector<double> minimum(dimension);
   (*minPoint) = ((*design).row(minIndex));
   //  SetMinPoint(minimum);
   minValue = (simplexValues[minIndex]);
   
} // FindMinMaxIndices()

long NMSearch::SecondHighestPtIndex()
{
   if(simplexValues == NULL) {
      cerr << "Error in SecondHighestPtValue() - "
           << "The vector of simplexValues is NULL!!\n";
      // ++++BEWARE---magic number!! use a flag constant instead.+++ ---pls
      return -1;
   }
   long secondMaxIndex = minIndex;
   double secondMax = simplexValues[minIndex];
   for( long i = 0; i <= dimension; i++ ) {
      if(i != maxIndex) {
         if( simplexValues[i] > secondMax ) {
            secondMax = simplexValues[i];
            secondMaxIndex = i;
         } // inner if
      } // outer if
   } // for
   return secondMaxIndex;
} // SecondHighestPtValue()

void NMSearch::FindCentroid()
{
   (*centroid) = 0.0;
   for( long i = 0; i <= dimension; i++ ) {
      if( i != maxIndex ) {
         (*centroid) = (*centroid) + (*design).row(i);
      } // if
   } // for
   (*centroid) = (*centroid) * ( 1.0 / static_cast<double>(dimension) );
} // FindCentroid()

void NMSearch::FindContractionPt()
{
   Vector<double> *maxPrimePt = scratch;
   if(simplexValues[maxIndex] <= reflectionPtValue) {
      *maxPrimePt = (*design).row(maxIndex);
      maxPrimePtValue = simplexValues[maxIndex];
      maxPrimePtId = 1;
   } // if
   else {
      maxPrimePt = reflectionPt;
      maxPrimePtValue = reflectionPtValue;
      maxPrimePtId = 0;
   } // else

   (*contractionPt) = ( (*centroid) * (1.0 - beta) ) +
                      ( beta * (*maxPrimePt) );
   bool success;
   fcnCall(dimension, (*contractionPt), contractionPtValue, success, some_object);
   if(!success) {
      cerr << "Error finding f(x) for contraction point at"
           << "function call #" << functionCalls << ".\n";
   } // if
} // FindContractionPt()


void NMSearch::FindReflectionPt()
{ 
   bool success;
   (*reflectionPt) = 0.0;
   (*reflectionPt) = ( (*centroid) * (1.0 + alpha) ) -
                     ( alpha * (*design).row(maxIndex) );
   fcnCall(dimension, (*reflectionPt), reflectionPtValue, success, some_object);
   if(!success) {
      cerr << "Error finding f(x) for reflection point at"
           << "function call #" << functionCalls << ".\n";
   } // if
} // FindReflectionPt()


void NMSearch::FindExpansionPt()
{
   (*expansionPt) = 0.0;
   (*expansionPt) = ( (*centroid) * (1.0 - gamma) ) +
                    ( gamma * (*reflectionPt) );
   bool success;
   fcnCall(dimension, (*expansionPt), expansionPtValue, success, some_object);
   if(!success) {
      cerr << "Error finding f(x) for expansion point at"
           << "function call #" << functionCalls << ".\n";
   } // if
} // FindExpansionPt()
