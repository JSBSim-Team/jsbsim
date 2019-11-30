/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGFunction.cpp
Author: Jon Berndt
Date started: 8/25/2004
Purpose: Stores various parameter types for functions

 ------------- Copyright (C) 2004  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iomanip>
#include <random>
#include <chrono>
#include <memory>

#include "simgear/misc/strutils.hxx"
#include "FGFDMExec.h"
#include "FGFunction.h"
#include "FGTable.h"
#include "FGRealValue.h"
#include "input_output/FGXMLElement.h"
#include "math/FGFunctionValue.h"


using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

const double invlog2val = 1.0/log10(2.0);
constexpr unsigned int MaxArgs = 9999;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

class WrongNumberOfArguments : public runtime_error
{
public:
  WrongNumberOfArguments(const string &msg, const vector<FGParameter_ptr> &p,
                         Element* el)
    : runtime_error(msg), Parameters(p), element(el) {}
  size_t NumberOfArguments(void) const { return Parameters.size(); }
  FGParameter* FirstParameter(void) const { return *(Parameters.cbegin()); }
  const Element* GetElement(void) const { return element; }

private:
  const vector<FGParameter_ptr> Parameters;
  const Element* element;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename func_t, unsigned int Nmin>
class aFunc: public FGFunction
{
public:
  aFunc(const func_t& _f, FGFDMExec* fdmex, Element* el,
        const string& prefix, FGPropertyValue* v, unsigned int Nmax=Nmin,
        FGFunction::OddEven odd_even=FGFunction::OddEven::Either)
    : FGFunction(fdmex->GetPropertyManager()), f(_f)
  {
    Load(el, v, fdmex, prefix);
    CheckMinArguments(el, Nmin);
    CheckMaxArguments(el, Nmax);
    CheckOddOrEvenArguments(el, odd_even);
  }

  double GetValue(void) const override {
    return cached ? cachedValue : f(Parameters);
  }

protected:
  void bind(Element* el, const string& Prefix) override {
    string nName = CreateOutputNode(el, Prefix);
    if (!nName.empty())
      PropertyManager->Tie(nName, this, &aFunc<func_t, Nmin>::GetValue);
  }

private:
  const func_t f;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Template specialization for functions without parameters.

template<typename func_t>
class aFunc<func_t, 0>: public FGFunction
{
public:
  aFunc(const func_t& _f, FGPropertyManager* pm, Element* el,
        const string& Prefix)
    : FGFunction(pm), f(_f)
  {
    if (el->GetNumElements() != 0) {
      ostringstream buffer;
      buffer << el->ReadFrom() << fgred << highint
             << "<" << el->GetName() << "> should have no arguments." << reset
             << endl;
      throw WrongNumberOfArguments(buffer.str(), Parameters, el);
    }

    bind(el, Prefix);
  }

  double GetValue(void) const override {
    double result = cached ? cachedValue : f();
    if (pNode) pNode->setDoubleValue(result);
    return result;
  }

