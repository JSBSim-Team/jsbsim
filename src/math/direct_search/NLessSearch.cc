/*  NLessSearch.cc 
 *  implementations of the derived class NLessSearch functions.
 *  The NLessSearch searches about a regular simplex (i.e. 
 *  minimal positive basis) until finding improvement in the
 *  objective function value.  Then the search relocates to the
 *  improving point and begins again.
 *  Liz Dolan, The College of William & Mary, 1999
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
 *  DirectSearch version, modified 5/00 - 1/01 by Anne Shepherd              
 */                                                                 

#include <math/direct_search/NLessSearch.h>

NLessSearch::NLessSearch(long numberOfVariables, Vector<double> &startPoint)
  : PatternSearch(numberOfVariables, startPoint)
{
  IDnumber = 2500;   
}

NLessSearch::NLessSearch(long dim, 
			 Vector<double> &startPoint,
			 double startStep,
			 double stopStep,
			 void (*objective)(long vars, Vector<double> &x, 
					   double & func, bool& flag,
                                           void* an_obj),
			 void * input_obj)
  : PatternSearch(dim, startPoint, startStep, stopStep,
		 objective, input_obj)
{
  IDnumber = 2500;   
}
 

NLessSearch::~NLessSearch()
{
  IDnumber = 2500;   
}
 
NLessSearch& NLessSearch::operator=(const NLessSearch &A)
{
  PatternSearch::CopySearch(A);
  return (*this);
}/*end operator=*/


void NLessSearch::BeginSearch()
{
  ExploratoryMoves();
} //BeginNLessSearch()

void NLessSearch::ExploratoryMoves()
{
  long dim = GetDimension();
  CreatePattern();     //creates a regular simplex
  Vector<double> currentPoint(dim);
  Vector<double> nextPoint(dim);
  double value;
  double nextValue;
  long length;
  bool success = false;
  GetMinPoint(currentPoint);  //initialize min point
  bool flag = false;
  fcn_name(dimension, (*minPoint), minValue, flag, some_object);
  if (!flag)
  cerr << "\nError signal in objective function at starting point.\n";
  GetMinVal(value);           //and obj. func. value
  GetPatternLength(length);
  //search the pattern in each direction until improvement is found,
  //then stop and begin a new iteration at the better point

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
		    i = -1; //start the search over at the new point
		  }//if there is improvement
	      }//if able to get a function value
	}//for now we know that there aren't better points around this one
	UpdatePattern();
    }while(!Stop());//while we haven't stopped()
}//ExploratoryMoves

void NLessSearch::CreatePattern()
/*
  For information on how to build a regular
  simplex, see pages 79-81 S.L.S. Jacoby, J.S. Kowalik 
  and J.T. Pizzo.
  Iterative Methods for Nonlinear Optimization Problems. 
  Prentice Hall, Inc., Englewood Cliffs, NJ. 1972.
*/
{
  long vars = GetDimension();
  //refer to book for p and q
  double p = (sqrt(vars+1.0) -1.0 + vars)/(vars*sqrt(2.0));
  double q = (sqrt(vars+1.0) -1.0)/(vars*sqrt(2.0));
  Matrix<double> nlessPattern(vars, vars+1);  //vars + 1 = # of vectors in pattern
  InitializeDesign(vars + 1, &nlessPattern); //initialize pattern to default
  Vector<double> basis(vars);  
  //basis is the first point used to create a simplex according to the algorithm
  if(vars > 0)
    {
      for(long j = 0; j < vars; j++) 
	{
	  basis[j] = (- (p+((vars-1)*q))/(vars+1));
	  nlessPattern[j][0] = basis[j];
	}

      for(long i = 1; i < vars + 1; i++)
	{
	  for(long k = 0; k < vars; k++)
	    {
	      nlessPattern[k][i] = basis[k] + q;
	    }
	  nlessPattern[i-1][i] = basis[i-1] + p;
	}//outer for
      //make sure that the vectors of the pattern not only point
      //in the right direction, but are also of desired length
      SizePattern(vars, nlessPattern, 1.0);
      InitializeDesign(vars + 1, &nlessPattern);
    }//if there's anything to allocate
}//CreatePattern
  
void NLessSearch::UpdatePattern()
{
  ScalePattern(0.5);
}//UpdatePattern

void NLessSearch::SizePattern(long dimens, Matrix<double> &pat, double size)
{
  long length;
  GetPatternLength(length);  //length of the pattern
  Vector<double> compare(dimens);
  Vector<double> compareTo(dimens);
  double compDist; //distance from the centroid
  compareTo = pat.col(0); //initialize for testing
  for(long i = 0;i < length;i++)
    {
      compare = pat.col(i);
      compDist = 0;

      //find the distance between the vectors being compared
      compDist = compare.l2norm();

      //resize to desired length, based on knowledge of
      //relationships in the ratios of similar triangles
      for(long j = 0; j < dimens; j++)
	{
	  pat[j][i] = size*compare[j]/compDist;
	}
      compDist = 0;
      
    }//for

}







