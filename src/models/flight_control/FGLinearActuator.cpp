/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
#include "math/FGParameterValue.h"
#include <iostream>

using namespace std;

namespace JSBSim {
    
    /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
     * CLASS IMPLEMENTATION
     * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
    
    FGLinear_Actuator::FGLinear_Actuator(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
    {
        
        if (element->FindElement("set")) {
            isSetProperty = true;
            setProperty = PropertyManager->CreatePropertyObject<double>(element->FindElementValue("set"));
        }
        if (element->FindElement("reset")) {
            isResetProperty = true;
            resetProperty = PropertyManager->CreatePropertyObject<double>(element->FindElementValue("reset"));
        }
        
        ptrVersus = nullptr;
        if (element->FindElement("versus")) {
            string property_string = element->FindElementValue("versus");
            ptrVersus = new FGParameterValue(property_string, PropertyManager);
            double setVersus = ptrVersus->GetValue();
            if (ptrVersus->IsConstant()) {
                if (setVersus >= 0.5) {
                    versus = 1;
                } else if (setVersus <= -0.5) {
                    versus = -1;
                }
            }
        } else versus = 0;
        
        ptrBias = nullptr;
        if (element->FindElement("bias")) {
            string property_string = element->FindElementValue("bias");
            ptrBias = new FGParameterValue(property_string, PropertyManager);
            bias = ptrBias->GetValue();
        } else bias = 0;
        
        if (element->FindElement("module")) {
            module = element->FindElementValueAsNumber("module");
            if (module < 0) module = 1.0;
        }
        if (element->FindElement("hysteresis")) {
            hysteresis = element->FindElementValueAsNumber("hysteresis");
            if (hysteresis < 0) hysteresis = 0.0;
        }
        
        if (element->FindElement("lag")) {
            double aLag = element->FindElementValueAsNumber("lag");
            if (aLag > 0.0) {
                lag = aLag;
                double denom = 2.00 + dt*lag;
                ca = dt * lag / denom;
                cb = (2.00 - dt * lag) / denom;
            }
        }
        
        if (element->FindElement("rate")) {
            double aRate = element->FindElementValueAsNumber("rate");
            if (aRate <= 0 or aRate > 1.0) aRate = 0.5;
            rate = aRate;
        }
        
        if (element->FindElement("gain")) gain = element->FindElementValueAsNumber("gain");
        
        FGFCSComponent::bind();
        
        Debug(0);
    }
    
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    FGLinear_Actuator::~FGLinear_Actuator()
    {
        Debug(1);
    }
    
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    bool FGLinear_Actuator::Run(void )
    {
        
        if (isSetProperty) set = abs(setProperty) >= 0.5;
        if (isResetProperty) reset = abs(resetProperty) >= 0.5;
        
        if (reset) {
            input_mem = 0.0;
            countSpin = 0;
            direction = 0;
            Output = 0.0;
            input_lost = 0.0;
        } else {
            if (set) {
                Input = InputNodes[0]->getDoubleValue() - input_lost;
                double input_delta = Input - input_mem;
                if (abs(input_delta) >= hysteresis) {
                    if (ptrVersus && !ptrVersus->IsConstant()) {
                        double setVersus = ptrVersus->GetValue();
                        if (setVersus >= 0.5) {
                            versus = 1;
                        } else if (setVersus <= -0.5) {
                            versus = -1;
                        } else versus = 0;
                    }
                    if (abs(input_delta) <= (module * rate)) {
                        if (input_delta > 0.0) {
                            direction = 1;
                        } else if (input_delta < 0.0) {
                            direction = -1;
                        }
                    }
                    if ((versus == 0) || (versus == direction)) {
                        input_mem = Input;
                        if (direction != 0) {
                            if (abs(input_delta) >= (module*rate)) {
                                if (direction > 0) {
                                    countSpin++;
                                    direction = 0;
                                } else if (direction < 0) {
                                    countSpin--;
                                    direction = 0;
                                }
                            }
                        }
                    } else if ((versus != 0) && (direction != 0) && (versus != direction)) {
                        input_lost += input_delta;
                    }
                }
            }
            Output = gain * (bias + input_mem + module*countSpin);
        }
        
        if (lag > 0.0) {
            double input = Output; 
            Output = ca * (input + previousLagInput) + previousLagOutput * cb;
            previousLagInput = input;
            previousLagOutput = Output;
        }
        
        Clip();
        if (IsOutput) SetOutput();
        
        return true;
    }
    
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
                cout << "  input_mem: " << input_mem << endl;
                cout << "       bias: " << bias << endl;
                cout << "     module: " << module << endl;
                cout << " hysteresis: " << hysteresis << endl;
                cout << "       rate: " << rate << endl;
                cout << "     versus: " << versus << endl;
                cout << "  direction: " << direction << endl;
                cout << "  countSpin: " << countSpin << endl;
                cout << "        Lag: " << lag << endl;
                cout << "       Gain: " << gain << endl;
                cout << "        set: " << set << endl;
                cout << "      reset: " << reset << endl;
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
    
}



