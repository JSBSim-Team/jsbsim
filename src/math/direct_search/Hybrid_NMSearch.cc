/** Hybrid_NMSearch.cc

    Implementaion file for class Hybrid_NMSearch, a hybrid
    Nelder-Mead/EdHJ search.

    Anne Shepherd, College of William and Mary, 2000

*/

#include <math/direct_search/Hybrid_NMSearch.h>

    
Hybrid_NMSearch::Hybrid_NMSearch(long dim, Vector<double> &startPoint )
    :NMSearch(dim, startPoint)
{
    ESearchStoplength = 10e-8;
    NSearchCalls = 0;
    ESearchCalls = 0;
    TotalCalls = 0;
    /*  Note that Stop_on_std must be set to true here; otherwise, you
     *  end up with just an NMSearch that terminates based on delta.
     */
    Stop_on_std = true;
    IDnumber = 3210;
}
Hybrid_NMSearch::Hybrid_NMSearch(long dim, Vector<double> &startPoint, 
                                  double sig, Vector<double> &lengths)
    :NMSearch(dim, startPoint, sig, lengths)
{
    ESearchStoplength = 10e-8;
    NSearchCalls = 0;
    ESearchCalls = 0;
    TotalCalls = 0;
    /*  Note that Stop_on_std must be set to true here; otherwise, you
     *  end up with just an NMSearch that terminates based on delta.
     */
    Stop_on_std = true;
    IDnumber = 3210;
}
    
Hybrid_NMSearch::Hybrid_NMSearch(long dim, Vector<double> &startPoint,
                                 double NewSigma, double NewAlpha, 
                                 double NewBeta, double NewGamma)
    :NMSearch(dim, startPoint, NewSigma, NewAlpha, NewBeta, NewGamma)
{
    ESearchStoplength = 10e-8;
    NSearchCalls = 0;
    ESearchCalls = 0;
    TotalCalls = 0;
    /*  Note that Stop_on_std must be set to true here; otherwise, you
     *  end up with just an NMSearch that terminates based on delta.
     */
    Stop_on_std = true;
    IDnumber = 3210;
}
    
Hybrid_NMSearch::Hybrid_NMSearch(const Hybrid_NMSearch& Original)
    :NMSearch(Original)
{
    stoppingStepLength = Original.stoppingStepLength;
    ESearchStoplength = Original.ESearchStoplength ;
    NSearchCalls =  Original.NSearchCalls;
    ESearchCalls =  Original.ESearchCalls;
    TotalCalls =  Original.TotalCalls;
    /*  Note that Stop_on_std must be set to true here; otherwise, you
     *  end up with just an NMSearch that terminates based on delta.
     */
    Stop_on_std = true;
    IDnumber = 3210;
}
        
      
Hybrid_NMSearch::Hybrid_NMSearch(long dim, 
                                  Vector<double> &startPoint,
                                  double NewSigma, double NewAlpha,
                                  double NewBeta, double NewGamma,
                                  double startStep,
                                  double stopStep,
                                  double EdHJStop,
                                  void (*objective)(long vars,
                                                    Vector<double> &x, 
                                                    double & func,
                                                    bool& flag,
                                                    void* an_obj),
                                  void * input_obj)    
    :NMSearch(dim, startPoint, NewSigma, NewAlpha, NewBeta, NewGamma,
              startStep, stopStep, objective, input_obj)
{
    ESearchStoplength = EdHJStop;
    NSearchCalls = 0;
    ESearchCalls = 0;
    TotalCalls = 0;
    /*  Note that Stop_on_std must be set to true here; otherwise, you
     *  end up with just an NMSearch that terminates based on delta.
     */
    Stop_on_std = true;
    IDnumber = 3210;
}
    

   
Hybrid_NMSearch::~Hybrid_NMSearch()
{
}

void Hybrid_NMSearch::BeginSearch()
{
    NMSearch::BeginSearch();
    NSearchCalls = functionCalls;
#ifdef VERB
    cout <<"number of calls for NMSearch phase is: "
         << functionCalls <<endl;
#endif
    double EStartDelta = GetDelta();
    // double EStartDelta = 1.0;
#ifdef VERB
    cout <<"delta at the end of the NMSearch phase is: "
         << GetDelta()<<endl;
    cout << "\nEStartDelta = " <<  EStartDelta;
    cout << "\nESearchStoplength = " <<  ESearchStoplength;
#endif 
    if (EStartDelta < ESearchStoplength ) {
        cout << "\nWe've already hit delta.  returning...\n";
        return;     
    }
    
    Vector<double> EStartPoint(*minPoint);

    // We need to set MaxCalls appropriately for the EdHJSearch.
    
    long EMaxCalls = GetMaxCalls() - functionCalls; 

    // Now we construct an EdHJSearch object with the settings
    // taken from the end state of the NMSearch.
    
    EdHJSearch ESearch(dimension, EStartPoint, EStartDelta,
                          ESearchStoplength, fcn_name, NULL);
    ESearch.SetStoppingStepLength(ESearchStoplength);
    ESearch.SetMaxCalls(EMaxCalls);

    // if we have set "exact," we need to carry that over to the
    // EdHJSearch.
    if (IsExact()) {
        ESearch.SetMaxCallsExact(EMaxCalls);  
    }
    else {
        ESearch.SetMaxCalls(EMaxCalls);
    }
#ifdef VERB
    cout << "This is the final state of the NMSearch: ";
    PrintDesign();
#endif
    ESearch.SetMinVal(minValue);
#ifdef VERB
    cout << "This is the initial state of the EdHJSearch: ";
    ESearch.PrintDesign();
#endif
    ESearch.BeginSearch();
    
    ESearchCalls = ESearch.GetFunctionCalls();
    TotalCalls = NSearchCalls + ESearchCalls;
    
    Vector<double> newMin(dimension,0.0);
    ESearch.GetMinPoint(newMin);
    SetMinPoint(newMin);
    double newMinVal;
    ESearch.GetMinVal(newMinVal);
    SetMinVal(newMinVal);
#ifdef VERB
    cout << "This is the final state of the EdHJSearch: ";
    ESearch.PrintDesign();
    cout << "Last delta for the EdHJSearch is  ";
    cout << ESearch.GetDelta() << endl;
#endif
}








