/*
Copyright (c) 2009, Brian Mills
All rights reserved.

Copyright (c) 2021, Agostino De Marco, Elia Tarasov, Michal Podhradsky, Tilda Sikström

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "JSBSimInterface.h"
#include <models/FGAircraft.h>
#include <models/FGAccelerations.h>
#include <math/FGQuaternion.h>

/* 2021-07-08 compiles with JSBSim 1.1.6
 */

JSBSimInterface::JSBSimInterface(int numOutputPorts)
{
	_ac_model_loaded = false;
	fdmExec = new FGFDMExec;
	pm = fdmExec->GetPropertyManager().get();
	propagate = fdmExec->GetPropagate().get();
	accel = fdmExec->GetAccelerations().get();
	auxiliary = fdmExec->GetAuxiliary().get();
	aerodynamics = fdmExec->GetAerodynamics().get();
	propulsion = fdmExec->GetPropulsion().get();
	fcs = fdmExec->GetFCS().get();
	ic = new FGInitialCondition(fdmExec);
	for (int i = 0; i < numOutputPorts; i++) {
		std::vector<SGPropertyNode*> emptyVector;
		outputPorts.push_back(emptyVector);
	}
	//verbosityLevel = JSBSimInterface::eSilent;
}

JSBSimInterface::JSBSimInterface(double dt, int numOutputPorts)
{
	_ac_model_loaded = false;
	fdmExec = new FGFDMExec;
	fdmExec->Setdt(dt);
	mexPrintf("Simulation dt set to %f\n",fdmExec->GetDeltaT());
	pm = fdmExec->GetPropertyManager().get();
	propagate = fdmExec->GetPropagate().get();
	accel = fdmExec->GetAccelerations().get();
	auxiliary = fdmExec->GetAuxiliary().get();
	aerodynamics = fdmExec->GetAerodynamics().get();
	propulsion = fdmExec->GetPropulsion().get();
	fcs = fdmExec->GetFCS().get();
	ic = new FGInitialCondition(fdmExec);
	for (int i = 0; i < numOutputPorts; i++) {
		std::vector<SGPropertyNode*> emptyVector;
		outputPorts.push_back(emptyVector);
	}
	//verbosityLevel = JSBSimInterface::eSilent;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
JSBSimInterface::~JSBSimInterface(void)
{
    delete fdmExec;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::OpenAircraft(const std::string& acName)
{

	if (!fdmExec->GetAircraft()->GetAircraftName().empty()) return false;

    mexPrintf("\tSetting up JSBSim with standard 'aircraft', 'engine', and 'system' paths.\n");
    if (!fdmExec->SetAircraftPath (SGPath("aircraft"))) return false;
    if (!fdmExec->SetEnginePath   (SGPath("engine"))) return false;
    if (!fdmExec->SetSystemsPath  (SGPath("systems"))) return false;

    mexPrintf("\tLoading aircraft '%s' ...\n",acName.c_str());

    if ( ! fdmExec->LoadModel(SGPath("aircraft"), SGPath("engine"), SGPath("systems"), acName)) return false;

	_ac_model_loaded = true;

  	return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::OpenScript(const SGPath& script, double delta_t, const SGPath& initfile)
{

    if (!fdmExec->SetAircraftPath (SGPath("aircraft"))) return false;
    if (!fdmExec->SetEnginePath   (SGPath("engine"))) return false;
    if (!fdmExec->SetSystemsPath  (SGPath("systems"))) return false;

    if (!fdmExec->LoadScript(script, delta_t, initfile)) return false;

    if (!fdmExec->RunIC()) return false;

    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::LoadIC(SGPath ResetName)
{

    auto IC = fdmExec->GetIC();

    if (!IC->Load(ResetName)) return false;

    if (!fdmExec->RunIC()) return false;

	return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::Update()
{

    fdmExec->Run();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::AddInputPropertyNode(std::string property)
{

	SGPropertyNode* node = pm->GetNode(property);
	if (node == nullptr || !node->getAttribute(SGPropertyNode::Attribute::WRITE)) return false;

	inputPort.push_back(node);
	return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::AddWeatherPropertyNode(std::string property)
{

	if (!(property.substr(0, std::string("atmosphere/").size()) == std::string("atmosphere/"))) return false;

	SGPropertyNode* node = pm->GetNode(property);
	if (node == nullptr || !node->getAttribute(SGPropertyNode::Attribute::WRITE)) return false;

	weatherPort.push_back(node);
	return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::AddOutputPropertyNode(std::string property, const int outputPort)
{

	if (outputPort >= outputPorts.size()) return false;

	SGPropertyNode* node = pm->GetNode(property);
	if (node == nullptr || !node->getAttribute(SGPropertyNode::Attribute::READ)) return false;

	outputPorts.at(outputPort).push_back(node);
	return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::CopyInputControlsToJSBSim(std::vector<double> controls) {
    // TODO: error handling if controls is not correct size.

	if (!fdmExec) return false;

	SGPropertyNode* node;
	for (int i = 0; i < inputPort.size(); i++) {
		node = inputPort.at(i);
		switch (node->getType()) {
			case simgear::props::BOOL:
				node->setBoolValue(controls[i]);
				break;
			case simgear::props::INT:
				node->setIntValue(controls[i]);
				break;
			case simgear::props::LONG:
				node->setLongValue(controls[i]);
				break;
			case simgear::props::FLOAT:
				node->setFloatValue(controls[i]);
				break;
			case simgear::props::DOUBLE:
				node->setDoubleValue(controls[i]);
				break;
			default:
				return false;
		}
	}

    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::CopyInputWeatherToJSBSim(std::vector<double> weather) {
    // TODO: error handling if weather is not correct size.

	if (!fdmExec) return false;

	SGPropertyNode* node;
	for (int i = 0; i < weatherPort.size(); i++) {
		node = weatherPort.at(i);
		switch (node->getType()) {
			case simgear::props::BOOL:
				node->setBoolValue(weather[i]);
				break;
			case simgear::props::INT:
				node->setIntValue(weather[i]);
				break;
			case simgear::props::LONG:
				node->setLongValue(weather[i]);
				break;
			case simgear::props::FLOAT:
				node->setFloatValue(weather[i]);
				break;
			case simgear::props::DOUBLE:
				node->setDoubleValue(weather[i]);
				break;
			default:
				return false;
		}
	}

    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::CopyOutputsFromJSBSim(double *stateArray, const int outputPort) {

	if (outputPort >= outputPorts.size()) {
		mexPrintf("Output port selected is out of bounds.\n");
	}

	SGPropertyNode* node;
	std::vector<SGPropertyNode*> port = outputPorts.at(outputPort);
	for (int i = 0; i < port.size(); i++) {
		node = port.at(i);
		switch (node->getType()) {
			case simgear::props::BOOL:
				stateArray[i] = node->getBoolValue();
				break;
			case simgear::props::INT:
				stateArray[i] = node->getIntValue();
				break;
			case simgear::props::LONG:
				stateArray[i] = node->getLongValue();
				break;
			case simgear::props::FLOAT:
				stateArray[i] = node->getFloatValue();
				break;
			case simgear::props::DOUBLE:
				stateArray[i] = node->getDoubleValue();
				break;
			default:
				return false;
		}
	}

	return true;
}