  // Functions without parameters are assumed to be non-const
  bool IsConstant(void) const override {
    return false;
  }

protected:
  // The method GetValue() is not bound for functions without parameters because
  // we do not want the property to return a different value each time it is
  // read.
  void bind(Element* el, const string& Prefix) override {
    CreateOutputNode(el, Prefix);
    // Initialize the node to a sensible value.
    if (pNode) pNode->setDoubleValue(f());
  }

private:
  const func_t f;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool GetBinary(double val, const string &ctxMsg)
{
  val = fabs(val);
  if (val < 1E-9) return false;
  else if (val-1 < 1E-9) return true;
  else {
    cerr << ctxMsg << FGJSBBase::fgred << FGJSBBase::highint
         << "Malformed conditional check in function definition."
         << FGJSBBase::reset << endl;
    throw("Fatal Error.");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Hides the machinery to create a class for functions from <math.h> such as
// sin, cos, exp, etc.

FGFunction* make_MathFn(double(*math_fn)(double), FGFDMExec* fdmex, Element* el,
                        const string& prefix, FGPropertyValue* v)
{
  auto f = [math_fn](const std::vector<FGParameter_ptr> &p)->double {
             return math_fn(p[0]->GetValue());
           };
  return new aFunc<decltype(f), 1>(f, fdmex, el, prefix, v);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Manage the functions with a variable number of arguments.
// It handles the special case where a single argument is provided to the
// function: in that case the function is ignored and replaced by its argument.

template<typename func_t>
FGParameter_ptr VarArgsFn(const func_t& _f, FGFDMExec* fdmex, Element* el,
                          const string& prefix, FGPropertyValue* v)
{
  try {
    return new aFunc<func_t, 2>(_f, fdmex, el, prefix, v, MaxArgs);
  }
  catch(WrongNumberOfArguments& e) {
    if ((e.GetElement() == el) && (e.NumberOfArguments() == 1)) {
      cerr << el->ReadFrom() << FGJSBBase::fgred
           << "<" << el->GetName()
           << "> only has one argument which makes it a no-op." << endl
           << "Its argument will be evaluated but <" << el->GetName()
           << "> will not be applied to the result." << FGJSBBase::reset << endl;
      return e.FirstParameter();
    }
    else
      throw e.what();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFunction::FGFunction(FGFDMExec* fdmex, Element* el, const string& prefix,
                       FGPropertyValue* var)
  : FGFunction(fdmex->GetPropertyManager())
{
  Load(el, var, fdmex, prefix);
  CheckMinArguments(el, 1);
  CheckMaxArguments(el, 1);

  string sCopyTo = el->GetAttributeValue("copyto");

  if (!sCopyTo.empty()) {
    if (sCopyTo.find("#") != string::npos) {
      if (is_number(prefix))
        sCopyTo = replace(sCopyTo,"#",prefix);
      else {
        cerr << el->ReadFrom() << fgred
             << "Illegal use of the special character '#'" << reset << endl
             << "The 'copyto' argument in function " << Name << " is ignored."
             << endl;
        return;
      }
    }

    pCopyTo = PropertyManager->GetNode(sCopyTo);
    if (!pCopyTo)
      cerr << el->ReadFrom() << fgred
           << "Property \"" << sCopyTo
           << "\" must be previously defined in function " << Name << reset
           << "The 'copyto' argument is ignored." << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::CheckMinArguments(Element* el, unsigned int _min)
{
  if (Parameters.size() < _min) {
    ostringstream buffer;
    buffer << el->ReadFrom() << fgred << highint
           << "<" << el->GetName() << "> should have at least " << _min
           << " argument(s)." << reset << endl;
    throw WrongNumberOfArguments(buffer.str(), Parameters, el);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::CheckMaxArguments(Element* el, unsigned int _max)
{
  if (Parameters.size() > _max) {
    ostringstream buffer;
    buffer << el->ReadFrom() << fgred << highint
           << "<" << el->GetName() << "> should have no more than " << _max
           << " argument(s)." << reset << endl;
    throw WrongNumberOfArguments(buffer.str(), Parameters, el);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::CheckOddOrEvenArguments(Element* el, OddEven odd_even)
{

  switch(odd_even) {
  case OddEven::Even:
    if (Parameters.size() % 2 == 1) {
      cerr << el->ReadFrom() << fgred << highint
           << "<" << el->GetName() << "> must have an even number of arguments."
           << reset << endl;
      throw("Fatal Error");
    }
    break;
  case OddEven::Odd:
    if (Parameters.size() % 2 == 0) {
      cerr << el->ReadFrom() << fgred << highint
           << "<" << el->GetName() << "> must have an odd number of arguments."
           << reset << endl;
      throw("Fatal Error");
    }
    break;
  default:
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

shared_ptr<default_random_engine> makeRandomEngine(Element *el, FGFDMExec* fdmex)
{
  string seed_attr = el->GetAttributeValue("seed");
  unsigned int seed;
  if (seed_attr.empty())
    return fdmex->GetRandomEngine();
  else if (seed_attr == "time_now")
    seed = chrono::system_clock::now().time_since_epoch().count();
  else
    seed = atoi(seed_attr.c_str());
  return make_shared<default_random_engine>(seed);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::Load(Element* el, FGPropertyValue* var, FGFDMExec* fdmex,
                      const string& Prefix)
{
  Name = el->GetAttributeValue("name");
  Element* element = el->GetElement();
      
  auto sum = [](const decltype(Parameters)& Parameters)->double {
               double temp = 0.0;

               for (auto p: Parameters)
                 temp += p->GetValue();

               return temp;
             };
  
  while (element) {
    string operation = element->GetName();

    // data types
    if (operation == "property" || operation == "p") {
      string property_name = element->GetDataLine();

      if (var && simgear::strutils::strip(property_name) == "#")
        Parameters.push_back(var);
      else {
        if (property_name.find("#") != string::npos) {
          if (is_number(Prefix)) {
            property_name = replace(property_name,"#",Prefix);
          }
          else {
            cerr << element->ReadFrom()
                 << fgred << "Illegal use of the special character '#'"
                 << reset << endl;
            throw("Fatal Error.");
          }
        }

        if (element->HasAttribute("apply")) {
          string function_str = element->GetAttributeValue("apply");
          FGTemplateFunc* f = fdmex->GetTemplateFunc(function_str);
          if (f)
            Parameters.push_back(new FGFunctionValue(property_name,
                                                     PropertyManager, f));
          else {
            cerr << element->ReadFrom()
                 << fgred << highint << "  No function by the name "
                 << function_str << " has been defined. This property will "
                 << "not be logged. You should check your configuration file."
                 << reset << endl;
          }
        }
        else
          Parameters.push_back(new FGPropertyValue(property_name,
                                                   PropertyManager));
      }
    } else if (operation == "value" || operation == "v") {
      Parameters.push_back(new FGRealValue(element->GetDataAsNumber()));
    } else if (operation == "pi") {
      Parameters.push_back(new FGRealValue(M_PI));
    } else if (operation == "table" || operation == "t") {
      Parameters.push_back(new FGTable(PropertyManager, element, Prefix));
      // operations
    } else if (operation == "product") {
      auto f = [](const decltype(Parameters)& Parameters)->double {
                 double temp = 1.0;

                 for (auto p: Parameters)
                   temp *= p->GetValue();

                 return temp;
               };
      Parameters.push_back(VarArgsFn<decltype(f)>(f, fdmex, element, Prefix, var));
    } else if (operation == "sum") {
      Parameters.push_back(VarArgsFn<decltype(sum)>(sum, fdmex, element, Prefix, var));
    } else if (operation == "avg") {
      auto avg = [&](const decltype(Parameters)& p)->double {
                   return sum(p) / p.size();
                 };
      Parameters.push_back(VarArgsFn<decltype(avg)>(avg, fdmex, element, Prefix, var));
    } else if (operation == "difference") {
      auto f = [](const decltype(Parameters)& Parameters)->double {
                 double temp = Parameters[0]->GetValue();

                 for (auto p = Parameters.begin()+1; p != Parameters.end(); ++p)
                   temp -= (*p)->GetValue();

                 return temp;
               };
      Parameters.push_back(VarArgsFn<decltype(f)>(f, fdmex, element, Prefix, var));
    } else if (operation == "min") {
      auto f = [](const decltype(Parameters)& Parameters)->double {
                 double _min = HUGE_VAL;

                 for (auto p : Parameters) {
                   double x = p->GetValue();
                   if (x < _min)
                     _min = x;
                 }

                 return _min;
               };
      Parameters.push_back(VarArgsFn<decltype(f)>(f, fdmex, element, Prefix, var));
    } else if (operation == "max") {
      auto f = [](const decltype(Parameters)& Parameters)->double {
                 double _max = -HUGE_VAL;

                 for (auto p : Parameters) {
                   double x = p->GetValue();
                   if (x > _max)
                     _max = x;
                 }

                 return _max;
               };
      Parameters.push_back(VarArgsFn<decltype(f)>(f, fdmex, element, Prefix, var));
    } else if (operation == "and") {
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& Parameters)->double {
                 for (auto p : Parameters) {
                   if (!GetBinary(p->GetValue(), ctxMsg)) // As soon as one parameter is false, the expression is guaranteed to be false.
                     return 0.0;
                 }

                 return 1.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix,
                                                     var, MaxArgs));
    } else if (operation == "or") {
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& Parameters)->double {
                 for (auto p : Parameters) {
                   if (GetBinary(p->GetValue(), ctxMsg)) // As soon as one parameter is true, the expression is guaranteed to be true.
                     return 1.0;
                 }

                 return 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix,
                                                     var, MaxArgs));
    } else if (operation == "quotient") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double y = p[1]->GetValue();
                 return y != 0.0 ? p[0]->GetValue()/y : HUGE_VAL;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "pow") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return pow(p[0]->GetValue(), p[1]->GetValue());
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "toradians") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue()*M_PI/180.;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "todegrees") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue()*180./M_PI;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "sqrt") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double x = p[0]->GetValue();
                 return x >= 0.0 ? sqrt(x) : -HUGE_VAL;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "log2") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double x = p[0]->GetValue();
                 return x > 0.0 ? log10(x)*invlog2val : -HUGE_VAL;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "ln") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double x = p[0]->GetValue();
                 return x > 0.0 ? log(x) : -HUGE_VAL;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "log10") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double x = p[0]->GetValue();
                 return x > 0.0 ? log10(x) : -HUGE_VAL;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "sign") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() < 0.0 ? -1 : 1; // 0.0 counts as positive.
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "exp") {
      Parameters.push_back(make_MathFn(exp, fdmex, element, Prefix, var));
    } else if (operation == "abs") {
      Parameters.push_back(make_MathFn(fabs, fdmex, element, Prefix, var));
    } else if (operation == "sin") {
      Parameters.push_back(make_MathFn(sin, fdmex, element, Prefix, var));
    } else if (operation == "cos") {
      Parameters.push_back(make_MathFn(cos, fdmex, element, Prefix, var));
    } else if (operation == "tan") {
      Parameters.push_back(make_MathFn(tan, fdmex, element, Prefix, var));
    } else if (operation == "asin") {
      Parameters.push_back(make_MathFn(asin, fdmex, element, Prefix, var));
    } else if (operation == "acos") {
      Parameters.push_back(make_MathFn(acos, fdmex, element, Prefix, var));
    } else if (operation == "atan") {
      Parameters.push_back(make_MathFn(atan, fdmex, element, Prefix, var));
    } else if (operation == "floor") {
      Parameters.push_back(make_MathFn(floor, fdmex, element, Prefix, var));
    } else if (operation == "ceil") {
      Parameters.push_back(make_MathFn(ceil, fdmex, element, Prefix, var));
    } else if (operation == "fmod") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double y = p[1]->GetValue();
                 return y != 0.0 ? fmod(p[0]->GetValue(), y) : HUGE_VAL;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "atan2") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return atan2(p[0]->GetValue(), p[1]->GetValue());
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "mod") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return static_cast<int>(p[0]->GetValue()) % static_cast<int>(p[1]->GetValue());
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "fraction") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double scratch;
                 return modf(p[0]->GetValue(), &scratch);
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "integer") {
      auto f = [](const decltype(Parameters)& p)->double {
                 double result;
                 modf(p[0]->GetValue(), &result);
                 return result;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "lt") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() < p[1]->GetValue() ? 1.0 : 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "le") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() <= p[1]->GetValue() ? 1.0 : 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "gt") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() > p[1]->GetValue() ? 1.0 : 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "ge") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() >= p[1]->GetValue() ? 1.0 : 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "eq") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() == p[1]->GetValue() ? 1.0 : 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "nq") {
      auto f = [](const decltype(Parameters)& p)->double {
                 return p[0]->GetValue() != p[1]->GetValue() ? 1.0 : 0.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix, var));
    } else if (operation == "not") {
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& p)->double {
                 return GetBinary(p[0]->GetValue(), ctxMsg) ? 0.0 : 1.0;
               };
      Parameters.push_back(new aFunc<decltype(f), 1>(f, fdmex, element, Prefix, var));
    } else if (operation == "ifthen") {
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& p)->double {
                 if (GetBinary(p[0]->GetValue(), ctxMsg))
                   return p[1]->GetValue();
                 else
                   return p[2]->GetValue();
               };
      Parameters.push_back(new aFunc<decltype(f), 3>(f, fdmex, element, Prefix, var));
    } else if (operation == "random") {
      double mean = 0.0;
      double stddev = 1.0;
      string mean_attr = element->GetAttributeValue("mean");
      string stddev_attr = element->GetAttributeValue("stddev");
      if (!mean_attr.empty())
        mean = atof(mean_attr.c_str());
      if (!stddev_attr.empty())
        stddev = atof(stddev_attr.c_str());
      auto distribution = make_shared<normal_distribution<double>>(mean, stddev);
      auto generator(makeRandomEngine(element, fdmex));
      auto f = [generator, distribution]()->double {
                 return (*distribution.get())(*generator);
               };
      Parameters.push_back(new aFunc<decltype(f), 0>(f, PropertyManager, element,
                                                     Prefix));
    } else if (operation == "urandom") {
      double lower = -1.0;
      double upper = 1.0;
      string lower_attr = element->GetAttributeValue("lower");
      string upper_attr = element->GetAttributeValue("upper");
      if (!lower_attr.empty())
        lower = atof(lower_attr.c_str());
      if (!upper_attr.empty())
        upper = atof(upper_attr.c_str());
      auto distribution = make_shared<uniform_real_distribution<double>>(lower, upper);
      auto generator(makeRandomEngine(element, fdmex));
      auto f = [generator, distribution]()->double {
                 return (*distribution.get())(*generator);
               };
      Parameters.push_back(new aFunc<decltype(f), 0>(f, PropertyManager, element,
                                                     Prefix));
    } else if (operation == "switch") {
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& p)->double {
                 double temp = p[0]->GetValue();
                 if (temp < 0.0) {
                   cerr << ctxMsg << fgred << highint
                        << "The switch function index (" << temp
                        << ") is negative." << reset << endl;
                   throw("Fatal error");
                 }
                 size_t n = p.size()-1;
                 size_t i = static_cast<size_t>(temp+0.5);

                 if (i < n)
                   return p[i+1]->GetValue();
                 else {
                   cerr << ctxMsg << fgred << highint
                        << "The switch function index (" << temp
                        << ") selected a value above the range of supplied values"
                        << "[0:" << n-1 << "]"
                        << " - not enough values were supplied." << reset << endl;
                   throw("Fatal error");
                 }
               };
      Parameters.push_back(new aFunc<decltype(f), 2>(f, fdmex, element, Prefix,
                                                     var, MaxArgs));
    } else if (operation == "interpolate1d") {
      auto f = [](const decltype(Parameters)& p)->double {
                 // This is using the bisection algorithm. Special care has been
                 // taken to evaluate each parameter only once.
                 size_t n = p.size();
                 double x = p[0]->GetValue();
                 double xmin = p[1]->GetValue();
                 double ymin = p[2]->GetValue();
                 if (x <= xmin) return ymin;

                 double xmax = p[n-2]->GetValue();
                 double ymax = p[n-1]->GetValue();
                 if (x >= xmax) return ymax;

                 size_t nmin = 0;
                 size_t nmax = (n-3)/2;
                 while (nmax-nmin > 1) {
                   size_t m = (nmax-nmin)/2+nmin;
                   double xm = p[2*m+1]->GetValue();
                   double ym = p[2*m+2]->GetValue();
                   if (x < xm) {
                     xmax = xm;
                     ymax = ym;
                     nmax= m;
                   } else if (x > xm) {
                     xmin = xm;
                     ymin = ym;
                     nmin = m;
                   }
                   else
                     return ym;
                 }

                 return ymin + (x-xmin)*(ymax-ymin)/(xmax-xmin);
               };
      Parameters.push_back(new aFunc<decltype(f), 5>(f, fdmex, element, Prefix,
                                                     var, MaxArgs, OddEven::Odd));
    } else if (operation == "rotation_alpha_local") {
      // Calculates local angle of attack for skydiver body component.
      // Euler angles from the intermediate body frame to the local body frame
      // must be from a z-y-x axis rotation order
      auto f = [](const decltype(Parameters)& p)->double {
                 double alpha = p[0]->GetValue()*degtorad; //angle of attack of intermediate body frame
                 double beta = p[1]->GetValue()*degtorad;  //sideslip angle of intermediate body frame
                 double phi = p[3]->GetValue()*degtorad;   //x-axis Euler angle from the intermediate body frame to the local body frame
                 double theta = p[4]->GetValue()*degtorad; //y-axis Euler angle from the intermediate body frame to the local body frame
                 double psi = p[5]->GetValue()*degtorad;   //z-axis Euler angle from the intermediate body frame to the local body frame

                 FGQuaternion qTb2l(phi, theta, psi);
                 double cos_beta = cos(beta);
                 FGColumnVector3 wind_body(cos(alpha)*cos_beta, sin(beta),
                                           sin(alpha)*cos_beta);
                 FGColumnVector3 wind_local = qTb2l.GetT()*wind_body;

                 if (fabs(fabs(wind_local(eY)) - 1.0) < 1E-9)
                   return 0.0;
                 else
                   return atan2(wind_local(eZ), wind_local(eX))*radtodeg;
               };
      Parameters.push_back(new aFunc<decltype(f), 6>(f, fdmex, element, Prefix, var));
    } else if (operation == "rotation_beta_local") {
      // Calculates local angle of sideslip for skydiver body component.
      // Euler angles from the intermediate body frame to the local body frame
      // must be from a z-y-x axis rotation order
      auto f = [](const decltype(Parameters)& p)->double {
                 double alpha = p[0]->GetValue()*degtorad; //angle of attack of intermediate body frame
                 double beta = p[1]->GetValue()*degtorad;  //sideslip angle of intermediate body frame
                 double phi = p[3]->GetValue()*degtorad;   //x-axis Euler angle from the intermediate body frame to the local body frame
                 double theta = p[4]->GetValue()*degtorad; //y-axis Euler angle from the intermediate body frame to the local body frame
                 double psi = p[5]->GetValue()*degtorad;   //z-axis Euler angle from the intermediate body frame to the local body frame
                 FGQuaternion qTb2l(phi, theta, psi);
                 double cos_beta = cos(beta);
                 FGColumnVector3 wind_body(cos(alpha)*cos_beta, sin(beta),
                                           sin(alpha)*cos_beta);
                 FGColumnVector3 wind_local = qTb2l.GetT()*wind_body;

                 if (fabs(fabs(wind_local(eY)) - 1.0) < 1E-9)
                   return wind_local(eY) > 0.0 ? 0.5*M_PI : -0.5*M_PI;

                 double alpha_local = atan2(wind_local(eZ), wind_local(eX));
                 double cosa = cos(alpha_local);
                 double sina = sin(alpha_local);
                 double cosb;

                 if (fabs(cosa) > fabs(sina)) 
                   cosb = wind_local(eX) / cosa;
                 else
                   cosb = wind_local(eZ) / sina;  

                 return atan2(wind_local(eY), cosb)*radtodeg;
               };
      Parameters.push_back(new aFunc<decltype(f), 6>(f, fdmex, element, Prefix, var));
    } else if (operation == "rotation_gamma_local") {
      // Calculates local roll angle for skydiver body component.
      // Euler angles from the intermediate body frame to the local body frame
      // must be from a z-y-x axis rotation order
      auto f = [](const decltype(Parameters)& p)->double {
                 double alpha = p[0]->GetValue()*degtorad; //angle of attack of intermediate body frame
                 double beta = p[1]->GetValue()*degtorad;  //sideslip angle of intermediate body frame
                 double gamma = p[2]->GetValue()*degtorad; //roll angle of intermediate body frame
                 double phi = p[3]->GetValue()*degtorad;   //x-axis Euler angle from the intermediate body frame to the local body frame
                 double theta = p[4]->GetValue()*degtorad; //y-axis Euler angle from the intermediate body frame to the local body frame
                 double psi = p[5]->GetValue()*degtorad;   //z-axis Euler angle from the intermediate body frame to the local body frame
                 double cos_alpha = cos(alpha), sin_alpha = sin(alpha);
                 double cos_beta = cos(beta),   sin_beta = sin(beta);
                 double cos_gamma = cos(gamma), sin_gamma = sin(gamma);
                 FGQuaternion qTb2l(phi, theta, psi);
                 FGColumnVector3 wind_body_X(cos_alpha*cos_beta, sin_beta,
                                             sin_alpha*cos_beta);
                 FGColumnVector3 wind_body_Y(-sin_alpha*sin_gamma-sin_beta*cos_alpha*cos_gamma,
                                             cos_beta*cos_gamma,
                                             -sin_alpha*sin_beta*cos_gamma+sin_gamma*cos_alpha);
                 FGColumnVector3 wind_local_X = qTb2l.GetT()*wind_body_X;
                 FGColumnVector3 wind_local_Y = qTb2l.GetT()*wind_body_Y;
                 double cosacosb = wind_local_X(eX);
                 double sinb = wind_local_X(eY);
                 double sinacosb = wind_local_X(eZ);
                 double sinc, cosc;

                 if (fabs(sinb) < 1E-9) { // cos(beta_local) == 1.0
                   cosc = wind_local_Y(eY);

                   if (fabs(cosacosb) > fabs(sinacosb))
                     sinc = wind_local_Y(eZ) / cosacosb;
                   else
                     sinc = -wind_local_Y(eX) / sinacosb;
                 }
                 else if (fabs(fabs(sinb)-1.0) < 1E-9) { // cos(beta_local) == 0.0
                   sinc = wind_local_Y(eZ);
                   cosc = -wind_local_Y(eX);
                 }
                 else {
                   sinc = cosacosb*wind_local_Y(eZ)-sinacosb*wind_local_Y(eX);
                   cosc = (-sinacosb*wind_local_Y(eZ)-cosacosb*wind_local_Y(eX))/sinb;
                 }

                 return atan2(sinc, cosc)*radtodeg;
               };
      Parameters.push_back(new aFunc<decltype(f), 6>(f, fdmex, element, Prefix, var));
    } else if (operation == "rotation_bf_to_wf") {
      // Transforms the input vector from a body frame to a wind frame. The
      // origin of the vector remains the same.
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& p)->double {
                 double rx = p[0]->GetValue();             //x component of input vector
                 double ry = p[1]->GetValue();             //y component of input vector
                 double rz = p[2]->GetValue();             //z component of input vector
                 double alpha = p[3]->GetValue()*degtorad; //angle of attack of the body frame
                 double beta = p[4]->GetValue()*degtorad;  //sideslip angle of the body frame
                 double gamma = p[5]->GetValue()*degtorad; //roll angle of the body frame
                 int idx = static_cast<int>(p[6]->GetValue());

                 if ((idx < 1) || (idx > 3)) {
                   cerr << ctxMsg << fgred << highint
                        << "The index must be one of the integer value 1, 2 or 3."
                        << reset << endl;
                   throw("Fatal error");
                 }

                 FGQuaternion qa(eY, -alpha), qb(eZ, beta), qc(eX, -gamma);
                 FGMatrix33 mT = (qa*qb*qc).GetT();
                 FGColumnVector3 r0(rx, ry, rz);
                 FGColumnVector3 r = mT*r0;

                 return r(idx);
               };
      Parameters.push_back(new aFunc<decltype(f), 7>(f, fdmex, element, Prefix, var));
    } else if (operation == "rotation_wf_to_bf") {
      // Transforms the input vector from q wind frame to a body frame. The
      // origin of the vector remains the same.
      string ctxMsg = element->ReadFrom();
      auto f = [ctxMsg](const decltype(Parameters)& p)->double {
                 double rx = p[0]->GetValue();             //x component of input vector
                 double ry = p[1]->GetValue();             //y component of input vector
                 double rz = p[2]->GetValue();             //z component of input vector
                 double alpha = p[3]->GetValue()*degtorad; //angle of attack of the body frame
                 double beta = p[4]->GetValue()*degtorad;  //sideslip angle of the body frame
                 double gamma = p[5]->GetValue()*degtorad; //roll angle of the body frame
                 int idx = static_cast<int>(p[6]->GetValue());

                 if ((idx < 1) || (idx > 3)) {
                   cerr << ctxMsg << fgred << highint
                        << "The index must be one of the integer value 1, 2 or 3."
                        << reset << endl;
                   throw("Fatal error");
                 }

                 FGQuaternion qa(eY, -alpha), qb(eZ, beta), qc(eX, -gamma);
                 FGMatrix33 mT = (qa*qb*qc).GetT();
                 FGColumnVector3 r0(rx, ry, rz);
                 mT.T();
                 FGColumnVector3 r = mT*r0;

                 return r(idx);
               };
      Parameters.push_back(new aFunc<decltype(f), 7>(f, fdmex, element, Prefix, var));
    } else if (operation != "description") {
      cerr << element->ReadFrom() << fgred << highint
           << "Bad operation <" << operation
           << "> detected in configuration file" << reset << endl;
    }

    // Optimize functions applied on constant parameters by replacing them by
    // their constant result.
    if (!Parameters.empty()){
      FGFunction* p = dynamic_cast<FGFunction*>(Parameters.back().ptr());

      if (p && p->IsConstant()) {
        double constant = p->GetValue();
        FGPropertyNode_ptr node = p->pNode;
        string pName = p->GetName();

        Parameters.pop_back();
        Parameters.push_back(new FGRealValue(constant));
        if (debug_lvl > 0)
          cout << element->ReadFrom() << fggreen << highint
               << "<" << operation << "> is applied on constant parameters."
               << endl << "It will be replaced by its result ("
               << constant << ")";

        if (node) {
          node->setDoubleValue(constant);
          node->setAttribute(SGPropertyNode::WRITE, false);
          if (debug_lvl > 0)
            cout << " and the property " << pName
                 << " will be unbound and made read only.";
        }
        cout << reset << endl << endl;
      }
    }
    element = el->GetNextElement();
  }

  bind(el, Prefix); // Allow any function to save its value

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFunction::~FGFunction()
{
  if (pNode && pNode->isTied()) {
    string pName = pNode->GetFullyQualifiedName();
    PropertyManager->Untie(pName);
  }

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFunction::IsConstant(void) const
{
  for (auto p: Parameters) {
    if (!p->IsConstant())
      return false;
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::cacheValue(bool cache)
{
  cached = false; // Must set cached to false prior to calling GetValue(), else
                  // it will _never_ calculate the value;
  if (cache) {
    cachedValue = GetValue();
    cached = true;
  }
}
  
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFunction::GetValue(void) const
{
  if (cached) return cachedValue;

  double val = Parameters[0]->GetValue();

  if (pCopyTo) pCopyTo->setDoubleValue(val);

  return val;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFunction::GetValueAsString(void) const
{
  ostringstream buffer;

  buffer << setw(9) << setprecision(6) << GetValue();
  return buffer.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFunction::CreateOutputNode(Element* el, const string& Prefix)
{
  string nName;

  if ( !Name.empty() ) {
    if (Prefix.empty())
      nName  = PropertyManager->mkPropertyName(Name, false);
    else {
      if (is_number(Prefix)) {
        if (Name.find("#") != string::npos) { // if "#" is found
          Name = replace(Name,"#",Prefix);
          nName  = PropertyManager->mkPropertyName(Name, false);
        } else {
          cerr << el->ReadFrom()
               << "Malformed function name with number: " << Prefix
               << " and property name: " << Name
               << " but no \"#\" sign for substitution." << endl;
        }
      } else {
        nName  = PropertyManager->mkPropertyName(Prefix + "/" + Name, false);
      }
    }

    pNode = PropertyManager->GetNode(nName, true);
    if (pNode->isTied()) {
      cerr << el->ReadFrom()
           << "Property " << nName << " has already been successfully bound (late)." << endl;
      throw("Failed to bind the property to an existing already tied node.");
    }
  }

  return nName;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFunction::bind(Element* el, const string& Prefix)
{
  string nName = CreateOutputNode(el, Prefix);

  if (!nName.empty())
    PropertyManager->Tie(nName, this, &FGFunction::GetValue);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGFunction::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      if (!Name.empty())
        cout << "    Function: " << Name << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFunction" << endl;
    if (from == 1) cout << "Destroyed:    FGFunction" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}

}
