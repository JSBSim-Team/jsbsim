/*******************************************************************************

 Header:       FGTrimLong.cpp
 Author:       Tony Peden
 Date started: 9/8/99

 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.


 HISTORY
--------------------------------------------------------------------------------
9/8/99   TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class takes the given set of IC's and finds the angle of attack, elevator,
and throttle setting required to fly steady level. This is currently for in-air
conditions only.  It is implemented using an iterative, one-axis-at-a-time 
scheme. */




/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGInitialCondition.h"
#include "FGTrimLong.h"
#include "FGAircraft.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/





FGTrimLong::FGTrimLong(FGFDMExec *FDMExec,FGInitialCondition *FGIC )
{

    Ncycles=100;
    Naxis=40;
    Tolerance=1E-3;
    A_Tolerance = Tolerance / 10;
    Alpha_Gain=0.07;
    Throttle_Gain=0.01;
    Elev_Gain=0.05;
    alphaMin=-5;alphaMax=20;
    Debug=0;
    fdmex=FDMExec;
    fgic=FGIC;
    udotf=&FGTrimLong::udot_func;
    wdotf=&FGTrimLong::wdot_func;
    qdotf=&FGTrimLong::qdot_func;

}

FGTrimLong::~FGTrimLong(void) {}

void FGTrimLong::setThrottlesPct(float tt)
{
    int N=fdmex -> GetAircraft() -> GetNumEngines();
    for(int i=0;i<N;i++)
        fdmex -> GetFCS() -> SetThrottleCmd(i,tt);
}


bool FGTrimLong::checkLimits(trimfp fp, float min, float max)
{
    float lo,hi;
    lo=(this->*fp)(min);
    hi=(this->*fp)(max);
    if(lo*hi >= 0) {
        cout << "Lo: " << lo << "Hi: " << hi << endl;
        return false;
    }
    else
        return true;
}

bool FGTrimLong::solve(trimfp fp,float guess,float desired, float *result, float eps, int max_iterations) {

    float x1,x2,x3,f1,f2,f3,d,d0;
    float const relax =0.9;

    int i;
    d=1;
    bool success=false;
    //initializations
    if(findInterval(fp,&x1,&x3,guess,desired,max_iterations)) {

        f1=(this->*fp)(x1)-desired;
        f3=(this->*fp)(x3)-desired;
        d0=fabs(x3-x1);

        //iterations
        i=0;
        while ((fabs(d) > eps) && (i < max_iterations)){
            if(Debug > 1)
                cout << "FGTrimLong::solve i,x1,x2,x3: " << i << ", " << x1 << ", " << x2 << ", " << x3 << endl;

            d=(x3-x1)/d0;
            x2=x1-d*d0*f1/(f3-f1);
            f2=(this->*fp)(x2)-desired;
            if(f1*f2 <= 0.0){
                x3=x2;
                f3=f2;
                f1=relax*f1;
            }
            else if(f2*f3 <= 0){
                x1=x2;
                f1=f2;
                f3=relax*f3;
            }
            //cout << i << endl;
            i++;
        }//end while
        if(i < 100) {
            success=true;
            *result=x2;
        }

    } else {
        cout << "No sign change in interval: " << x1 << "," << x3 << endl;
    }
    return success;
}

bool FGTrimLong::findInterval(trimfp fp, float *lo, float *hi,float guess,float desired,int max_iterations) {

    int i=0;
    bool found=false;
    float flo,fhi,fguess;
    float xlo,xhi,step;
    step=0.1*guess;
    fguess=(this->*fp)(guess)-desired;
    xlo=xhi=guess;
    do{
        step=2*step;
        xlo-=step;
        xhi+=step;
        i++;
        flo=(this->*fp)(xlo)-desired;
        fhi=(this->*fp)(xhi)-desired;
        if(flo*fhi <=0){  //found interval with root
            found=true;
            if(flo*fguess <= 0){  //narrow interval down a bit
                xhi=xlo+step;    //to pass solver interval that is as
                //small as possible
            }
            else if(fhi*fguess <= 0){
                xlo=xhi-step;
            }
        }
        if(Debug > 1)
            cout << "FGTrimLong::findInterval: i=" << i << " Lo= " << xlo << " Hi= " << xhi << " flo*fhi: " << flo*fhi << endl;
    }
    while((found == 0) && (i <= max_iterations));
    *lo=xlo;
    *hi=xhi;
    return found;
}

