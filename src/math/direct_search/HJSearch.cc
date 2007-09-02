/*  HJSearch.cc
 *  implementation of the HJSearch class to find a minimal objective
 *  function solution.
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

#include <math/direct_search/HJSearch.h>


//+++++++++++++change these steps to deltas?  I don't see why we can't --
//  have a look at it.  then we can change the stop() funciton as well.
//  same for EdHJSearch stuff.  +++++++++    -pls 6/12
HJSearch::HJSearch(long numberOfVariables, Vector<double> &startPoint)
  : PatternSearch(numberOfVariables, startPoint)
{
  step = initialStepLength;  //as provided by the user or default
  factor = 0.5;              //factor by which the step length is reduced
  IDnumber = 2300;    
}

HJSearch::HJSearch(const HJSearch & Original): PatternSearch(Original)
{
  step = Original.step;
  factor = Original.factor;    //factor by which the step length is reduced
  IDnumber = 2300;    
}

HJSearch::HJSearch(long dim, 
		   Vector<double> &startPoint,
		   double startStep,
		   double stopStep,
		   void (*objective)(long vars, Vector<double> &x, 
				     double & func, bool& flag, void* an_obj),
		   void * input_obj)
  : PatternSearch(dim, startPoint, startStep, stopStep,
		 objective, input_obj)
{
  step = initialStepLength;  //as provided by the user or default
  factor = 0.5;              //factor by which the step length is reduced
  IDnumber = 2300;    
} // special constructor for use with MAPS

 
HJSearch::~HJSearch()
{
}


HJSearch& HJSearch::operator=(const HJSearch& A)
{
  CopySearch(A);
  return(*this);
}
void HJSearch::CleanSlate(long dimensions, Vector<double> &startPoint)
//reinitialize all values
{
  PatternSearch::CleanSlate(dimensions, startPoint);
  step = initialStepLength;
}

//reinitialize all values--overloaded for use with MAPS
void HJSearch::CleanSlate(long dim, 
			  Vector<double> &startPoint,
			  double startStep,
			  double stopStep,
			  void (*objective)(long vars, Vector<double>&x,
                                            double & func, 
					    bool& flag, void* an_obj),
			  void * input_obj)
{
  PatternSearch::CleanSlate(dim, startPoint, startStep, stopStep,
                            objective, input_obj);
  step = initialStepLength;
}


void HJSearch::BeginSearch()
{
  ExploratoryMoves();
} //BeginHJSearch

void HJSearch::ExploratoryMoves()
{
  long dimens = GetDimension();
  Vector<double> currentPoint(dimens);
  Vector<double> lastImprovingPoint(dimens); //last base point
  Vector<double> storage(dimens); //for intermediate storage to reduce rounding error
  Vector<double> direction(dimens, 0.0); //direction of pattern extended step
  double value;                   //objective function value
  double positiveValue;           //obj.fun. value in the positive step
  double negativeValue;           // " for the negative step
  double lastImprovingValue;      //obj.fun.value of last base point
  bool success = false;
  bool flag = false;
  fcn_name(dimension, (*minPoint), minValue, flag, some_object);
  if (!flag)
  cerr << "\nError signal in objective function at starting point.\n";
  GetMinVal(value);
  GetMinVal(lastImprovingValue);
  GetMinPoint(currentPoint);      //initialize to the user initial point
  GetMinPoint(lastImprovingPoint);
  GetMinPoint(storage);
  bool foundImprove = false;
  long i = 0;
  do
    {
      for(i=0; i < dimens; i++)
	{  
	  currentPoint[i] += step; 
          if(BreakOnExact()) break;
	  fcnCall(dimens, currentPoint, positiveValue, success, some_object);
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
	      currentPoint[i] -= (step);
              if(BreakOnExact()) break;  
	      fcnCall(dimens, currentPoint, negativeValue, success, some_object);
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
	  foundImprove = false;      //reset for next iteration
	}//for
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
	  else   //some step yielded improvement
	    {
	      lastImprovingValue = value;
	      ReplaceMinimum(currentPoint, value);
	      lastImprovingPoint = currentPoint;

	      //take the pattern extending step and find its value
	      currentPoint = direction + currentPoint;
	      storage = currentPoint;
              if (BreakOnExact()) break;    
	      fcnCall(dimens, currentPoint, value, success, some_object);
	    }	  
	}
      else
	{
	  if(isnear(currentPoint, lastImprovingPoint, factor*step))
	    {
                //I added this so we would stop after a pattern-reducing
                // step if it's time to stop. --pla 7/00
                if (Stop()) {
                    break;
                }
	      step = step * factor;
	    }
	  else
	    {
	      //this case can only occur after an unsuccessful
	      //search about a pattern-step-located point.
	      //move back to the point that was improving from the
	      //search about the last base point
	      GetMinPoint(currentPoint);
	      GetMinVal(value);
	      storage = currentPoint;
	    }
	}
    } while(!Stop());//while we haven't stopped()
  //But does this end only after a shrinking step??
}//ExploratoryMoves

void HJSearch::CopySearch(const HJSearch & Original)
{
  PatternSearch::CopySearch(Original);
  step = Original.step;
  factor = Original.factor;
}

bool HJSearch::Stop()
//Makes certain search has not exceeded maxCalls or 
//is stepping at less than stoppingStepLength

{
    return ( (maxCalls != NO_MAX && functionCalls >= maxCalls)
             || (step < stoppingStepLength) );   
}//Stop

double HJSearch::GetDelta()
{
  return step;
}


