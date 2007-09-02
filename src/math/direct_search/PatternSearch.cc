/* PatternSearch.cc
 * class implementation of pattern search optimization basis
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
 *  DirectSearch version, modified by P.L.(Anne) Shepherd, 5/00 -
 *     01/01
 */                                                                 

#include <math/direct_search/PatternSearch.h>

//constructors & destructor

//Remove any attempt to evaluate the function in the constructor
PatternSearch::PatternSearch(long dim, Vector<double> &startPoint)
    : DirectSearch(dim, startPoint)
{
    patternLength = 0;
    initialStepLength = 1.0;
    delta = initialStepLength;
    IDnumber = 2000;
    // InitializeStartPoint();
}//constructor initializes private data members to user defined starting values

PatternSearch::PatternSearch(const PatternSearch & Original)
    : DirectSearch(Original)
{
    Original.GetPatternLength(patternLength);
    initialStepLength = Original.initialStepLength;
    delta = Original.delta;
    exact_count = Original.exact_count;
    IDnumber = 2000;
}//deep copy constructor


/**special constructor using void function and object pointers.  
   The objective function 
     can be sent in as the fourth parameter here; 
     the last parameter is used for any other
     information that may be necessary; it is used in MAPS, 
     for example, to send in an
     object from an outside class. Usually, though, 
     the final parameter will simply be 
     set to NULL.*/
PatternSearch::PatternSearch(long dim, 
                             Vector<double> &startPoint,
                             double startStep,
                             double stopStep,
                             void (*objective)(long vars, Vector<double> &x, 
                                               double & func, bool& flag, void* an_obj),
                             void * input_obj)
    : DirectSearch(dim, startPoint, stopStep, objective, input_obj)
{
    patternLength = 0;
    initialStepLength = startStep;
    delta = initialStepLength;
    IDnumber = 2000;
} //special constructor for MAPS
  

PatternSearch::~PatternSearch()
{
}//destructor

//other search initialization routines

void PatternSearch::CopySearch(const PatternSearch & Original)
{
    if(this != &Original) {
        DirectSearch::CopySearch(Original);
        Original.GetPatternLength(patternLength);
        delta = Original.delta;
        initialStepLength = Original.initialStepLength;
    }
}//deep copy  

PatternSearch& PatternSearch::operator=(const PatternSearch& A)
{
    CopySearch(A);
    return(*this);
}

void PatternSearch::CleanSlate(long dim, Vector<double> &startPoint)
//reinitialize all values
{
    DirectSearch::CleanSlate(dim, startPoint);
    patternLength = 0;
    GetInitialStepLength(initialStepLength);
    delta = initialStepLength;
}//reinitialize all search values to appropriate user-defined start values

void PatternSearch::CleanSlate(long dim, 
                               Vector<double> &startPoint,
                               double startStep,
                               double stopStep,
                               void (*objective)(long vars, Vector<double> &x, 
                                                 double & func, bool& flag, void* an_obj),
                               void * input_obj)
//reinitialize all values--overloaded for use with MAPS
{
    DirectSearch::CleanSlate(dim, startPoint, stopStep, objective, input_obj);
    patternLength = 0;
    initialStepLength = startStep;
    delta = initialStepLength;
} //CleanSlat overloaded for use with MAPS
  
//algorithmic routines



void PatternSearch::InitializeDesign(long patternSize, const Matrix<double> *designPtr)
{
    DirectSearch::InitializeDesign(designPtr);
    patternLength = patternSize;
}//InitializeDesign

//Reads the pattern in column by column.
//The first digit read must be the number of columns.
//++++++++++We'd better test this!!!!!!+++++++++++++

void PatternSearch::ReadInFile(istream & fp)
{
    Matrix<double> * temp = NULL;
    long dim = GetDimension();

    //++++++we need to add some error handling here.+++++++  --pls
    if (fp==NULL) {
        cout << "\nThere's no file to read!! Exiting...\n";
        exit(1);
    } //if
    fp >> patternLength;  //the length of the pattern must precede the pattern
    new_Matrix(temp, dim, patternLength);
    for(long i = 0; i < patternLength; i++)
    {
        for(long j = 0; j < dim; j++)
	{
            fp >> *temp[j][i];
	}//inner for
    }//outer for
    InitializeDesign(patternLength, temp);
    delete temp;
}//ReadInFile

void PatternSearch::PrintDesign() const
{
    double minValue;
    Vector<double> optPoint;

    cout << "\nNumber of function calls so far is:";
    cout << GetFunctionCalls() << endl;
  
    cout << "\nMinimum value is:"; 
    GetMinVal(minValue);
    cout << minValue << endl;

    cout << "\nMinimum point is:\n";
    GetMinPoint(optPoint);
    cout << optPoint << endl;

    cout << "\nStep Length is:";
    cout << GetDelta() << endl;
} //printDesign


//query functions


void PatternSearch::GetPatternLength(long & len) const
{
    len = patternLength;
}//GetPatternLength - trial steps in pattern (# of columns)

void PatternSearch::GetInitialStepLength(double & stepLen)
{
    stepLen = initialStepLength;
} //GetInitialStepLength

void PatternSearch::SetInitialStepLength(double & stepLen)
{
    initialStepLength = stepLen;
}

double PatternSearch::GetDelta() const
{
    return delta;
}//GetDelta - returns delta, the lattice refinement value

//protected member functions

void PatternSearch::NextPoint(long index, const Vector<double> & currentPoint, 
			      Vector<double> & nextPoint)
{
    //To get the next point, add the currentPoint to the
    //product of the pattern vector at index and lattice step length
    //++++++++add a msg for null pointer++++++  --pls 6/12
    Matrix<double> *temp = NULL;
    new_Matrix_Init(temp, (*design));
    if (temp != NULL && patternLength > index)
        nextPoint = currentPoint + (delta * ((*temp).col(index)));
    delete temp;
    temp = NULL;
}

// why does it have to be a const here??? 
//  ---took out b/c compiler whined --pls 6/6
void PatternSearch::ReplaceMinimum(Vector<double> & newPoint, double newValue)
{
    (*minPoint) = newPoint;
    minValue = newValue;
    // SetMinPoint(newPoint);
    // SetMinVal(newValue);
}//UpdateMinPoint

void PatternSearch::ScalePattern(double scalar)
{
    delta = delta * scalar; 
}//ScalePattern--scale delta step length by scalar

//Stop based on cap on function evaluations or 
// lower limit on trial step length 

bool PatternSearch::Stop()  
{
    return ( (DirectSearch::Stop() ) || (delta < stoppingStepLength) );
}//Stop 

//pattern-altering functions
