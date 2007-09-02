/*  CoordinateSearch.cc
 *  implementation of the CoordinateSearch class to find a minimal
 *  objective function solution.
 *  A Coordinate search checks the positive and negative quardinate vectors for
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
 *  DirectSearch version, modified 5/00 by P.L. Shepherd 5/00 - 01/01
 *
 */                                                                 

#include "CoordinateSearch.h"

CoordinateSearch::CoordinateSearch(long numberOfVariables,
                                   Vector<double> &startPoint)
  : PatternSearch(numberOfVariables, startPoint)
{
    IDnumber = 2100;
}

CoordinateSearch::CoordinateSearch(long dim, 
				   Vector<double> &startPoint,
				   double startStep,
				   double stopStep,
				   void (*objective)(long vars, Vector<double> &x, 
						     double & func,
                                                     bool& flag,
                                                     void* an_obj),
				   void * input_obj)
  : PatternSearch(dim, startPoint, startStep, stopStep,
		 objective, input_obj)
{
    IDnumber = 2100;
}  //special constructor for use with MAPS
  
CoordinateSearch::~CoordinateSearch()
{
} //destructor


CoordinateSearch& CoordinateSearch::operator=(const CoordinateSearch &A)
{
  PatternSearch::CopySearch(A);
  return (*this);
}/*end operator=*/


void CoordinateSearch::BeginSearch()
{
  ExploratoryMoves();
} //BeginCoordinateSearch

void CoordinateSearch::ExploratoryMoves()
{
  long dim = GetDimension();
  CreatePattern();
  Vector<double> currentPoint(dim);
  Vector<double> nextPoint(dim);
  double value;
  double nextValue;
  long length;
  bool success = false;
  bool decrease = false;
  GetMinPoint(currentPoint);

  bool flag = false;
  fcn_name(dimension, (*minPoint), minValue, flag, some_object);
  if (!flag)
  cerr << "\nError signal in objective function at starting point.\n";
  GetMinVal(value);
  GetPatternLength(length);
  do
    {
      decrease = false;
      for( long i = 0; i < length; i++)
	{
	    NextPoint(i, currentPoint, nextPoint);
            if(BreakOnExact()) break; 
	    fcnCall(dim, nextPoint, nextValue, success, some_object);
	    if(success)
	      {
		if(nextValue < value)
		  {
		    decrease = true;
		    ReplaceMinimum(nextPoint, nextValue);
		    value = nextValue;
		    currentPoint = nextPoint;
		  }//if there is improvement
	      }//if able to get a function value
	    if( (i==(length - 1)) & (decrease==true))
	      {
		decrease = false;
		i = -1;
		//if the iteration found function decrease, repeat local search
	      } 
	}//for now we know that there aren't better points around this one
      UpdatePattern();
    }while(!Stop());//while we haven't stopped()
}//ExploratoryMoves

void CoordinateSearch::CreatePattern()
{
  long vars = GetDimension();
  Matrix<double>* coorPattern = NULL;
  new_Matrix(coorPattern, vars, 2*vars);
  if(vars > 0)
    {
      for(long j = 0; j < vars; j++)
	{
	  (*coorPattern)[j][2*j] = 1.0;
	  (*coorPattern)[j][2*j+1] = -1.0;
	}//for
      InitializeDesign(2*vars, coorPattern);
      delete coorPattern;
    }//if there's anything to allocate

}//CreatePattern
  
void CoordinateSearch::UpdatePattern()
{
  ScalePattern(0.5);
}//UpdatePattern


