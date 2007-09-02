/*SMDSearch.cc
 *Declarations of Sequential version of Torczon's Multi-Directional Search
 *Adam Gurson College of William & Mary 2000
 */

#include <math/direct_search/SMDSearch.h>

// constructors & destructors

SMDSearch::SMDSearch(long dim, Vector<double> &startPoint)
  : SimplexSearch(dim, startPoint)
{
   new_array(simplexVBits, (dimension+1));
   new_Matrix(refSimplex, dimension+1,dimension);
   new_array(refSimplexValues, (dimension+1));
   new_array(refSimplexVBits, (dimension+1));
   IDnumber = 3300;
} // SMDSearch() (default)

SMDSearch::SMDSearch(long dim, Vector<double> &startPoint, double NewSigma)
  : SimplexSearch(dim, startPoint, NewSigma)
{
   new_array(simplexVBits, (dimension+1));
   new_Matrix(refSimplex, dimension+1,dimension);
   new_array(refSimplexValues, (dimension+1));
   new_array(refSimplexVBits, (dimension+1));
   IDnumber = 3300;
} // SMDSearch() (special)

SMDSearch::SMDSearch(long dim, Vector<double> &startPoint, 
		     double NewSigma, Vector<double> &lengths)
  : SimplexSearch(dim,startPoint, NewSigma, lengths)
{
   new_array(simplexVBits, (dimension+1));
   new_Matrix(refSimplex, dimension+1,dimension);
   new_array(refSimplexValues, (dimension+1));
   new_array(refSimplexVBits, (dimension+1));
   IDnumber = 3300;
} // SMDSearch() (special with starting_edgeLengths)

SMDSearch::SMDSearch(const SMDSearch& Original)
  : SimplexSearch(Original)
{
   new_array(simplexVBits, (dimension+1));
   for(int j = 0; j <= dimension; j++) {
     simplexVBits[j] = Original.simplexVBits[j];
   } //for
   new_Matrix_Init(refSimplex, *(Original.refSimplex));
   //refSimplex = Original.refSimplex;
   new_array(refSimplexValues, (dimension+1));
   for(int i = 0; i <= dimension; i++) {
     refSimplexValues[i] = Original.refSimplexValues[i];
   } //for
   new_array(refSimplexVBits, (dimension+1));
   for(int j = 0; j <= dimension; j++) {
     refSimplexVBits[j] = Original.refSimplexVBits[j];
   } //for
   delta = Original.delta;
   exact_count = Original.exact_count;
   IDnumber = Original.IDnumber;
} // SMDSearch() (copy constructor)


  /**special constructor using void function and object pointers.
     The objective function  can be sent in as the fourth
     parameter here; the last parameter is used for any other
     information that may be necessary; it is used in MAPS,
     for example, to send in an  object from an outside class.
     Usually, though, the final parameter will simply be 
     set to NULL. */
  SMDSearch::SMDSearch(long dim, 
               Vector<double> &startPoint,
               double startStep,
               double stopStep,
               void (*objective)(long vars, Vector<double> &x, 
                                 double & func, bool& flag,
                                 void* an_obj),
               void * input_obj)
   : SimplexSearch(dim, startPoint, startStep, stopStep, objective, input_obj)
{
   new_array(simplexVBits, (dimension+1));
   new_Matrix(refSimplex, dimension+1,dimension);
   new_array(refSimplexValues, (dimension+1));
   new_array(refSimplexVBits, (dimension+1));
   IDnumber = 3300;
} //special constructor using void function and object pointers

SMDSearch::~SMDSearch()
{
  if(simplexVBits != NULL) {
    delete [] simplexVBits;
    simplexVBits = NULL;
  } //if
  if(refSimplex != NULL) {
    delete refSimplex;
    refSimplex = NULL;
  } //if 
  if(refSimplexValues != NULL) {
    delete [] refSimplexValues;   
    refSimplexValues = NULL;
  } //if
  if(refSimplexVBits != NULL) {
    delete [] refSimplexVBits;
    refSimplexVBits = NULL;
  } //if
   //NOTE: Matrix and Vector classes have their own destructors
} // ~SMDSearch


SMDSearch& SMDSearch::operator = (const SMDSearch& A)
{
  CopySearch(A);
  return(*this);
} //overloaded assignment operator

void SMDSearch::ChooseRightSimplex()
{
  InitRightSimplex();
} // InitRegularTriangularSimplex()


void SMDSearch::ChooseRegularSimplex()
{
  InitRegSimplex();  
} // InitRegularTriangularSimplex()

void SMDSearch::BeginSearch()
{
  if(SimplexSpecified == false) {
    ChooseRightSimplex();
  } //if
  ExploratoryMoves();
} //BeginSMDSearch()


