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
	std::clock_t time_start=clock(), time_trimDone, time_linDone;

	// variables
	FGTrimmer::Constraints constraints;

	std::cout << "\n-----Performing Simplex Based Trim --------------\n" << std::endl;

	// defaults
    std::string aircraftName = fdm->GetAircraft()->GetAircraftName();
    FGPropertyNode* node = fdm->GetPropertyManager()->GetNode();
    double rtol = node->GetDouble("trim/solver/rtol");
    double abstol = node->GetDouble("trim/solver/abstol");
    double speed = node->GetDouble("trim/solver/speed"); // must be > 1, 2 typical
    double random = node->GetDouble("trim/solver/random");
    int iterMax = node->GetDouble("trim/solver/iterMax");
    bool showConvergeStatus = node->GetBool("trim/solver/showConvergeStatus");
    bool pause = node->GetBool("trim/solver/pause");
    bool showSimplex = node->GetBool("trim/solver/showSimplex");
    bool variablePropPitch = node->GetBool("trim/solver/variablePropPitch");

    // flight conditions
    double phi = fdm->GetIC()->GetPhiRadIC();
    double theta = fdm->GetIC()->GetThetaRadIC();
    double psi = fdm->GetIC()->GetPsiRadIC();
    double gd = fdm->GetInertial()->gravity();

    constraints.velocity = fdm->GetIC()->GetVtrueFpsIC();
    constraints.altitude = fdm->GetIC()->GetAltitudeASLFtIC();
    constraints.gamma = fdm->GetIC()->GetFlightPathAngleRadIC();
    constraints.rollRate = 0;
    constraints.pitchRate = 0;
    constraints.yawRate = tan(phi)*gd*cos(theta)/constraints.velocity;

    constraints.stabAxisRoll = true; // FIXME, make this an option

	// initial solver state
	int n = 6;
	std::vector<double> initialGuess(n), lowerBound(n), upperBound(n), initialStepSize(n);

	lowerBound[0] = node->GetDouble("trim/solver/throttleMin");
	lowerBound[1] = node->GetDouble("trim/solver/elevatorMin");
	lowerBound[2] = node->GetDouble("trim/solver/alphaMin");
	lowerBound[3] = node->GetDouble("trim/solver/aileronMin");
	lowerBound[4] = node->GetDouble("trim/solver/rudderMin");
	lowerBound[5] = node->GetDouble("trim/solver/betaMin");

	upperBound[0] = node->GetDouble("trim/solver/throttleMax");
	upperBound[1] = node->GetDouble("trim/solver/elevatorMax");
	upperBound[2] = node->GetDouble("trim/solver/alphaMax");
	upperBound[3] = node->GetDouble("trim/solver/aileronMax");
	upperBound[4] = node->GetDouble("trim/solver/rudderMax");
	upperBound[5] = node->GetDouble("trim/solver/betaMax");

       initialStepSize[0] = node->GetDouble("trim/solver/throttleStep");
	initialStepSize[1] = node->GetDouble("trim/solver/elevatorStep");
	initialStepSize[2] = node->GetDouble("trim/solver/alphaStep");
	initialStepSize[3] = node->GetDouble("trim/solver/aileronStep");
	initialStepSize[4] = node->GetDouble("trim/solver/rudderStep");
	initialStepSize[5] = node->GetDouble("trim/solver/betaStep");

       initialGuess[0] = node->GetDouble("trim/solver/throttleGuess");
	initialGuess[1] = node->GetDouble("trim/solver/elevatorGuess");
	initialGuess[2] = node->GetDouble("trim/solver/alphaGuess");
	initialGuess[3] = node->GetDouble("trim/solver/aileronGuess");
	initialGuess[4] = node->GetDouble("trim/solver/rudderGuess");
	initialGuess[5] = node->GetDouble("trim/solver/betaGuess");

	// solve
    FGTrimmer * trimmer = new FGTrimmer(fdm, &constraints);
    Callback callback(aircraftName, trimmer);
    FGNelderMead * solver = NULL;
	try
	{
         solver = new FGNelderMead(trimmer,initialGuess,
			lowerBound, upperBound, initialStepSize,iterMax,rtol,
			abstol,speed,random,showConvergeStatus,showSimplex,pause,&callback);
		 while(solver->status()==1) solver->update();
	}
	catch (const std::runtime_error & e)
	{
		std::cout << e.what() << std::endl;
        exit(1);
	}

	// output
	try
	{
        trimmer->printSolution(std::cout,solver->getSolution()); // this also loads the solution into the fdm
        std::cout << "\nfinal cost: " << std::scientific << std::setw(10) << trimmer->eval(solver->getSolution()) << std::endl;
	}
	catch(std::runtime_error & e)
	{
		std::cout << "caught std::runtime error" << std::endl;
		std::cout << "exception: " << e.what() << std::endl;
		exit(1);
	}

	time_trimDone = std::clock();
	std::cout << "\ntrim computation time: " << (time_trimDone - time_start)/double(CLOCKS_PER_SEC) << "s \n" << std::endl;

    if (solver) delete solver;
    if (trimmer) delete trimmer;
}

} // JSBSim

// vim:ts=4:sw=4
