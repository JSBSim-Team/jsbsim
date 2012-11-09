/*
 * FGSimplexTrim.cpp
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGSimplexTrim.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGSimplexTrim.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FGSimplexTrim.h"
#include <ctime>

namespace JSBSim {

FGSimplexTrim::FGSimplexTrim(FGFDMExec * fdm, TrimMode mode)
{
	using namespace JSBSim;

	std::clock_t time_start=clock(), time_trimDone, time_linDone;

	// variables
	fdm->Setdt(1./120);
	FGTrimmer::Constraints constraints;

	std::cout << "\n-----Performaing Simplex Based Trim --------------\n" << std::endl;

	// defaults
	constraints.velocity = fdm->GetAuxiliary()->GetVt();
	constraints.altitude = fdm->GetPropagate()->GetAltitudeASL();
	std::string aircraft = fdm->GetAircraft()->GetAircraftName();
	double rtol = fdm->GetPropertyManager()->GetDouble("trim/solver/rtol");
	double abstol = fdm->GetPropertyManager()->GetDouble("trim/solver/abstol");
	double speed = fdm->GetPropertyManager()->GetDouble("trim/solver/speed"); // must be > 1, 2 typical
	double random = fdm->GetPropertyManager()->GetDouble("trim/solver/random");
	int iterMax = fdm->GetPropertyManager()->GetDouble("trim/solver/iterMax");
	bool showConvergeStatus = fdm->GetPropertyManager()->GetBool("trim/solver/showConvergeStatus");
	bool pause = fdm->GetPropertyManager()->GetBool("trim/solver/pause");
	bool showSimplex = fdm->GetPropertyManager()->GetBool("trim/solver/showSimplex");
	bool variablePropPitch = fdm->GetPropertyManager()->GetBool("trim/solver/variablePropPitch");
	int debugLevel = fdm->GetPropertyManager()->GetInt("trim/solver/debugLevel");

	std::string fileName = aircraft;

	// input
	//std::cout << "input ( press enter to accept [default] )\n" << std::endl;

	// load model
	std::string aircraftName = fdm->GetAircraft()->GetAircraftName();
	//prompt("\tdebug level\t\t",debugLevel);
	//fdm->SetDebugLevel(debugLevel);
	//std::cout << "model selection" << std::endl;
	//while (1)
	//{
		//prompt("\taircraft\t\t",aircraft);
		//prompt("\toutput file name\t",fileName);
		//fdm->LoadModel("../aircraft","../engine","../systems",aircraft);
		//aircraftName = fdm->GetAircraft()->GetAircraftName();
		//if (aircraftName == "")
		//{
			//std::cout << "\tfailed to load aircraft" << std::endl;
		//}
		//else
		//{
			//std::cout << "\tsuccessfully loaded: " <<  aircraftName << std::endl;
			//break;
		//}
	//}

	// Turn on propulsion system
	fdm->GetPropulsion()->InitRunning(-1);

	// get propulsion pointer to determine type/ etc.
	FGEngine * engine0 = fdm->GetPropulsion()->GetEngine(0);
	FGThruster * thruster0 = engine0->GetThruster();

	// flight conditions
	//std::cout << "\nflight conditions: " << std::endl;
	//prompt("\taltitude, ft\t\t",constraints.altitude);
	//prompt("\tvelocity, ft/s\t\t",constraints.velocity);
	//prompt("\tgamma, deg\t\t",constraints.gamma); constraints.gamma = constraints.gamma*M_PI/180;
	
	double phi = fdm->GetPropagate()->GetEuler(1);
	double theta = fdm->GetPropagate()->GetEuler(2);
	double psi = fdm->GetPropagate()->GetEuler(3);

	// TODO check that this works properly
	constraints.gamma = theta;

	//if (thruster0->GetType()==FGThruster::ttPropeller)
		//prompt("\tvariable prop pitch?\t\t",variablePropPitch);
	// FIXME, enable
		
    constraints.rollRate = fdm->GetIC()->GetPRadpsIC();
    constraints.pitchRate = fdm->GetIC()->GetQRadpsIC();
    constraints.yawRate = fdm->GetIC()->GetRRadpsIC();
    constraints.stabAxisRoll = true; // FIXME, make this an option

	// solver properties
	// TODO make these options
	//std::cout << "\nsolver properties: " << std::endl;
	//std::cout << std::scientific;
	//prompt("\tshow converge status?\t",showConvergeStatus);
	//prompt("\tshow simplex?\t\t",showSimplex);
	//prompt("\tpause?\t\t\t",pause);
	//prompt("\trelative tolerance\t",rtol);
	//prompt("\tabsolute tolerance\t",abstol);
	//prompt("\tmax iterations\t\t",iterMax);
	//prompt("\tconvergence speed\t",speed);
	//prompt("\trandomization ratio\t",random);
	//std::cout << std::fixed;

	// initial solver state
	int n = 6;
	std::vector<double> initialGuess(n), lowerBound(n), upperBound(n), initialStepSize(n);

	lowerBound[0] = fdm->GetPropertyManager()->GetDouble("trim/solver/throttleMin");
	lowerBound[1] = fdm->GetPropertyManager()->GetDouble("trim/solver/elevatorMin");
	lowerBound[2] = fdm->GetPropertyManager()->GetDouble("trim/solver/alphaMin");
	lowerBound[3] = fdm->GetPropertyManager()->GetDouble("trim/solver/aileronMin");
	lowerBound[4] = fdm->GetPropertyManager()->GetDouble("trim/solver/rudderMin");
	lowerBound[5] = fdm->GetPropertyManager()->GetDouble("trim/solver/betaMin");

	upperBound[0] = fdm->GetPropertyManager()->GetDouble("trim/solver/throttleMax");
	upperBound[1] = fdm->GetPropertyManager()->GetDouble("trim/solver/elevatorMax");
	upperBound[2] = fdm->GetPropertyManager()->GetDouble("trim/solver/alphaMax");
	upperBound[3] = fdm->GetPropertyManager()->GetDouble("trim/solver/aileronMax");
	upperBound[4] = fdm->GetPropertyManager()->GetDouble("trim/solver/rudderMax");
	upperBound[5] = fdm->GetPropertyManager()->GetDouble("trim/solver/betaMax");

    initialStepSize[0] = fdm->GetPropertyManager()->GetDouble("trim/solver/throttleStep");
	initialStepSize[1] = fdm->GetPropertyManager()->GetDouble("trim/solver/elevatorStep");
	initialStepSize[2] = fdm->GetPropertyManager()->GetDouble("trim/solver/alphaStep");
	initialStepSize[3] = fdm->GetPropertyManager()->GetDouble("trim/solver/aileronStep");
	initialStepSize[4] = fdm->GetPropertyManager()->GetDouble("trim/solver/rudderStep");
	initialStepSize[5] = fdm->GetPropertyManager()->GetDouble("trim/solver/betaStep");

    initialGuess[0] = fdm->GetPropertyManager()->GetDouble("trim/solver/throttleGuess");
	initialGuess[1] = fdm->GetPropertyManager()->GetDouble("trim/solver/elevatorGuess");
	initialGuess[2] = fdm->GetPropertyManager()->GetDouble("trim/solver/alphaGuess");
	initialGuess[3] = fdm->GetPropertyManager()->GetDouble("trim/solver/aileronGuess");
	initialGuess[4] = fdm->GetPropertyManager()->GetDouble("trim/solver/rudderGuess");
	initialGuess[5] = fdm->GetPropertyManager()->GetDouble("trim/solver/betaGuess");

	// solve
	FGTrimmer trimmer(fdm, &constraints);
	Callback callback(fileName,&trimmer);
	FGNelderMead * solver;
	try
	{
		 solver = new FGNelderMead(&trimmer,initialGuess,
			lowerBound, upperBound, initialStepSize,iterMax,rtol,
			abstol,speed,random,showConvergeStatus,showSimplex,pause,&callback);
		 while(solver->status()==1) solver->update();
	}
	catch (const std::runtime_error & e)
	{
		std::cout << e.what() << std::endl;
		//exit(1);
	}

	// output
	try
	{
		trimmer.printSolution(std::cout,solver->getSolution()); // this also loads the solution into the fdm
		std::cout << "\nfinal cost: " << std::scientific << std::setw(10) << trimmer.eval(solver->getSolution()) << std::endl;
	}
	catch(std::runtime_error & e)
	{
		std::cout << "caught std::runtime error" << std::endl;
		std::cout << "exception: " << e.what() << std::endl;
		exit(1);
	}

	time_trimDone = std::clock();
	std::cout << "\ntrim computation time: " << (time_trimDone - time_start)/double(CLOCKS_PER_SEC) << "s \n" << std::endl;

	//std::cout << "\nsimulating flight to determine trim stability" << std::endl;

	//std::cout << "\nt = 5 seconds" << std::endl;
	//for (int i=0;i<5*120;i++) fdm->Run();
	//trimmer.printState();

	//std::cout << "\nt = 10 seconds" << std::endl;
	//for (int i=0;i<5*120;i++) fdm->Run();
	//trimmer.printState();

	std::cout << "\nlinearization: " << std::endl;
	FGStateSpace ss(fdm);

	ss.x.add(new FGStateSpace::Vt);
	ss.x.add(new FGStateSpace::Alpha);
	ss.x.add(new FGStateSpace::Theta);
	ss.x.add(new FGStateSpace::Q);

	if (thruster0->GetType()==FGThruster::ttPropeller)
	{
		ss.x.add(new FGStateSpace::Rpm0);
		if (variablePropPitch) ss.x.add(new FGStateSpace::PropPitch);
		int numEngines = fdm->GetPropulsion()->GetNumEngines();
		if (numEngines>1) ss.x.add(new FGStateSpace::Rpm1);
		if (numEngines>2) ss.x.add(new FGStateSpace::Rpm2);
		if (numEngines>3) ss.x.add(new FGStateSpace::Rpm3);
	}
	ss.x.add(new FGStateSpace::Beta);
	ss.x.add(new FGStateSpace::Phi);
	ss.x.add(new FGStateSpace::P);
	ss.x.add(new FGStateSpace::Psi);
	ss.x.add(new FGStateSpace::R);
	ss.x.add(new FGStateSpace::Latitude);
	ss.x.add(new FGStateSpace::Longitude);
	ss.x.add(new FGStateSpace::Alt);

	ss.u.add(new FGStateSpace::ThrottleCmd);
	ss.u.add(new FGStateSpace::DaCmd);
	ss.u.add(new FGStateSpace::DeCmd);
	ss.u.add(new FGStateSpace::DrCmd);

	// state feedback
	ss.y = ss.x;

	std::vector< std::vector<double> > A,B,C,D;
	std::vector<double> x0 = ss.x.get(), u0 = ss.u.get();
	std::vector<double> y0 = x0; // state feedback
	std::cout << ss << std::endl;

	ss.linearize(x0,u0,y0,A,B,C,D);

	int width=10;
	std::cout.precision(3);
	std::cout
		<< std::fixed
		<< std::right
		<< "\nA=\n" << std::setw(width) << A
		<< "\nB=\n" << std::setw(width) << B
		<< "\nC=\n" << std::setw(width) << C
		<< "\nD=\n" << std::setw(width) << D
		<< std::endl;

	// write scicoslab file
	std::ofstream scicos(std::string(aircraft+"_lin.sce").c_str());
	scicos.precision(10);
	width=20;
	scicos
	<< std::scientific
	<< aircraft << ".x0=..\n" << std::setw(width) << x0 << ";\n"
	<< aircraft << ".u0=..\n" << std::setw(width) << u0 << ";\n"
	<< aircraft << ".sys = syslin('c',..\n"
	<< std::setw(width) << A << ",..\n"
	<< std::setw(width) << B << ",..\n"
	<< std::setw(width) << C << ",..\n"
	<< std::setw(width) << D << ");\n"
	<< aircraft << ".tfm = ss2tf(" << aircraft << ".sys);\n"
	<< std::endl;

	time_linDone = std::clock();
	std::cout << "\nlinearization computation time: " << (time_linDone - time_trimDone)/double(CLOCKS_PER_SEC) << " s\n" << std::endl;

    if (solver) delete solver;
}

} // JSBSim

// vim:ts=4:sw=4