void SMDSearch::InitRightSimplex()
{
   Matrix<double> *plex = NULL;
   new_Matrix(plex, dimension+1, dimension);
   for( long i = 0; i < dimension; i++ ) {
      // we're building the minPoint component-by-component into
      // the (n+1)st row
      (*plex)[dimension][i] = (*minPoint)[i];

      // now fill in the ith row with the proper point
      for( long j = 0; j < dimension; j++ ) {
         (*plex)[i][j] = (*minPoint)[j];
         if( i == j )
            (*plex)[i][j] += (*starting_edgeLengths)[i];
      }
      // SimplexSearch::InitRightSimplex(plex);
      // for( long i = 0; i < dimension; i++ ) { 
      if((*starting_edgeLengths)[i] > delta ) {
          delta = (*starting_edgeLengths)[i];
      }
   } //for
   InitGeneralSimplex(plex);
   if(plex != NULL) delete plex;
   plex = NULL;
} // InitVariableLengthRightSimplex()


void SMDSearch::InitRegSimplex()  
{
   Matrix<double> *plex = NULL;
   new_Matrix(plex, dimension+1, dimension);
   SimplexSpecified = true;
   SimplexSearch::Initialize_Regular(plex);
   InitGeneralSimplex(plex); 
   delta = (*starting_edgeLengths)[0]; 
   if(plex != NULL) delete plex;
   plex = NULL;
} // InitRegularTriangularSimplex()

void SMDSearch::InitGeneralSimplex(Matrix<double> *plex)
{
   functionCalls = 0;
   if( design != NULL ) { 
     delete design; 
     design = NULL;
   }
   if( simplexValues != NULL ) {
     delete [] simplexValues;
     simplexValues = NULL;
   }

/*
#if defined(AGODEMAR)

   // agodemar: check bounds here

   Matrix<double> myPlex = (*plex);

   for( long j = 0; j <= dimension; j++ ) 
   {
     for( long k = 0; k < dimension; k++ )
     {
       if ( myPlex[j][k] > bds.upper[k] ) myPlex[j][k]=bds.upper[k];
       if ( myPlex[j][k] < bds.lower[k] ) myPlex[j][k]=bds.lower[k];
     }
   }

   new_Matrix_Init(design, myPlex);

   // ... agodemar
#else
*/
   new_Matrix_Init(design, (*plex));
//#endif

   new_array(simplexValues, dimension+1);
   //  (*simplex) = (*plex);

   // zero out the valid bits
   for(long i = 0; i < dimension; i++)
     simplexVBits[i] = 0;

   // NOTE: the basePoint MUST be located in the last row of plex

/*
#if defined(AGODEMAR)
   Vector<double> basePoint = myPlex.row(dimension); // agodemar
#else
*/
   Vector<double> basePoint = (*plex).row(dimension);
//#endif

   // evaluate f(basePoint) and initialize it as the min
   bool success;
   fcnCall(dimension, (basePoint), simplexValues[dimension],
           success, some_object);
   if(!success) cerr<<"Error with basePoint in initial simplex.\n";
   simplexVBits[dimension] = 1;
   (*minPoint) = (basePoint);
   minValue = simplexValues[dimension];
   currentIndex = dimension;
   minIndex = dimension;

   // if we still haven't defined delta, go through the simplex and
   // define delta to be the length of the LONGEST simplex side
   double temp;
   if( delta < 0.0 ) {
     for( long j = 0; j < dimension; j++ ) {
       for ( long k = j+1; k <= dimension; k++ ) {
         temp = ( ((*design).row(j)) - ((*design).row(k)) ).l2norm();
         if( temp > delta ) delta = temp;
       } // inner for
     } // outer for
   } // outer if

   // if delta is still not defined, there is a definite problem
   if( delta < 0.0 )
     cout << "Error in simplex initialization: delta not set.\n";
} // InitGeneralSimplex()


void SMDSearch::ReadInFile(istream& fp)
{
   if(fp == NULL) {
      cerr<<"No Input Stream in ReadSimplexFile()!\n";
      return; // There's no file handle!!
   }

   //++++What is basePoint here for?? it just gets destroyed.++++ ---pls   
   // Vector<double> *basePoint = NULL;
   // new_Vector(basePoint, dimension);
   Matrix<double> *plex = NULL;
   new_Matrix(plex, dimension+1, dimension);
   for( long i = 0; i <= dimension; i++ ) {
      for ( long j = 0; j < dimension; j++ ) {
         fp >> (*plex)[i][j];
      } // inner for
   } // outer for
   //(*basePoint) = (*plex).row(dimension);
   InitGeneralSimplex(plex);
   // delete basePoint;
   // basePoint = NULL;
   delete plex;
   plex = NULL;
   SimplexSpecified = true;
} // ReadSimplexFile()

// Query functions


