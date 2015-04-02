/*
 * FGNelderMead.cpp
 * Copyright (C) James Goppert 2010 <james.goppert@gmail.com>
 *
 * FGNelderMead.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGNelderMead.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FGNelderMead.h"
#include <limits>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <ctime>

namespace JSBSim
{

FGNelderMead::FGNelderMead(Function * f, const std::vector<double> & initialGuess,
                           const std::vector<double> & lowerBound,
                           const std::vector<double> & upperBound,
                           const std::vector<double> & initialStepSize, int iterMax,
                           double rtol, double abstol, double speed, double randomization,
                           bool showConvergeStatus,
                           bool showSimplex, bool pause, Callback * callback) :
        m_f(f), m_callback(callback), m_randomization(randomization),
        m_lowerBound(lowerBound), m_upperBound(upperBound),
        m_nDim(initialGuess.size()), m_nVert(m_nDim+1),
        m_iMax(1), m_iNextMax(1), m_iMin(1),
        m_simplex(m_nVert), m_cost(m_nVert), m_elemSum(m_nDim),
        m_status(1),
        initialGuess(initialGuess), initialStepSize(initialStepSize),
        iterMax(iterMax), iter(), rtol(rtol), abstol(abstol),
        speed(speed), showConvergeStatus(showConvergeStatus), showSimplex(showSimplex),
        pause(pause), rtolI(), minCostPrevResize(1), minCost(), minCostPrev(), maxCost(),
        nextMaxCost()
{
    srand ( time(NULL) ); // seed random number generator
}

void FGNelderMead::update()
{
    std::cout.precision(3);

    // reinitialize simplex whenever rtol condition is met
    if ( rtolI < rtol || iter == 0)
    {
        std::vector<double> guess(m_nDim);
        if (iter == 0)
        {
            //std::cout << "constructing simplex" << std::endl;
            guess = initialGuess;
        }
        else
        {
            if (std::abs(minCost-minCostPrevResize) < std::numeric_limits<float>::epsilon())
            {
                m_status = -1;
                throw std::runtime_error("unable to escape local minimum!");
            }
            //std::cout << "reinitializing step size" << std::endl;
            guess = m_simplex[m_iMin];
            minCostPrevResize = minCost;
        }
        constructSimplex(guess,initialStepSize);
    }

    // find vertex costs
    for (unsigned int vertex=0;vertex<m_nVert;vertex++)
    {
        try
        {
            m_cost[vertex] = eval(m_simplex[vertex]);   
        }
        catch (...)
        {
            m_status = -1;
            throw;
        }
    }

    // find max cost, next max cost, and min cost
    m_iMax = m_iNextMax = m_iMin = 0;
    for (unsigned int vertex=0;vertex<m_nVert;vertex++)
    {
        if ( m_cost[vertex] > m_cost[m_iMax] )
        {
            m_iMax = vertex;
        }
        else if ( m_cost[vertex] > m_cost[m_iNextMax] || m_iMax == m_iNextMax ) m_iNextMax = vertex;
        else if ( m_cost[vertex] < m_cost[m_iMin] ) m_iMin = vertex;

    }

    // callback
    if (m_callback) m_callback->eval(m_simplex[m_iMin]);

    // compute relative tolerance
    rtolI = 2*std::abs(m_cost[m_iMax] -
                       m_cost[m_iMin])/(std::abs(m_cost[m_iMax]+std::abs(m_cost[m_iMin])+
                                                 std::numeric_limits<double>::epsilon()));

    // check for max iteration break condition
    if (iter > iterMax)
    {
        m_status = -1;
        throw std::runtime_error("max iterations exceeded!");
    }
    // check for convergence break condition
    else if ( m_cost[m_iMin] < abstol )
    {
        //std::cout << "\nsimplex converged" << std::endl;
        m_status = 0;
        return;
    }

    // compute element sum of simplex vertices
    for (unsigned int dim=0;dim<m_nDim;dim++)
    {
        m_elemSum[dim] = 0;
        for (unsigned int vertex=0;vertex<m_nVert;vertex++)
            m_elemSum[dim] += m_simplex[vertex][dim];
    }

    // min and max costs
    minCostPrev = minCost;
    minCost = m_cost[m_iMin];
    maxCost = m_cost[m_iMax];
    nextMaxCost = m_cost[m_iNextMax];

    // output cost and simplex
    if (showConvergeStatus)
    {
        if ( (minCostPrev + std::numeric_limits<float>::epsilon() )
                < minCost && minCostPrev != 0)
        {
            std::cout << "\twarning: simplex cost increased"
                      << std::scientific
                      << "\n\tcost: " << minCost
                      << "\n\tcost previous: " << minCostPrev
                      << std::fixed << std::endl;
        }

        std::cout << "i: " << iter
                  << std::scientific
                  << "\tcost: " << m_cost[m_iMin]
                  << "\trtol: " << rtolI
                  << std::fixed
                  << "\talpha: " << m_simplex[m_iMin][2]*180/M_PI
                  << "\tbeta: " << m_simplex[m_iMin][5]*180/M_PI
                  << "\tthrottle: " << m_simplex[m_iMin][0]
                  << "\televator: " << m_simplex[m_iMin][1]
                  << "\taileron: " << m_simplex[m_iMin][3]
                  << "\trudder: " << m_simplex[m_iMin][4]
                  << std::endl;
    }
    if (showSimplex)
    {
        std::cout << "simplex: " << std::endl;;
        for (unsigned int j=0;j<m_nVert;j++)
            std::cout << "\t" << std::scientific
                      << std::setw(10) << m_cost[j];
        std::cout << std::endl;
        for (unsigned int j=0;j<m_nVert;j++) std::cout << "\t\t" << j;
        std::cout << std::endl;
        for (unsigned int i=0;i<m_nDim;i++)
        {
            for (unsigned int j=0;j<m_nVert;j++)
                std::cout << "\t" << std::setw(10) << m_simplex[j][i];
            std::cout << std::endl;
        }
        std::cout << std::fixed
                  << "\n\tiMax: " <<  m_iMax
                  << "\t\tiNextMax: " <<  m_iNextMax
                  << "\t\tiMin: " <<  m_iMin << std::endl;
    }

    if (pause)
    {
        std::cout << "paused, press any key to continue" << std::endl;
        std::cin.get();
    }


    // costs
    
    try
    {
        // try inversion
        double costTry = tryStretch(-1.0);
        //std::cout << "cost Try 0: " << costTry << std::endl;

        // if lower cost than best, then try further stretch by double speed factor
        if (costTry < minCost)
        {
            double costTry0 = costTry;
            costTry = tryStretch(speed);
            //std::cout << "cost Try 1: " << costTry << std::endl;

            if (showSimplex)
            {
                if (costTry < costTry0) std::cout << "inversion about: " << m_iMax << std::endl;
                else std::cout << "inversion and stretch about: " << m_iMax << std::endl;
            }
        }
        // otherwise try a contraction
        else if (costTry > nextMaxCost)
        {
            // 1d contraction
            costTry = tryStretch(1./speed);
            //std::cout << "cost Try 2: " << costTry << std::endl;

            // if greater than max cost, contract about min
            if (costTry > maxCost)
            {
                if (showSimplex)
                    std::cout << "multiD contraction about: " << m_iMin << std::endl;
                contract();
            }
            else
            {
                if (showSimplex)
                    std::cout << "contraction about: " << m_iMin << std::endl;
            }
        }
    }

    catch (...)
    {
        m_status = -1;
        throw;
    }

    // iteration
    iter++;

}

int FGNelderMead::status()
{
    return m_status;
}

double FGNelderMead::getRandomFactor()
{
    double randFact = 1+(float(rand() % 1000)/500-1)*m_randomization;
    //std::cout << "random factor: " << randFact << std::endl;;
    return randFact;
}

std::vector<double> FGNelderMead::getSolution()
{
    return m_simplex[m_iMin];
}

double FGNelderMead::tryStretch(double factor)
{
    // randomize factor so we can avoid locking situations
    factor = factor*getRandomFactor();

    // create trial vertex
    double a= (1.0-factor)/m_nDim;
    double b = a - factor;
    std::vector<double> tryVertex(m_nDim);
    for (unsigned int dim=0;dim<m_nDim;dim++)
    {
        tryVertex[dim] = m_elemSum[dim]*a - m_simplex[m_iMax][dim]*b;
        boundVertex(tryVertex,m_lowerBound,m_upperBound);
    }

    // find trial cost
    double costTry = eval(tryVertex);

    // if trial cost lower than max
    if (costTry < m_cost[m_iMax])
    {
        // update the element sum of the simplex
        for (unsigned int dim=0;dim<m_nDim;dim++) m_elemSum[dim] +=
                tryVertex[dim] - m_simplex[m_iMax][dim];
        // replace the max vertex with the trial vertex
        for (unsigned int dim=0;dim<m_nDim;dim++) m_simplex[m_iMax][dim] = tryVertex[dim];
        // update the cost
        m_cost[m_iMax] = costTry;
        if (showSimplex) std::cout << "stretched\t" << m_iMax << "\tby : " << factor << std::endl;
    }
    return costTry;
}

void FGNelderMead::contract()
{
    for (unsigned int dim=0;dim<m_nDim;dim++)
    {
        for (unsigned int vertex=0;vertex<m_nVert;vertex++)
        {
            m_simplex[vertex][dim] =
                getRandomFactor()*0.5*(m_simplex[vertex][dim] +
                     m_simplex[m_iMin][dim]);
        }
    }
}

void FGNelderMead::constructSimplex(const std::vector<double> & guess,
                                    const std::vector<double> & stepSize)
{
    for (unsigned int vertex=0;vertex<m_nVert;vertex++)
    {
        m_simplex[vertex] = guess;
    }

    for (unsigned int dim=0;dim<m_nDim;dim++)
    {
        int vertex = dim + 1;
        m_simplex[vertex][dim] += stepSize[dim]*getRandomFactor();
        boundVertex(m_simplex[vertex],m_lowerBound,m_upperBound);
    }
    if (showSimplex)
    {
        std::cout << "simplex: " << std::endl;;
        for (unsigned int j=0;j<m_nVert;j++) std::cout << "\t\t" << j;
        std::cout << std::endl;
        for (unsigned int i=0;i<m_nDim;i++)
        {
            for (unsigned int j=0;j<m_nVert;j++)
                std::cout << "\t" << std::setw(10) << m_simplex[j][i];
            std::cout << std::endl;
        }
    }
}

void FGNelderMead::boundVertex(std::vector<double> & vertex,
                               const std::vector<double> & lowerBound,
                               const std::vector<double> & upperBound)
{
    for (unsigned int dim=0;dim<m_nDim;dim++)
    {
        if (vertex[dim] > upperBound[dim]) vertex[dim] = upperBound[dim];
        else if (vertex[dim] < lowerBound[dim]) vertex[dim] = lowerBound[dim];
    }
}

double FGNelderMead::eval(const std::vector<double> & vertex, bool check)
{
    if (check) {
        double cost0 = m_f->eval(vertex);
        double cost1 = m_f->eval(vertex);
        if ((cost0 - cost1) > std::numeric_limits<float>::epsilon()) {
            std::stringstream msg;
            msg.precision(10);
            msg << std::scientific
                << "dynamics not stable!"
                << "\tdiff: " << cost1 - cost0
                << "\tcost0: " << cost0
                << "\tcost1: " << cost1
                << std::endl;
            std::cout << msg.str();
            //throw std::runtime_error(msg.str());
        } else {
            return cost1;
        }
    }
    return m_f->eval(vertex);
}

} // JSBSim


// vim:ts=4:sw=4
