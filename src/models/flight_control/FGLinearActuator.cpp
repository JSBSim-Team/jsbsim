/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * 
 * Module:       FGLinearActuator.cpp
 * Author:       Adriano Bassignana
 * Date started: 2019-01-03
 * 
 * ------------- Copyright (C) 2000 -------------
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Further information about the GNU Lesser General Public License can also be found on
 * the world wide web at http://www.gnu.org.
 * 
 * FUNCTIONAL DESCRIPTION
 * --------------------------------------------------------------------------------
 * 
 * HISTORY
 * --------------------------------------------------------------------------------
 * 
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * COMMENTS, REFERENCES,  and NOTES
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * 
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * INCLUDES
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGLinearActuator.h"
#include "input_output/FGXMLElement.h"
#include <iostream>

using namespace std;

namespace JSBSim {
    
    /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
     * CLASS IMPLEMENTATION
     * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
    
    FGLinear_Actuator::FGLinear_Actuator(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
    {
        initialized = 1;
        versus = 0.0;
        isSetProperty = false;
        isResetProperty = false;
        direction = 0;
        countSpin = 0;
        input_prec = 0.0;
        module = 1.0;
        rate = 0.3;
        minRate = -0.3;
        maxRate = 0.3;
        lag = 0.0;
        lagPrevius = 0.0;
        gain = 1.0;
        
        if (element->FindElement("set")) {
            setProperty = PropertyManager->CreatePropertyObject<double>(element->FindElementValue("set"));
            isSetProperty = true;
        }
        if (element->FindElement("reset")) {
            resetProperty = PropertyManager->CreatePropertyObject<double>(element->FindElementValue("reset"));
            isResetProperty = true;
        }

        if (element->FindElement("versus")) versus = element->FindElementValueAsNumber("versus");
        if (element->FindElement("lag")) lag = element->FindElementValueAsNumber("lag");
        if (element->FindElement("module")) module = element->FindElementValueAsNumber("module");
        if (element->FindElement("rate")) {
            rate = element->FindElementValueAsNumber("rate");
            if (rate <= 0 or rate > 1.0) rate = 0.5;
            minRate = -(rate);
            maxRate = rate;
        }
        if (element->FindElement("gain")) gain = element->FindElementValueAsNumber("gain");
        
        FGFCSComponent::bind();
        
        Debug(0);
    }
    
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    FGLinear_Actuator::~FGLinear_Actuator()
    {
        Debug(1);
    }
    
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    bool FGLinear_Actuator::Run(void )
    {
        Input = InputNodes[0]->getDoubleValue();
        if (initialized) {
            input_prec = Input;
            initialized = 0;
            std::cerr << "FGLinear_Actuator Cicle_zero " << InputNodes[0]->GetNameWithSign() << std::endl;
            return true;
        }
        
        Output = 0.0;
        
        set = 1.0;
        reset = 0.0;
        
        if (isSetProperty) set = setProperty;
        if (isResetProperty) reset = resetProperty;
            
        if (set > 0 ) {
            
            double input_delta = Input - input_prec;
            
            if ((versus < 0.0 and input_delta > 0.0) or (versus  > 0.0 and input_delta < 0.0)) {
                input_delta = 0.0;
            }
            input_prec = Input;
            
            if (input_delta != 0.0) {
                if (input_delta <= (module * maxRate) and input_delta >= (module * minRate)) {
                    if (input_delta > 0.0) {
                        direction = 1;
                        //cout << "FGLinear_Actuator::Run" << InputNodes[0]->GetNameWithSign() << " direction + " << direction << endl;
                    } else if (input_delta < 0.0) {
                        direction = -1;
                        //cout << "FGLinear_Actuator::Run" << InputNodes[0]->GetNameWithSign() << " direction - " << direction << endl;
                    }
                }
                
                if (direction != 0) {
                    if (input_delta >= (module * maxRate) or input_delta <= (module * minRate)) {
                        if (direction > 0) {
                            countSpin++;
                            cout << "FGLinear_Actuator::Run" << InputNodes[0]->GetNameWithSign() << " countSpin++ " << countSpin << " maxRate " << maxRate << " minRate " << minRate << endl;
                            direction = 0;
                        } else if (direction < 0) {
                            countSpin--;
                            cout << "FGLinear_Actuator::Run" << InputNodes[0]->GetNameWithSign() << " countSpin-- " << countSpin << " maxRate " << maxRate << " minRate " << minRate << endl;
                            direction = 0;
                        }
                    }
                }
                
            }
        } else {
            Input = input_prec;
        }
        
        if (reset > 0) {
            Input = 0.0;
            countSpin = 0.0;
        }
        
        Output = gain * (Input + module*countSpin);
        Lag();
        Clip();
        if (IsOutput) SetOutput();
        
        return true;
    }
    
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    void FGLinear_Actuator::Lag(void)
    {
        // "Output" on the right side of the "=" is the current frame input
        // for this Lag filter
        double input = Output;
        
        if (lag > 0.0) {
            if (lag == lagPrevius) {
                Output = ca * (input + previousLagInput) + previousLagOutput * cb;
            } else {
                double denom = 2.00 + dt*lag;
                ca = dt * lag / denom;
                cb = (2.00 - dt * lag) / denom;
                previousLagInput = 0.0;
                previousLagOutput = 0.0;
                lagPrevius = lag;
            }
            
            previousLagInput = input;
            previousLagOutput = Output;
        }
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
    
    void FGLinear_Actuator::Debug(int from)
    {
        if (debug_lvl <= 0) return;
        
        if (debug_lvl & 1) { // Standard console startup message output
            if (from == 0) { // Constructor
                cout << "      INPUT: " << InputNodes[0]->GetNameWithSign() << endl;
                cout << "     module: " << module << endl;
                if (direction != 0.0) cout << " direction: " << direction << endl;
                cout << "       rate: " << rate << endl;
                if (versus != 0.0) cout << "     versus: " << versus << endl;
                cout << "  countSpin: " << countSpin << endl;
                if (lag != 0.0)  cout  << "       Lag: " << lag << endl;
                if (gain != 0.0)  cout << "      Gain: " << gain << endl;
                for (auto node: OutputNodes)
                    cout << "     OUTPUT: " << node->GetName() << endl;
            }
        }
        if (debug_lvl & 2 ) { // Instantiation/Destruction notification
            if (from == 0) cout << "Instantiated: FGLinear_Actuator" << endl;
            if (from == 1) cout << "Destroyed:    FGLinear_Actuator" << endl;
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
    
} //namespace JSBSim