void SMDSearch::GetCurrentSimplexVBits(long* &simVBits) const
{
  if(simVBits != NULL) delete[] simVBits; 
   new_array(simVBits, dimension+1);
   for( long i = 0; i <= dimension; i++ ) {
      simVBits[i] = simplexVBits[i];
   } // for
} // GetCurrentSimplexValues()


void SMDSearch::ExploratoryMoves()
{
   bool done;
   long lastMinIndex = minIndex;
   toleranceHit = 0;

   do {
     done = 0;
     CreateRefSimplex();

     if(DEBUG) {
       PrintDesign();
       printRefSimplex();
     }

#if defined(AGO_DIRECTSEARCH)
     PrintfMin();
#endif

     // Go through the Reflection Simplex First
     refCurrentIndex = lastMinIndex;
     while( !done && GetAnotherIndex(refCurrentIndex, refSimplexVBits) ) {
         if( exact_count == true
             && maxCalls != NO_MAX
             && functionCalls >= maxCalls ) break;
         CalculateRefFunctionValue(refCurrentIndex);
         refSimplexVBits[refCurrentIndex] = 1;

         if(DEBUG) printRefSimplex();

         if( refSimplexValues[refCurrentIndex] < minValue ) {
             (*minPoint) = (*refSimplex).row(refCurrentIndex);
             minValue = refSimplexValues[refCurrentIndex];
             lastMinIndex = minIndex;
             minIndex = refCurrentIndex;
             SwitchSimplices();
             done = 1;
         } // if
       
         if( exact_count == true
             && maxCalls != NO_MAX
             && functionCalls >= maxCalls ) break;
     } // while (reflection search)

     // Go through the Primary Simplex Next
     while( !done && GetAnotherIndex(currentIndex, simplexVBits) ) {
         if( exact_count == true
             && maxCalls != NO_MAX
             && functionCalls >= maxCalls ) break;
         CalculateFunctionValue(currentIndex); 
         simplexVBits[currentIndex] = 1;
         // NOTE: currentIndex initialized in InitGeneralSimplex()

         if(DEBUG) PrintDesign();

         if( simplexValues[currentIndex] < minValue ) {
             (*minPoint) = (*design).row(currentIndex);
             minValue = simplexValues[currentIndex];
             lastMinIndex = minIndex;
             minIndex = currentIndex;
             done = 1;
         } // if

         if( exact_count == true
             && maxCalls != NO_MAX
             && functionCalls >= maxCalls ) break;
         //if( functionCalls >= maxCalls ) return;
     } // while (primary search)
     
     // Still there's no new min now, shrink
     if( !done ) {
         if( exact_count == true
             && maxCalls != NO_MAX
             && functionCalls >= maxCalls ) break;
         ShrinkSimplex();
     }
   } while (!Stop());   // while stopping criteria is not satisfied
} // ExploratoryMoves()

bool SMDSearch::Stop()
{
    toleranceHit = static_cast<int>(delta < stoppingStepLength);
    bool stopBool = (SimplexSearch::Stop() || toleranceHit);
    return stopBool;
} // Stop()

void SMDSearch::CopySearch(const SMDSearch & Original)
{
  SimplexSearch::CopySearch(Original);
  if(simplexVBits != NULL) {
    delete[] simplexVBits;
    simplexVBits = NULL;
  }
  Original.GetCurrentSimplexVBits(simplexVBits); 
  //minValue = Original.minValue;
  delta = Original.delta;
  if(refSimplex != NULL) {
    delete refSimplex;
    refSimplex = NULL;
  }
  new_Matrix_Init(refSimplex, *(Original.refSimplex));
  if(refSimplexValues != NULL){ 
    delete [] refSimplexValues;
    refSimplexValues = NULL;
  } //if
  new_array(refSimplexValues, (dimension+1));
  for(int i = 0; i <= dimension; i++) {
    refSimplexValues[i] = Original.refSimplexValues[i];
  } //for
  if(refSimplexVBits != NULL) { 
    delete [] refSimplexVBits;
    refSimplexVBits = NULL;
  } //if
  new_array(refSimplexVBits, (dimension+1));
  for(int j = 0; j <= dimension; j++) {
    refSimplexVBits[j] = Original.refSimplexVBits[j];
  } //for
}

