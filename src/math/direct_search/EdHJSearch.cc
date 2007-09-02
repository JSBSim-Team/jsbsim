/*  EdHJSearch.cc
 *  implementation of the EdHJSearch class to find a minimal objective function
 *  solution.
 *  Includes a minor modification to the basic Hooke and Jeeves strategy to
 *  avoid making pattern steps directly after contractions (which mostly cover
 *  the same ground that was already covered in the search step preceding 
 *  the contraction.
 *  For a good description of the Hooke and Jeeves search algorithm
 *  I recommend Non-Linear Optimization Techniques by Box, Davies,
 *  and Swann, 1969. 
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
 *  DirectSearch version, modified 5/00 - 1/01 by Anne Shepherd          
 */                                                                 

#include <math/direct_search/EdHJSearch.h>


EdHJSearch::EdHJSearch(long numberOfVariables, Vector<double> &startPoint)
  : PatternSearch(numberOfVariables, startPoint)
{
  step = initialStepLength;
  factor = 0.5;
  IDnumber = 2400;    
}

EdHJSearch::EdHJSearch(const EdHJSearch & Original): PatternSearch(Original)
{
  step = Original.step;
  factor = Original.factor;
  IDnumber = 2400;   
} 

EdHJSearch::EdHJSearch(long dim, 
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
  step = initialStepLength;  //as provided by the user or default
  factor = 0.5;              //factor by which the step length is reduced
  IDnumber = 2400;   
} // special constructor for use with MAPS

 
EdHJSearch::~EdHJSearch()
{
}

void EdHJSearch::CopySearch(const EdHJSearch & Original)
{
  PatternSearch::CopySearch(Original);
  step = Original.step;
  factor = Original.factor;
} 

EdHJSearch& EdHJSearch::operator=(const EdHJSearch& A)
{
  CopySearch(A);
  return(*this);
}

void EdHJSearch::CleanSlate(long dimensions, Vector<double> &startPoint)
//reinitialize all values
{
  PatternSearch::CleanSlate(dimensions, startPoint);
  step = initialStepLength;
}

//reinitialize all values--overloaded for use with MAPS
void EdHJSearch::CleanSlate(long dim, 
			    Vector <double> &startPoint,
			    double startStep,
			    double stopStep,
			    void (*objective)(long vars, Vector<double> &x,
                                              double & func, 
					      bool& flag, void* an_obj),
			    void * input_obj)
{
  PatternSearch::CleanSlate(dim, startPoint, startStep, stopStep,
                            objective, input_obj);
  step = initialStepLength;
}


void EdHJSearch::BeginSearch()
{
  ExploratoryMoves();
} //BeginEdHJSearch()

void EdHJSearch::ExploratoryMoves()
{
  Vector<double> currentPoint(dimension);
  Vector<double> lastImprovingPoint(dimension); //last base point
  Vector<double> storage(dimension); //for intermediate storage
                                     //to reduce rounding error
  Vector<double> direction(dimension, 0.0); //direction of pattern extending step
  double value;                   //objective function value
  double positiveValue;           //obj.fun. value in the positive step 
  double negativeValue;           // " for the negative step 
  double lastImprovingValue;      //obj.fun.value of last base point
  bool success = false;
  bool foundImprove = false;
  bool contracted = false;
  bool flag = false;
  fcn_name(dimension, (*minPoint), minValue, flag, some_object);
  if (!flag)
  cerr << "\nError signal in objective function at starting point.\n";
  GetMinVal(value);
  GetMinVal(lastImprovingValue);
  GetMinPoint(currentPoint);      //initialize to the user initial point
  GetMinPoint(lastImprovingPoint);
  GetMinPoint(storage);
  do
    {
      for(long iteration=0;iteration < dimension; iteration++)
	{
	  
	  currentPoint[iteration] += step;
          if(BreakOnExact()) break; 
	  fcnCall(dimension, currentPoint, positiveValue, success, some_object);
	  if(!success)
	    {
	      positiveValue = value + 1.0;
	      //if the call returned unsuccessfully, set positiveValue
              //to a value that will not be improving	    
	    }
	  if(positiveValue < value)
	    {
	      value = positiveValue;
	      foundImprove = true;
	      //continue search in other dimensions from here
	    }//if positive is better
	    
	  if(!foundImprove)
	    {
	      currentPoint = storage;
	      currentPoint[iteration] -= (step);              
              if(BreakOnExact()) break; 
	      fcnCall(dimension, currentPoint, negativeValue, success, some_object);             
	      if(!success)
		{
		  negativeValue = value + 1.0;
		  //same kludge as in positive case
		}
	  
	      if(negativeValue < value)
		{
		  value = negativeValue;
		  foundImprove = true;
		  //continue search in other dimensions from here
		}//if negative direction is better
	    }//if we need to check the negative
	  if(!foundImprove)
	    {//reset to original position
	      currentPoint = storage;
	    }//if neither direction gave improvement
	  else
	    {
	      storage = currentPoint;
	    }
	  foundImprove = false; //reset for next iteration
	}//for
      //+++BUG!! it keeps hanging on this--only with new stuff --pls 7/21/00
      direction = currentPoint - lastImprovingPoint;
      //direction now holds the extended pattern step vector
      if(value < lastImprovingValue)
	{
	  //check whether the "new" point is within factor*step of the old
	  if(isnear(lastImprovingPoint, currentPoint, factor*step))
	    {
	      currentPoint = lastImprovingPoint;
	      value = lastImprovingValue;
	      storage = currentPoint;	      
	    }
	  else
	    {
	      lastImprovingValue = value;
	      ReplaceMinimum(currentPoint, value);
	      lastImprovingPoint = currentPoint;
	      if(!contracted)   //my (edd)  personal modification to the
                                // algorithm 
                      
		{
		  //take the pattern extending step and find its value
		  currentPoint = direction + currentPoint;
		  storage = currentPoint; 		             
                  if(BreakOnExact()) break; 
		  fcnCall(dimension, currentPoint, value, success,
                          some_object);
                  if(BreakOnExact()) break; 
       		}
	    }
	  contracted = false;
	}
      else
	{
	  if(isnear(currentPoint, lastImprovingPoint, factor * step))
	    {
	      step = step * factor;
	      contracted = true;
	    }
	  else
	    {
	      //this case can only occur after an unsuccessful
	      //search about a pattern-step-located point
	      //move back to the point that was improving from the
	      //search about the last base point
	      GetMinPoint(currentPoint);
	      GetMinVal(value);
	      storage = currentPoint;
	      contracted = false;
	    }
	}
    }while(!Stop());//while we haven't stopped()
}//ExploratoryMoves


bool EdHJSearch::Stop()
//makes certain search has not exceeded maxCalls or 
//is stepping at less than stoppingStepLength
  //++++can't we change this to use delta like the others?? --pls

{
    return ( (maxCalls != NO_MAX && functionCalls >= maxCalls)
        || (step < stoppingStepLength) );   
}//Stop

double EdHJSearch::GetDelta()
{
  return step;
}