float FGTrimLong::udot_func(float x) {
    fdmex->GetFCS()->SetThrottleCmd(-1,x);
    fdmex->RunIC(fgic);
    return fdmex->GetTranslation()->GetUVWdot()(1);
}

float FGTrimLong::wdot_func(float x) {
    fgic->SetAlphaDegIC(x);
    fdmex->RunIC(fgic);
    return fdmex->GetTranslation()->GetUVWdot()(3);
}

float FGTrimLong::qdot_func(float x) {
    fdmex->GetFCS()->SetDeCmd(x);
    fdmex->RunIC(fgic);
    return fdmex->GetRotation()->GetPQRdot()(2);
}

bool FGTrimLong::DoTrim(void)
{
    int k=0,j=0,sum=0,trim_failed=0,jmax=Naxis;

    float step,temp,min,max;
    float alpha,de,dth;

    trimfp fp;

    alpha=de=dth=0;
    fgic -> SetAlphaDegIC(alpha);
    fdmex -> GetFCS() -> SetDeCmd(de);
    setThrottlesPct(dth);
    fdmex -> RunIC(fgic);
    if(!checkLimits(wdotf,alphaMin,alphaMax))
        cout << "Sorry, it doesn't appear as if wdot is trimmable." << endl;
    if(!checkLimits(udotf,0,1))
        cout << "Sorry, it doesn't appear as if udot is trimmable." << endl;
    if(!checkLimits(qdotf,-1,1))
        cout << "Sorry, it doesn't appear as if qdot is trimmable." << endl;
    min=alphaMin; max=alphaMax;

    do {
        solve(wdotf,5,0,&wdot,Tolerance,Naxis);
        if(Debug > 0) {
            cout << "Alpha: " << fdmex->GetTranslation()->Getalpha()*RADTODEG
            << " wdot: " << fdmex->GetTranslation()->GetUVWdot()(3)
            << endl;
        }
        solve(udotf,0.5,0,&udot,Tolerance,Naxis);
        if(Debug > 0) {
            cout << "Throttle: " << fdmex->GetFCS()->GetThrottlePos(-1)
            << " udot: " << fdmex->GetTranslation()->GetUVWdot()(1)
            << endl;
        }
        solve(qdotf,0.5,0,&qdot,A_Tolerance,Naxis);
        if(Debug > 0) {
            cout << "Elevator: " << fdmex->GetFCS()->GetDePos()
            << " qdot: " << fdmex->GetRotation()->GetPQRdot()(1)
            << endl;
        }

        if((Ncycles > 3) && (k == Ncycles-2) ) {
            wdot=fdmex->GetTranslation()->GetUVWdot()(3);
            udot=fdmex->GetTranslation()->GetUVWdot()(1);
            qdot=fdmex->GetRotation()->GetPQRdot()(2);
            if((fabs(wdot) > Tolerance) || (fabs(udot) > Tolerance) || (fabs(qdot) > A_Tolerance)) {
                //failure is imminent, try to show the user why
                cout << endl << "Trimming routine failure is imminent" << endl;
                cout << "Hmm...let's see here" << endl;
                if(wdot > Tolerance)
                    cout << "wdot is too high: " << wdot << endl;
                else
                    cout << "wdot is ok" << endl;
                if(udot > Tolerance)
                    cout << "udot is too high: " << udot << endl;
                else
                    cout << "udot is ok" << endl;
                if(qdot > A_Tolerance)
                    cout << "qdot is too high: " << qdot << endl;
                else
                    cout << "qdot is ok" << endl;

                trim_failed=true;

            }
        }
        k++;
    }while(((fabs(wdot) > Tolerance) || (fabs(udot) > Tolerance) || (fabs(qdot) > A_Tolerance)) && (k < Ncycles));

    cout << "Total Iterations: " << k << endl;
    return trim_failed;
}