void SMDSearch::CreateRefSimplex()
{ 
  // copy the known flip point over
  for( long i = 0; i < dimension; i++ )
    (*refSimplex)[currentIndex][i] = (*design)[currentIndex][i];
  refSimplexValues[currentIndex] = simplexValues[currentIndex];
  refSimplexVBits[currentIndex] = simplexVBits[currentIndex];
  refCurrentIndex = currentIndex;

  // reflect the remaining points   
  for( long j = 0; j <= dimension; j++ ) {
    if( j != currentIndex ) {
      refSimplexVBits[j] = 0;
      (*scratch) = ( (*design).row(currentIndex) * 2.0 ) - (*design).row(j);
      for( long k = 0; k < dimension; k++ )
      {
/*
#if defined(AGODEMAR)
         // agodemar: check bounds here
         double aa = (*scratch)[k];
         if ( (*scratch)[k] > bds.upper[k] ) aa=bds.upper[k];
         if ( (*scratch)[k] < bds.lower[k] ) aa=bds.lower[k];
         (*refSimplex)[j][k] = aa;
         // ... agodemar
#else
*/
         (*refSimplex)[j][k] = (*scratch)[k];
//#endif
      }
    } // if
  } // outer for
} // CreateRefSimplex()

void SMDSearch::SwitchSimplices()
{
  // this allows us to remove the need to delete and
  // reallocate memory by simply swapping pointers
  // and using the same two "simplex memory slots"
  // for the entire search

  Matrix<double> *tmp1 = design;
  double         *tmp2 = simplexValues;
  long            *tmp3 = simplexVBits;
  long             tmp4 = currentIndex;

  design = refSimplex;
  simplexValues = refSimplexValues;
  simplexVBits = refSimplexVBits;
  currentIndex = refCurrentIndex;

  refSimplex = tmp1;
  refSimplexValues = tmp2;
  refSimplexVBits = tmp3;
  refCurrentIndex = tmp4;
} // SwitchSimplices()

void SMDSearch::ShrinkSimplex()
{
  if(DEBUG) cout << "Shrinking Simplex.\n\n";

   delta *= sigma;
   currentIndex = minIndex;
   Vector<double> *lowestPt = scratch;
   *lowestPt = (*design).row(minIndex);
   Vector<double> *tempPt = scratch2;
 
   for( long i = 0; i <= dimension; i++ ) {
      if( i != minIndex ) {
         *tempPt = (*design).row(i);
         (*tempPt) = (*tempPt) + ( sigma * ( (*lowestPt)-(*tempPt) ) );
         for( long j = 0; j < dimension; j++ ) {

/*
#if defined(AGODEMAR)
            // agodemar: check bounds here
            double aa = (*tempPt)[j];
            if ( (*tempPt)[j] > bds.upper[j] ) aa=bds.upper[j];
            if ( (*tempPt)[j] < bds.lower[j] ) aa=bds.lower[j];
            (*design)[i][j] = aa;
            // ... agodemar
#else
*/
            (*design)[i][j] = (*tempPt)[j];
//#endif
         } // inner for

         simplexVBits[i] = 0;

      } // if
   } // outer for
} // ShrinkSimplex()

long SMDSearch::GetAnotherIndex(long& index, long*& validBits)
{
  //++++ shouldn't these be bools or even ints, not longs?  --pls 7/00
  if ( !validBits[index] ) return 1;

  long initialIndex = index;
   
  do {
    index++;
    if( index > dimension ) index = 0;
  } while ( ( index != initialIndex) && 
            ( validBits[index] ) );
    
  if( index == initialIndex )
    return 0;
  else
    return 1;
} // GetAnotherIndex()

void SMDSearch::CalculateRefFunctionValue(long index)
{
   *scratch = (*refSimplex).row(index);
   bool success;
   fcnCall(dimension, (*scratch), 
           refSimplexValues[index], success, some_object);
   if(!success) cerr<<"Error calculating point at index "
                    << index << "in CalculateFunctionValue().\n";
} // CalculateFunctionValue()

void SMDSearch::PrintDesign() const
{
  cout << "Primary Simplex:\n";

  for( long i = 0; i <= dimension; i++ ) {
     cout << "Point: ";
     for ( long j = 0; j < dimension; j++ ) {
       cout << (*design)[i][j] << " ";
     } // inner for
     cout << "   Value: " << simplexValues[i];
     
     if( simplexVBits[i] )
       cout << "   Valid\n";
     else
       cout << "   Invalid\n";
  } // outer for

  cout << "FCalls: " << functionCalls 
       << "   Delta: " << delta << "\n\n";
  cout << "stopping step length = " << stoppingStepLength << endl;
} // PrintDesign()

void SMDSearch::printRefSimplex() const
{
  cout << "Reflection Simplex:\n";

  for( long i = 0; i <= dimension; i++ ) {
     cout << "Point: ";
     for ( long j = 0; j < dimension; j++ ) {
       cout << (*refSimplex)[i][j] << " ";
     } // inner for
     cout << "   Value: " << refSimplexValues[i];
     
     if( refSimplexVBits[i] )
       cout << "   Valid\n";
     else
       cout << "   Invalid\n";
  } // outer for

  cout << "FCalls: " << functionCalls 
       << "   Delta: " << delta << "\n\n";
} // printRefSimplex()
