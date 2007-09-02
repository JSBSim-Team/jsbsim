/*  CompassSearch.cc
 *  implementation of the CompassSearch class to find a minimal objective function
 *  solution.
 *  A compass search checks the positive and negative quardinate vectors for
 *  each dimension until improvement in the function values is found.  
 *  The search then relocates to the improving point and begins again.
 *  Liz Dolan, The College of William and Mary, 1999
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
 *  DirectSearch version, modified by P.L. Shepherd, 5/00 - 01/01               */                                                                 

#include <math/direct_search/CompassSearch.h>


CompassSearch::CompassSearch(long numberOfVariables, Vector<double> &startPoint)
  : PatternSearch(numberOfVariables, startPoint)
{
    IDnumber = 2200;
}


CompassSearch::CompassSearch(long dim, 
			     Vector<double> &startPoint,
			     double startStep,
			     double stopStep,
			     void (*objective)(long vars, Vector<double> &x, 
					       double & func, bool&
                                               flag,
                                               void* an_obj),
			     void * input_obj)
  : PatternSearch(dim, startPoint, startStep, stopStep,
		 objective, input_obj)
{    
    IDnumber = 2200;
}
 

CompassSearch::~CompassSearch()
{
}

CompassSearch& CompassSearch::operator=(const CompassSearch &A)
{
  PatternSearch::CopySearch(A);
  return (*this);
}/*end operator=*/


void CompassSearch::BeginSearch()
{
  ExploratoryMoves();
} //BeginCompassSearch

void CompassSearch::ExploratoryMoves()
{
  long dim = GetDimension();
  //++++why do we need this??  ---pls 6/00
  double pace = 0.0;
  pace = GetDelta();
  CreatePattern();
  Vector<double> currentPoint(dim);
  Vector<double> nextPoint(dim);
  double value;
  double nextValue;
  long length;
  bool success = false;
  GetMinPoint(currentPoint);

  bool flag = false;
  fcn_name(dim, (*minPoint), minValue, flag, some_object);
  if (!flag)
  cerr << "\nError signal in objective function at starting point.\n";
  GetMinVal(value);
  GetPatternLength(length);
  do
    {
      for( long i = 0; i < length; i++)
	{
	    NextPoint(i, currentPoint, nextPoint);  
            if(BreakOnExact()) break; 
	    fcnCall(dim, nextPoint, nextValue, success, some_object);
	    if(success)
	      {
		if(nextValue < value)
		  {
		    ReplaceMinimum(nextPoint, nextValue);
		    value = nextValue;
		    currentPoint = (nextPoint);
		    i = -1; //start the compass search over at the new point
		  }//if there is improvement
	      }//if able to get a function value
	}//for now we know that there aren't better points around this one
      UpdatePattern();
    }while(!Stop());//while we haven't stopped()
}//ExploratoryMoves

void CompassSearch::CreatePattern()
{
  long vars = GetDimension();
  Matrix<double>* compassPattern = NULL;
  new_Matrix(compassPattern, vars, 2*vars);
  if(vars > 0)
    {
      for(long j = 0; j < vars; j++)
	{
	  (*compassPattern)[j][2*j] = 1.0;
	  (*compassPattern)[j][2*j+1] = -1.0;
	}//for
      InitializeDesign(2*vars, compassPattern);
      delete compassPattern;
      compassPattern = NULL;
    }//if there's anything to allocate
}//CreatePattern
  
void CompassSearch::UpdatePattern()
{
  ScalePattern(SCALE_FACTOR);
}//UpdatePattern





