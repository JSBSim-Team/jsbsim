<?php

$version = 0.95;

//****************************************************
//                                                   *
// This is the CGI back-end for the Aero-Matic gen-  *
// erator. The front-end is aeromatic.html.          *
//                                                   *
// July 2003, David P. Culp, davidculp2@comcast.net  *
//                                                   * 
//****************************************************
// Updated: 29 Sep 2003, DPC - added high alpha/beta drag
// Updated:  5 Sep 2003, DPC - added prop transport
// Updated: 16 Oct 2003, DPC - added better FCS
// Updated:  9 Nov 2003, JSB - removed INVERT keyword and inserted "-"
// Updated: 21 Feb 2004, DPC - added VRP, increased version to 1.61
// Updated: 17 Jun 2004, JSB - increased version number to 1.65
// Updated: 14 Dec 2004, DPC - adapted to new XML format, v2.0
// Updated: 29 Dec 2005, DPC - more v2.0 updates, added incidence
// Updated: 21 Oct 2008, DPC - fixed drag due to elevator with <abs>
// Updated: 11 Apr 2009, DPC - use "0|1" for gear retractability
// Updated: 21 Jul 2011, DPC - fix rudder travel limit bug


header("Content-type: text/plain");


//***** GET DATA FROM USER ***************************

$ac_units           = $_POST['ac_units'];
$ac_name            = $_POST['ac_name'];
$ac_type            = $_POST['ac_type'];
$ac_weight          = $_POST['ac_weight'];
$ac_wingspan        = $_POST['ac_wingspan'];
$ac_length          = $_POST['ac_length'];
$ac_wingarea        = $_POST['ac_wingarea'];
$ac_geartype        = $_POST['ac_geartype'];
$ac_gearretract     = $_POST['ac_gearretract'];
$ac_numengines      = $_POST['ac_numengines'];
$ac_enginetype      = $_POST['ac_enginetype'];
$ac_enginelayout    = $_POST['ac_enginelayout'];
$ac_enginename      = $_POST['ac_enginename'];
$ac_enginepower     = $_POST['ac_enginepower'];
$ac_engineunits     = $_POST['ac_engineunits'];
$ac_augmented       = $_POST['ac_augmented'];
$ac_yawdamper       = $_POST['ac_yawdamper'];
$ac_wingchord       = $_POST['ac_wingchord'];
$ac_htailarea       = $_POST['ac_htailarea'];
$ac_htailarm        = $_POST['ac_htailarm'];
$ac_vtailarea       = $_POST['ac_vtailares'];
$ac_vtailarm        = $_POST['ac_vtailarm'];
$ac_emptyweight     = $_POST['ac_emptyweight'];

//***** CONVERT TO ENGLISH UNITS *********************

if ($ac_units == 1) { 
  $ac_weight *= 2.205;
  $ac_wingspan *= 3.281;
  $ac_length *= 3.281;
  $ac_wingarea *= 10.765;
  $ac_wingchord *= 3.281;
  $ac_htailarea *= 10.765;
  $ac_htailarm *= 3.281;
  $ac_vtailarea *= 10.765;
  $ac_vtailarm *= 3.281;
  $ac_emptyweight *= 2.205;
  }

//***** METRICS ***************************************

// first, estimate wing loading in psf
switch($ac_type) { 
  case 0: $ac_wingloading = 7.0;   break;  // glider
  case 1: $ac_wingloading = 14.0;  break;  // light single
  case 2: $ac_wingloading = 29.0;  break;  // light twin
  case 3: $ac_wingloading = 45.0;  break;  // WW2 fighter, racer
  case 4: $ac_wingloading = 95.0;  break;  // single-eng jet fighter
  case 5: $ac_wingloading = 100.0; break;  // 2-eng jet fighter
  case 6: $ac_wingloading = 110.0; break;  // 2-eng jet transport
  case 7: $ac_wingloading = 110.0; break;  // 3-eng jet transport
  case 8: $ac_wingloading = 110.0; break;  // 4-eng jet transport
  case 9: $ac_wingloading = 57.0;  break;  // prop transport
  }

// if no wing area given, use wing loading to estimate
if ($ac_wingarea == 0) {
    $wingarea_input = false;
    $ac_wingarea = $ac_weight / $ac_wingloading;
  }
  else {
    $wingarea_input = true;
    $ac_wingloading = $ac_weight / $ac_wingarea;
  }

// calculate wing chord
$ac_wingchord = $ac_wingarea / $ac_wingspan;

// calculate aspect ratio
$ac_aspectratio = $ac_wingspan / $ac_wingchord;

// calculate half-span
$ac_halfspan = $ac_wingspan / 2;

// for now let's use a standard 2 degrees wing incidence
$ac_wingincidence = 2.0;

// estimate horizontal tail area
if ($ac_htailarea == 0) {
  switch($ac_type) {
    case 0: $ac_htailarea = $ac_wingarea * 0.12; break;
    case 1: $ac_htailarea = $ac_wingarea * 0.16; break;
    case 2: $ac_htailarea = $ac_wingarea * 0.16; break;
    case 3: $ac_htailarea = $ac_wingarea * 0.17; break;
    case 4: $ac_htailarea = $ac_wingarea * 0.20; break;
    case 5: $ac_htailarea = $ac_wingarea * 0.20; break;
    case 6: $ac_htailarea = $ac_wingarea * 0.25; break;
    case 7: $ac_htailarea = $ac_wingarea * 0.25; break;
    case 8: $ac_htailarea = $ac_wingarea * 0.25; break;
    case 9: $ac_htailarea = $ac_wingarea * 0.16; break;
    }
  }

// estimate distance from CG to horizontal tail aero center
if ($ac_htailarm == 0) {
  switch($ac_type) {
    case 0: $ac_htailarm = $ac_length * 0.60; break;
    case 1: $ac_htailarm = $ac_length * 0.52; break;
    case 2: $ac_htailarm = $ac_length * 0.50; break;
    case 3: $ac_htailarm = $ac_length * 0.60; break;
    case 4: $ac_htailarm = $ac_length * 0.40; break;
    case 5: $ac_htailarm = $ac_length * 0.40; break;
    case 6: $ac_htailarm = $ac_length * 0.45; break;
    case 7: $ac_htailarm = $ac_length * 0.45; break;
    case 8: $ac_htailarm = $ac_length * 0.45; break;
    case 9: $ac_htailarm = $ac_length * 0.50; break;
   
    }
  }

// estimate vertical tail area
if ($ac_vtailarea == 0) {
  switch($ac_type) {
    case 0: $ac_vtailarea = $ac_wingarea * 0.10; break;
    case 1: $ac_vtailarea = $ac_wingarea * 0.10; break;
    case 2: $ac_vtailarea = $ac_wingarea * 0.18; break;
    case 3: $ac_vtailarea = $ac_wingarea * 0.10; break;
    case 4: $ac_vtailarea = $ac_wingarea * 0.12; break;
    case 5: $ac_vtailarea = $ac_wingarea * 0.18; break;
    case 6: $ac_vtailarea = $ac_wingarea * 0.20; break;
    case 7: $ac_vtailarea = $ac_wingarea * 0.20; break;
    case 8: $ac_vtailarea = $ac_wingarea * 0.20; break;
    case 9: $ac_vtailarea = $ac_wingarea * 0.18; break;
    }
  }

// estimate distance from CG to vertical tail aero center
if ($ac_vtailarm == 0) {
  switch($ac_type) {
    case 0: $ac_vtailarm = $ac_length * 0.60; break;
    case 1: $ac_vtailarm = $ac_length * 0.50; break;
    case 2: $ac_vtailarm = $ac_length * 0.50; break;
    case 3: $ac_vtailarm = $ac_length * 0.60; break;
    case 4: $ac_vtailarm = $ac_length * 0.40; break;
    case 5: $ac_vtailarm = $ac_length * 0.40; break;
    case 6: $ac_vtailarm = $ac_length * 0.45; break;
    case 7: $ac_vtailarm = $ac_length * 0.45; break;
    case 8: $ac_vtailarm = $ac_length * 0.45; break;
    case 9: $ac_vtailarm = $ac_length * 0.50; break;
    }
  }

//***** EMPTY WEIGHT *********************************

// estimate empty weight, based on max weight
if ($ac_emptyweight == 0) {
  switch($ac_type) {
    case 0:  $ac_emptyweight = $ac_weight * .84;
    case 1:  $ac_emptyweight = $ac_weight * .62;
    case 2:  $ac_emptyweight = $ac_weight * .61;
    case 3:  $ac_emptyweight = $ac_weight * .61;
    case 4:  $ac_emptyweight = $ac_weight * .53;
    case 5:  $ac_emptyweight = $ac_weight * .50;
    case 6:  $ac_emptyweight = $ac_weight * .55;
    case 7:  $ac_emptyweight = $ac_weight * .52;
    case 8:  $ac_emptyweight = $ac_weight * .49;
    case 9:  $ac_emptyweight = $ac_weight * .60;
    }
  }
  
//***** MOMENTS OF INERTIA ******************************

// use Roskam's formulae to estimate moments of inertia
switch($ac_type) {  // moment-of-inertia factors 
  case 0: $Rx = 0.34;$Ry = 0.33;$Rz = 0.47; break;
  case 1: $Rx = 0.27;$Ry = 0.36;$Rz = 0.42; break;
  case 2: $Rx = 0.27;$Ry = 0.35;$Rz = 0.45; break;
  case 3: $Rx = 0.27;$Ry = 0.36;$Rz = 0.42; break;
  case 4: $Rx = 0.27;$Ry = 0.35;$Rz = 0.40; break;
  case 5: $Rx = 0.29;$Ry = 0.34;$Rz = 0.41; break;
  case 6: $Rx = 0.25;$Ry = 0.38;$Rz = 0.46; break;
  case 7: $Rx = 0.25;$Ry = 0.36;$Rz = 0.47; break;
  case 8: $Rx = 0.32;$Ry = 0.34;$Rz = 0.47; break;
  case 9: $Rx = 0.32;$Ry = 0.35;$Rz = 0.47; break;
  }

// These are for an empty airplane  
$ac_rawixx = ($ac_emptyweight / 32.2)* pow(($Rx * $ac_wingspan / 2), 2);
$ac_rawiyy = ($ac_emptyweight / 32.2)* pow(($Ry * $ac_length / 2), 2);
$ac_rawizz = ($ac_emptyweight / 32.2)* pow(($Rz * (($ac_wingspan + $ac_length)/2) / 2), 2);
// assume 4 degree angle between longitudinal and inertial axes
// $ac_rawixz = abs($ac_rawizz - $ac_rawixx) * 0.06975647;

$ac_ixx = $ac_rawixx;
$ac_iyy = $ac_rawiyy;
$ac_izz = $ac_rawizz;
$ac_ixz = 0;
$ac_iyz = 0;
$ac_ixy = 0;


//***** CG LOCATION ***********************************

$ac_cglocx = ($ac_length - $ac_htailarm) * 12;
$ac_cglocy = 0;
$ac_cglocz = -($ac_length / 40.0) * 12;

//***** AERO REFERENCE POINT **************************

$ac_aerorpx = $ac_cglocx;
$ac_aerorpy = 0;
$ac_aerorpz = 0;

//***** PILOT EYEPOINT *********************************

// place pilot's eyepoint based on airplane type
switch($ac_type) {
  case 0: $ac_eyeptlocx = ($ac_length * 0.19) * 12; break;
  case 1: $ac_eyeptlocx = ($ac_length * 0.13) * 12; break;
  case 2: $ac_eyeptlocx = ($ac_length * 0.17) * 12; break;
  case 3: $ac_eyeptlocx = ($ac_length * 0.28) * 12; break;
  case 4: $ac_eyeptlocx = ($ac_length * 0.20) * 12; break;
  case 5: $ac_eyeptlocx = ($ac_length * 0.20) * 12; break;
  case 6: $ac_eyeptlocx = ($ac_length * 0.07) * 12; break;
  case 7: $ac_eyeptlocx = ($ac_length * 0.07) * 12; break;
  case 8: $ac_eyeptlocx = ($ac_length * 0.07) * 12; break;
  case 9: $ac_eyeptlocx = ($ac_length * 0.08) * 12; break;
  }

switch($ac_type) {
  case 0: $ac_eyeptlocy =   0; break;
  case 1: $ac_eyeptlocy = -18; break;
  case 2: $ac_eyeptlocy = -18; break;
  case 3: $ac_eyeptlocy =   0; break;
  case 4: $ac_eyeptlocy =   0; break;
  case 5: $ac_eyeptlocy =   0; break;
  case 6: $ac_eyeptlocy = -30; break;
  case 7: $ac_eyeptlocy = -30; break;
  case 8: $ac_eyeptlocy = -32; break;
  case 9: $ac_eyeptlocy = -24; break;
  }

switch($ac_type) {
  case 0: $ac_eyeptlocz =  9; break;
  case 1: $ac_eyeptlocz = 45; break;
  case 2: $ac_eyeptlocz = 45; break;
  case 3: $ac_eyeptlocz = 40; break;
  case 4: $ac_eyeptlocz = 36; break;
  case 5: $ac_eyeptlocz = 38; break;
  case 6: $ac_eyeptlocz = 70; break;
  case 7: $ac_eyeptlocz = 75; break;
  case 8: $ac_eyeptlocz = 80; break;
  case 9: $ac_eyeptlocz = 65; break;
  }

//***** LANDING GEAR *********************************

// set main gear longitudinal location relative to CG
switch($ac_geartype) {
  case 0: $ac_gearlocx_main = $ac_cglocx * 1.04; break;
  case 1: $ac_gearlocx_main = $ac_cglocx * 0.91; break;
  }

// set main gear lateral location
switch($ac_type) {
  case 0: $ac_gearlocy_main = $ac_wingspan * 0.005 * 12; break;
  case 1: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 2: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 3: $ac_gearlocy_main = $ac_wingspan * 0.15 * 12;  break;
  case 4: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 5: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 6: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 7: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 8: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12;  break;
  case 9: $ac_gearlocy_main = $ac_wingspan * 0.11 * 12;  break;
  }

// set main gear length (from aircraft centerline, extended)
switch($ac_geartype) {
  case 0: $ac_gearlocz_main = -($ac_length * 0.12 * 12); break;
  case 1: $ac_gearlocz_main = -($ac_length * 0.20 * 12); break;
  }
if($ac_type == 0) $ac_gearlocz_main = -($ac_length / 10 * 12);  // glider

$ac_gearlocx_nose = $ac_length * 0.13 * 12;
$ac_gearlocy_nose = 0;
$ac_gearlocz_nose = $ac_gearlocz_main;
if($ac_type == 0) $ac_gearlocz_nose = $ac_gearlocz_main * 0.6;  // glider

$ac_gearlocx_tail = $ac_length * 0.91 * 12;
$ac_gearlocy_tail = 0;
$ac_gearlocz_tail = $ac_gearlocz_main * 0.30;

$ac_gearspring_main = $ac_weight * 1.0;
$ac_gearspring_nose = $ac_weight * 0.3;
$ac_gearspring_tail = $ac_weight * 1.0;

$ac_geardamp_main = $ac_weight * 0.5;
$ac_geardamp_nose = $ac_weight * 0.5;
$ac_geardamp_tail = $ac_weight * 0.8;

$ac_geardynamic = 0.5;
$ac_gearstatic  = 0.8;
$ac_bearingstatic = 0.006;	// 2 x 2 x 0.0015, friction for a ball-bearing
$ac_gearrolling = 0.02;
$ac_bearingrolling = 0.003;	// 2 x 0.0015, friction for a ball-bearing
if($ac_type == 0) $ac_gearrolling = 0.5;  // glider

$ac_gearsteerable_nose = 'STEERABLE';
$ac_gearsteerable_main = 'FIXED';
$ac_gearsteerable_tail = 'CASTERED';
$ac_gearmaxsteer = 5;
if($ac_gearretract == 0)
  $ac_retract = '0';
else
  $ac_retract = '1';

//***** PROPULSION ************************************

// spread engines out in reasonable locations

// forward fuselage engines
if ($ac_enginelayout == 0) {
  $leftmost = ($ac_numengines * -20) + 20;
  for( $i=0; $i<$ac_numengines; $i++) {   
      $ac_englocx[$i] = 36.0;
      $ac_englocy[$i] = $leftmost + ($i * 40);
      $ac_englocz[$i] = 0; 
     }
}

// mid fuselage engines
if ($ac_enginelayout == 1) {
  $leftmost = ($ac_numengines * -20) + 20;
  for( $i=0; $i<$ac_numengines; $i++) {   
      $ac_englocx[$i] = $ac_cglocx;
      $ac_englocy[$i] = $leftmost + ($i * 40);
      $ac_englocz[$i] = -12.0; 
     }
} 

// aft fuselage engines
if ($ac_enginelayout == 2) {
  $leftmost = ($ac_numengines * -20) + 20;
  for($i=0; $i<$ac_numengines; $i++) {   
      $ac_englocx[$i] = ($ac_length * 12) - 60.0;
      $ac_englocy[$i] = $leftmost + ($i * 40);
      $ac_englocz[$i] = 0; 
     }
} 

// wing engines (odd one goes in middle)
if ($ac_enginelayout == 3) {
  $halfcount = intval( $ac_numengines / 2 );
  $remainder = $ac_numengines - ($halfcount * 2);
  for($i=0; $i<$halfcount; $i++) {                 //left wing
      $ac_englocx[$i] = $ac_cglocx;
      $ac_englocy[$i] = $ac_wingspan * -2.0;       //span/-2/3*12
      $ac_englocz[$i] = -40; 
     }    
  for($j=$i; $j<$halfcount+$remainder; $j++) {     //center
      $ac_englocx[$j] = $ac_cglocx;
      $ac_englocy[$j] = 0;
      $ac_englocz[$j] = -20; 
     }    
  for($k=$j; $k<$ac_numengines; $k++) {            //right wing
      $ac_englocx[$k] = $ac_cglocx;
      $ac_englocy[$k] = $ac_wingspan * 2.0;        //span/2/3*12
      $ac_englocz[$k] = -40; 
     }    
  }

// wing and tail engines
if ($ac_enginelayout == 4) {
  $halfcount = intval( $ac_numengines / 2 );
  $remainder = $ac_numengines - ($halfcount * 2);
  for($i=0; $i<$halfcount; $i++) {                 //left wing
      $ac_englocx[$i] = $ac_cglocx;
      $ac_englocy[$i] = $ac_wingspan * -2.0;       //span/-2/3*12
      $ac_englocz[$i] = -40; 
     }    
  for($j=$i; $j<$halfcount+$remainder; $j++) {     //center
      $ac_englocx[$j] = $ac_length - 60;
      $ac_englocy[$j] = 0;
      $ac_englocz[$j] = 60; 
     }    
  for($k=$j; $i<$ac_numengines; $i++) {            //right wing
      $ac_englocx[$k] = $ac_cglocx;
      $ac_englocy[$k] = $ac_wingspan * 2.0;        //span/2/3*12 
      $ac_englocz[$k] = -40; 
     }    
  }

// wing and nose engines      
if ($ac_enginelayout == 5) {
  $halfcount = intval( $ac_numengines / 2 );
  $remainder = $ac_numengines - ($halfcount * 2);
  for($i=0; $i<$halfcount; $i++) {                 //left wing
      $ac_englocx[$i] = $ac_cglocx;
      $ac_englocy[$i] = $ac_wingspan * -2.0;       //span/-2/3*12
      $ac_englocz[$i] = -40; 
     }    
  for($j=$i; $j<$halfcount+$remainder; $j++) {     //center
      $ac_englocx[$j] = 36.0;
      $ac_englocy[$j] = 0;
      $ac_englocz[$j] = 0; 
     }    
  for($k=$j; $i<$ac_numengines; $i++) {            //right wing
      $ac_englocx[$k] = $ac_cglocx;
      $ac_englocy[$k] = $ac_wingspan * 2.0;        //span/2/3*12
      $ac_englocz[$k] = -40; 
     }    
  }

// thruster goes where engine is
for($i=0; $i<$ac_numengines; $i++) {
  $ac_engpitch[$i] = 0;
  $ac_engyaw[$i] = 0;
  $ac_engfeed[$i] = $i;
  $ac_thrusterlocx[$i] = $ac_englocx[$i];
  $ac_thrusterlocy[$i] = $ac_englocy[$i];
  $ac_thrusterlocz[$i] = $ac_englocz[$i];
  $ac_thrusterpitch[$i] = 0;
  $ac_thrusteryaw[$i] = 0;
  }

// thruster type (note: only piston engine gets a propeller)
switch($ac_enginetype) {
  case 0: $ac_thrustertype = 'prop';   break;
  case 1: $ac_thrustertype = 'direct'; break;
  case 2: $ac_thrustertype = 'direct'; break;
  case 3: $ac_thrustertype = 'direct'; break;
  }

//***** FUEL TANKS **********************************

// an N-engined airplane will have N+1 fuel tanks
// all tanks located at CG and are half full
$ac_tanklocx = $ac_cglocx;
$ac_tanklocy = $ac_cglocy;
$ac_tanklocz = $ac_cglocz;
$ac_tankradius = 1;
switch($ac_type) {  // capacity in pounds
  case 0: $ac_tankcapacity =    0; break;
  case 1: $ac_tankcapacity =  100; break;
  case 2: $ac_tankcapacity =  300; break;
  case 3: $ac_tankcapacity = 1000; break;
  case 4: $ac_tankcapacity = 3000; break;
  case 5: $ac_tankcapacity = 4500; break;
  case 6: $ac_tankcapacity = $ac_weight/23.0/($ac_numengines + 1); break;
  case 7: $ac_tankcapacity = $ac_weight/16.0/($ac_numengines + 1); break;
  case 8: $ac_tankcapacity = $ac_weight/16.0/($ac_numengines + 1); break;
  case 9: $ac_tankcapacity = $ac_weight/18.0/($ac_numengines + 1); break;
  }
$ac_tankcontents = $ac_tankcapacity/2;

//***** LIFT ****************************************

// estimate slope of lift curve based on airplane type
// units: per radian
switch($ac_type) {
  case 0: $ac_CLalpha = 5.5; break;
  case 1: $ac_CLalpha = 5.0; break;
  case 2: $ac_CLalpha = 4.8; break;
  case 3: $ac_CLalpha = 4.5; break;
  case 4: $ac_CLalpha = 3.5; break;
  case 5: $ac_CLalpha = 3.6; break;
  case 6: $ac_CLalpha = 4.4; break;
  case 7: $ac_CLalpha = 4.4; break;
  case 8: $ac_CLalpha = 4.4; break;
  case 9: $ac_CLalpha = 4.9; break;
  }

// estimate CL at zero alpha
switch($ac_type) {
  case 0: $ac_CL0 = 0.25; break;
  case 1: $ac_CL0 = 0.25; break;
  case 2: $ac_CL0 = 0.24; break;
  case 3: $ac_CL0 = 0.17; break;
  case 4: $ac_CL0 = 0.08; break;
  case 5: $ac_CL0 = 0.08; break;
  case 6: $ac_CL0 = 0.20; break;
  case 7: $ac_CL0 = 0.20; break;
  case 8: $ac_CL0 = 0.20; break;
  case 9: $ac_CL0 = 0.24; break;
  }

// estimate stall CL, based on airplane type
switch($ac_type) {
  case 0: $ac_CLmax = 1.40; break;
  case 1: $ac_CLmax = 1.40; break;
  case 2: $ac_CLmax = 1.30; break;
  case 3: $ac_CLmax = 1.20; break;
  case 4: $ac_CLmax = 1.00; break;
  case 5: $ac_CLmax = 1.00; break;
  case 6: $ac_CLmax = 1.20; break;
  case 7: $ac_CLmax = 1.20; break;
  case 8: $ac_CLmax = 1.20; break;
  case 9: $ac_CLmax = 1.40; break;
  }

// estimate delta-CL-flaps, based on airplane type
switch($ac_type) {
  case 0: $ac_dCLflaps = 0.200; break;
  case 1: $ac_dCLflaps = 0.400; break;
  case 2: $ac_dCLflaps = 0.400; break;
  case 3: $ac_dCLflaps = 0.300; break;
  case 4: $ac_dCLflaps = 0.350; break;
  case 5: $ac_dCLflaps = 0.350; break;
  case 6: $ac_dCLflaps = 1.500; break;
  case 7: $ac_dCLflaps = 1.500; break;
  case 8: $ac_dCLflaps = 1.500; break;
  case 9: $ac_dCLflaps = 0.600; break;
  }

// some types have speedbrakes in wings, affecting lift
switch($ac_type) {
  case 0: $ac_dCLspeedbrake = -0.05; break;
  case 1: $ac_dCLspeedbrake =  0.00; break;
  case 2: $ac_dCLspeedbrake =  0.00; break;
  case 3: $ac_dCLspeedbrake =  0.00; break;
  case 4: $ac_dCLspeedbrake =  0.00; break;
  case 5: $ac_dCLspeedbrake =  0.00; break;
  case 6: $ac_dCLspeedbrake = -0.10; break;
  case 7: $ac_dCLspeedbrake = -0.09; break;
  case 8: $ac_dCLspeedbrake = -0.08; break;
  case 9: $ac_dCLspeedbrake =  0.00; break;
  }

// estimate lift due to elevator deflection
$ac_CLde = 0.2;

//***** DRAG *****************************************

// estimate drag at zero lift, based on airplane type
// NOT including landing gear
switch($ac_type) {
  case 0: $ac_CD0 = 0.010; break;
  case 1: $ac_CD0 = 0.024; break;
  case 2: $ac_CD0 = 0.025; break;
  case 3: $ac_CD0 = 0.020; break;
  case 4: $ac_CD0 = 0.021; break;
  case 5: $ac_CD0 = 0.024; break;
  case 6: $ac_CD0 = 0.020; break;
  case 7: $ac_CD0 = 0.019; break;
  case 8: $ac_CD0 = 0.017; break;
  case 9: $ac_CD0 = 0.025; break;
  }

// add gear drag if fixed gear
if($ac_gearretract == 0) {
switch($ac_type) {
  case 0: $ac_CD0 += 0.002; break;
  case 1: $ac_CD0 += 0.004; break;
  case 2: $ac_CD0 += 0.004; break;
  case 3: $ac_CD0 += 0.004; break;
  case 4: $ac_CD0 += 0.005; break;
  case 5: $ac_CD0 += 0.005; break;
  case 6: $ac_CD0 += 0.002; break;
  case 7: $ac_CD0 += 0.002; break;
  case 8: $ac_CD0 += 0.002; break;
  case 9: $ac_CD0 += 0.003; break;
  }
}

// estimate induced drag coefficient K
switch($ac_type) {
  case 0: $ac_K = 0.023; break;
  case 1: $ac_K = 0.040; break;
  case 2: $ac_K = 0.041; break;
  case 3: $ac_K = 0.060; break;
  case 4: $ac_K = 0.120; break;
  case 5: $ac_K = 0.120; break;
  case 6: $ac_K = 0.043; break;
  case 7: $ac_K = 0.042; break;
  case 8: $ac_K = 0.042; break;
  case 9: $ac_K = 0.039; break;
  }

// CD flaps
switch($ac_type) {
  case 0: $ac_CDflaps = 0.024; break;
  case 1: $ac_CDflaps = 0.030; break;
  case 2: $ac_CDflaps = 0.039; break;
  case 3: $ac_CDflaps = 0.040; break;
  case 4: $ac_CDflaps = 0.080; break;
  case 5: $ac_CDflaps = 0.075; break;
  case 6: $ac_CDflaps = 0.059; break;
  case 7: $ac_CDflaps = 0.057; break;
  case 8: $ac_CDflaps = 0.055; break;
  case 9: $ac_CDflaps = 0.035; break;
  }

// estimate drag from landing gear down
switch($ac_type) {
  case 0: $ac_CDgear = 0.012; break;
  case 1: $ac_CDgear = 0.030; break;
  case 2: $ac_CDgear = 0.030; break;
  case 3: $ac_CDgear = 0.030; break;
  case 4: $ac_CDgear = 0.020; break;
  case 5: $ac_CDgear = 0.020; break;
  case 6: $ac_CDgear = 0.015; break;
  case 7: $ac_CDgear = 0.013; break;
  case 8: $ac_CDgear = 0.011; break;
  case 9: $ac_CDgear = 0.023; break;
  }

$ac_CDde = 0.04;             // elevator deflection
$ac_CDbeta = 0.2;            // sideslip
$ac_CDspeedbrake = $ac_CD0;  // speedbrake

// estimate critical mach, based on airplane type
switch($ac_type) {
  case 0: $ac_Mcrit = 0.70; break;
  case 1: $ac_Mcrit = 0.70; break;
  case 2: $ac_Mcrit = 0.72; break;
  case 3: $ac_Mcrit = 0.75; break;
  case 4: $ac_Mcrit = 0.81; break;
  case 5: $ac_Mcrit = 0.81; break;
  case 6: $ac_Mcrit = 0.79; break;
  case 7: $ac_Mcrit = 0.79; break;
  case 8: $ac_Mcrit = 0.79; break;
  case 9: $ac_Mcrit = 0.70; break;
  }

//***** SIDE *************************************

// estimate side force due to sideslip (beta)
$ac_CYbeta = -1;

//***** ROLL *************************************

// estimate roll coefficients
$ac_Clbeta = -0.1;     // sideslip
$ac_Clp = -0.4;        // roll rate
$ac_Clr = 0.15;        // yaw rate
switch($ac_type) {     // aileron
  case 0: $ac_Clda = 0.06; break;
  case 1: $ac_Clda = 0.17; break;
  case 2: $ac_Clda = 0.17; break;
  case 3: $ac_Clda = 0.18; break;
  case 4: $ac_Clda = 0.11; break;
  case 5: $ac_Clda = 0.12; break;
  case 6: $ac_Clda = 0.10; break;
  case 7: $ac_Clda = 0.10; break;
  case 8: $ac_Clda = 0.10; break;
  case 9: $ac_Clda = 0.15; break;
  }
$ac_Cldr = 0.01;       // rudder deflection

//***** PITCH ************************************

// estimate pitch coefficients
switch($ac_type) {
  case  0: // glider
    $ac_Cmalpha = -0.5;    // per radian alpha
    $ac_Cmde = -0.8;       // elevator deflection
    $ac_Cmq = -9.0;        // pitch rate
    $ac_Cmadot = -12.0;    // alpha-dot
    break;
  case  1: // light single
    $ac_Cmalpha = -0.5;    // per radian alpha
    $ac_Cmde = -1.1;       // elevator deflection
    $ac_Cmq = -12.0;       // pitch rate
    $ac_Cmadot = -7.0;     // alpha-dot
    break;
  case  2: // light twin
    $ac_Cmalpha = -0.4;    // per radian alpha
    $ac_Cmde = -1.0;       // elevator deflection
    $ac_Cmq = -22.0;       // pitch rate
    $ac_Cmadot = -8.0;     // alpha-dot
    break;
  case  3: // WWII fighter/racer/aerobatic
    $ac_Cmalpha = -0.5;    // per radian alpha
    $ac_Cmde = -1.0;       // elevator deflection
    $ac_Cmq = -15.0;       // pitch rate
    $ac_Cmadot = -7.0;     // alpha-dot
    break;
  case  4: // single engine jet fighter
    $ac_Cmalpha = -0.3;    // per radian alpha
    $ac_Cmde = -0.8;       // elevator deflection
    $ac_Cmq = -18.0;       // pitch rate
    $ac_Cmadot = -9.0;     // alpha-dot
    break;
  case  5: // two engine jet fighter
    $ac_Cmalpha = -0.3;    // per radian alpha
    $ac_Cmde = -0.8;       // elevator deflection
    $ac_Cmq = -18.0;       // pitch rate
    $ac_Cmadot = -9.0;     // alpha-dot
    break;
  case  6: // two engine jet transport
    $ac_Cmalpha = -0.6;    // per radian alpha
    $ac_Cmde = -1.2;       // elevator deflection
    $ac_Cmq = -17.0;       // pitch rate
    $ac_Cmadot = -6.0;     // alpha-dot
    break;
  case  7: // three engine jet transport
    $ac_Cmalpha = -0.6;    // per radian alpha
    $ac_Cmde = -1.2;       // elevator deflection
    $ac_Cmq = -17.0;       // pitch rate
    $ac_Cmadot = -6.0;     // alpha-dot
    break;
  case  8: // four+ engine jet transport
    $ac_Cmalpha = -0.7;    // per radian alpha
    $ac_Cmde = -1.3;       // elevator deflection
    $ac_Cmq = -21.0;       // pitch rate
    $ac_Cmadot = -4.0;     // alpha-dot
    break;
  case  9: // multi-engine prop transport
    $ac_Cmalpha = -0.4;    // per radian alpha
    $ac_Cmde = -1.0;       // elevator deflection
    $ac_Cmq = -22.0;       // pitch rate
    $ac_Cmadot = -8.0;     // alpha-dot
    break;
  }

//***** YAW **************************************

// estimate yaw coefficients
$ac_Cnbeta = 0.12;     // sideslip
$ac_Cnr = -0.15;       // yaw rate
$ac_Cndr = -0.10;      // rudder deflection
if($ac_type == 0) $ac_Cndr = -0.03;  // glider

switch($ac_type) {                   // adverse yaw
  case 0: $ac_Cnda = -0.02;  break;
  case 1: $ac_Cnda = -0.01;  break;
  case 2: $ac_Cnda = -0.01;  break;
  case 3: $ac_Cnda = -0.003; break;
  case 4: $ac_Cnda =  0.0;   break;
  case 5: $ac_Cnda =  0.0;   break;
  case 6: $ac_Cnda =  0.0;   break;
  case 7: $ac_Cnda =  0.0;   break;
  case 8: $ac_Cnda =  0.0;   break;
  case 9: $ac_Cnda = -0.008; break;
  }

// get the server date/time
$date_string = date('Y-m-d');  

//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************

print("<?xml version=\"1.0\"?>\n");
print("<?xml-stylesheet type=\"text/xsl\" href=\"http://jsbsim.sourceforge.net/JSBSim.xsl\"?>\n");
print("<fdm_config name=\"$ac_name\" version=\"2.0\" release=\"ALPHA\"\n");
print("   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
print("   xsi:noNamespaceSchemaLocation=\"http://jsbsim.sourceforge.net/JSBSim.xsd\">\n\n");

print(" <fileheader>\n");
print("  <author> Aeromatic v $version </author>\n");
print("  <filecreationdate>$date_string</filecreationdate>\n");
print("  <version>\$Revision: 1.13 $</version>\n");
print("  <description> Models a $ac_name. </description>\n");
print(" </fileheader>\n\n");
 
print("<!--\n  File:     $ac_name.xml\n");
print("  Inputs:\n");
print("    name:          $ac_name\n");
switch($ac_type) {
  case 0: print("    type:          glider\n"); break;
  case 1: print("    type:          light single\n"); break;
  case 2: print("    type:          light twin\n"); break;
  case 3: print("    type:          WWII fighter, subsonic sport, aerobatic\n"); break;
  case 4: print("    type:          single-engine transonic/supersonic fighter\n"); break;
  case 5: print("    type:          two-engine transonic/supersonic fighter\n"); break;
  case 6: print("    type:          two-engine transonic transport\n"); break;
  case 7: print("    type:          three-engine transonic transport\n"); break;
  case 8: print("    type:          four-engine transonic transport\n"); break;
  case 9: print("    type:          multi-engine prop transport\n"); break;
  }
print("    max weight:    $ac_weight lb\n");
print("    wing span:     $ac_wingspan ft\n");
print("    length:        $ac_length ft\n");
if($wingarea_input == false)
  print("    wing area:     unspecified\n");
else
  print("    wing area:     $ac_wingarea sq-ft\n");
switch($ac_geartype) {
  case 0: print("    gear type:     tricycle\n"); break;
  case 1: print("    gear type:     taildragger\n"); break; 
}
switch($ac_gearretract) {
  case 0: print("    retractable?:  no\n"); break;
  case 1: print("    retractable?:  yes\n"); break; 
}
print("    # engines:     $ac_numengines\n");
switch($ac_enginetype) {
  case 0: print("    engine type:   piston\n"); break; 
  case 1: print("    engine type:   turbine\n"); break; 
  case 2: print("    engine type:   turboprop\n"); break; 
  case 3: print("    engine type:   rocket\n"); break; 
}
switch($ac_enginelayout) {
  case 0: print("    engine layout: forward fuselage\n"); break; 
  case 1: print("    engine layout: middle fuselage\n"); break; 
  case 2: print("    engine layout: aft fuselage\n"); break; 
  case 3: print("    engine layout: wings\n"); break; 
  case 4: print("    engine layout: wings and tail\n"); break; 
  case 5: print("    engine layout: wings and nose\n"); break; 
}
if($ac_yawdamper)
  print("    yaw damper?    yes\n");
else
  print("    yaw damper?    no\n\n");
print("  Outputs:\n");
printf("    wing loading:  %2.2f lb/sq-ft\n", $ac_wingloading);
print("    CL-alpha:      $ac_CLalpha per radian\n");
print("    CL-0:          $ac_CL0\n");
print("    CL-max:        $ac_CLmax\n");
print("    CD-0:          $ac_CD0\n");
print("    K:             $ac_K"); 
print("\n-->\n\n"); 

//***** METRICS **********************************

print(" <metrics>\n");
printf("   <wingarea  unit=\"FT2\"> %7.2f </wingarea>\n", $ac_wingarea);
printf("   <wingspan  unit=\"FT\" > %7.2f </wingspan>\n", $ac_wingspan);
printf("   <wing_incidence>       %7.2f </wing_incidence>\n", $ac_wingincidence);
printf("   <chord     unit=\"FT\" > %7.2f </chord>\n", $ac_wingchord);
printf("   <htailarea unit=\"FT2\"> %7.2f </htailarea>\n", $ac_htailarea);
printf("   <htailarm  unit=\"FT\" > %7.2f </htailarm>\n", $ac_htailarm);
printf("   <vtailarea unit=\"FT2\"> %7.2f </vtailarea>\n", $ac_vtailarea);
printf("   <vtailarm  unit=\"FT\" > %7.2f </vtailarm>\n", $ac_vtailarm);
print("   <location name=\"AERORP\" unit=\"IN\">\n");
printf("     <x> %6.2f </x>\n", $ac_aerorpx);
printf("     <y> %6.2f </y>\n", $ac_aerorpy);
printf("     <z> %6.2f </z>\n", $ac_aerorpz);
print("   </location>\n");
print("   <location name=\"EYEPOINT\" unit=\"IN\">\n");
printf("     <x> %6.2f </x>\n", $ac_eyeptlocx);
printf("     <y> %6.2f </y>\n", $ac_eyeptlocy);
printf("     <z> %6.2f </z>\n", $ac_eyeptlocz);
print("   </location>\n");
print("   <location name=\"VRP\" unit=\"IN\">\n");
print("     <x>0</x>\n");
print("     <y>0</y>\n");
print("     <z>0</z>\n");
print("   </location>\n");
print(" </metrics>\n\n");

print(" <mass_balance>\n");
printf("   <ixx unit=\"SLUG*FT2\">  %8.0f </ixx>\n", $ac_ixx);
printf("   <iyy unit=\"SLUG*FT2\">  %8.0f </iyy>\n", $ac_iyy);
printf("   <izz unit=\"SLUG*FT2\">  %8.0f </izz>\n", $ac_izz);
//printf("   <ixy unit=\"SLUG*FT2\">  %8.0f </ixy>\n", $ac_ixy);
//printf("   <ixz unit=\"SLUG*FT2\">  %8.0f </ixz>\n", $ac_ixz);
//printf("   <iyz unit=\"SLUG*FT2\">  %8.0f </iyz>\n", $ac_iyz);
printf("   <emptywt unit=\"LBS\" >  %8.0f </emptywt>\n", $ac_emptyweight);
print("   <location name=\"CG\" unit=\"IN\">\n");
printf("     <x> %6.2f </x>\n", $ac_cglocx);
printf("     <y> %6.2f </y>\n", $ac_cglocy);
printf("     <z> %6.2f </z>\n", $ac_cglocz);
print("   </location>\n");
print(" </mass_balance>\n\n");


//***** LANDING GEAR ******************************

print(" <ground_reactions>\n\n");

if($ac_type == 0) {  // if this is a glider

  print("  <contact type=\"BOGEY\" name=\"LEFT_MAIN\">\n");
  print("    <location unit=\"IN\">\n");
  printf("      <x> %6.2f </x>\n", $ac_gearlocx_main);
  printf("      <y> %6.2f </y>\n", -$ac_gearlocy_main);
  printf("      <z> %6.2f </z>\n", $ac_gearlocz_main);
  print("    </location>\n");
  printf("    <static_friction>  %4.3f </static_friction>\n", $ac_gearstatic);
  printf("    <dynamic_friction> %4.3f </dynamic_friction>\n", $ac_geardynamic);
  printf("    <rolling_friction> %4.3f </rolling_friction>\n", $ac_gearrolling);
  printf("    <spring_coeff  unit=\"LBS/FT\">     %8.2f </spring_coeff>\n", $ac_gearspring_main);
  printf("    <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_main);
  print("    <max_steer unit=\"DEG\">0</max_steer>\n");
  print("    <brake_group>NONE</brake_group>\n");
  print("    <retractable>$ac_retract</retractable>\n");
  print("  </contact>\n\n");
  
  print("  <contact type=\"BOGEY\" name=\"RIGHT_MAIN\">\n");
  print("    <location unit=\"IN\">\n");
  printf("     <x> %6.2f </x>\n", $ac_gearlocx_main);
  printf("     <y> %6.2f </y>\n", $ac_gearlocy_main);
  printf("     <z> %6.2f </z>\n", $ac_gearlocz_main);
  print("   </location>\n");
  printf("   <static_friction>  %4.3f </static_friction>\n", $ac_gearstatic);
  printf("   <dynamic_friction> %4.3f </dynamic_friction>\n", $ac_geardynamic);
  printf("   <rolling_friction> %4.3f </rolling_friction>\n", $ac_gearrolling);
  printf("   <spring_coeff unit=\"LBS/FT\">     %8.2f </spring_coeff>\n", $ac_gearspring_main);
  printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_main);
  print("   <max_steer unit=\"DEG\">0</max_steer>\n");
  print("   <brake_group>NONE</brake_group>\n");
  print("   <retractable>$ac_retract</retractable>\n");
  print("  </contact>\n\n");

  print("  <contact type=\"BOGEY\" name=\"NOSE\">\n");
  print("    <location unit=\"IN\">\n");
  printf("     <x> %6.2f </x>\n", $ac_gearlocx_nose);
  printf("     <y> %6.2f </y>\n", $ac_gearlocy_nose);
  printf("     <z> %6.2f </z>\n", $ac_gearlocz_nose);
  print("   </location>\n");
  printf("   <static_friction>  %4.3f </static_friction>\n", $ac_gearstatic);
  printf("   <dynamic_friction> %4.3f </dynamic_friction>\n", $ac_geardynamic);
  printf("   <rolling_friction> %4.3f </rolling_friction>\n", $ac_gearrolling);
  printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_nose);
  printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_nose);
  print("   <max_steer unit=\"DEG\">0</max_steer>\n");
  print("   <brake_group>NONE</brake_group>\n");
  print("   <retractable>$ac_retract</retractable>\n");
  print("  </contact>\n\n");

  print("  <contact type=\"STRUCTURE\" name=\"LEFT_WING\">\n");
  print("    <location unit=\"IN\">\n");
  printf("     <x> %6.2f </x>\n", $ac_cglocx);
  printf("     <y> %6.2f </y>\n", -$ac_halfspan);
  printf("     <z> %6.2f </z>\n", $ac_cglocz);
  print("   </location>\n");
  printf("   <static_friction>  %2.2f </static_friction>\n", 1.0);
  printf("   <dynamic_friction> %2.2f </dynamic_friction>\n", 1.0);
  printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_main);
  printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_gearspring_main);
  print("  </contact>\n\n");

  print("  <contact type=\"STRUCTURE\" name=\"RIGHT_WING\">\n");
  print("    <location unit=\"IN\">\n");
  printf("     <x> %6.2f </x>\n", $ac_cglocx);
  printf("     <y> %6.2f </y>\n", $ac_halfspan);
  printf("     <z> %6.2f </z>\n", $ac_cglocz);
  print("   </location>\n");
  printf("   <static_friction>  %2.2f </static_friction>\n", 1.0);
  printf("   <dynamic_friction> %2.2f </dynamic_friction>\n", 1.0);
  printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_main);
  printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_gearspring_main);
  print("  </contact>\n\n");
 }
 else {
   if ($ac_geartype == 0) {  // if this is a tricycle gear

    print("  <contact type=\"BOGEY\" name=\"NOSE\">\n");
    print("   <location unit=\"IN\">\n");
    printf("     <x> %6.2f </x>\n", $ac_gearlocx_nose);
    printf("     <y> %6.2f </y>\n", $ac_gearlocy_nose);
    printf("     <z> %6.2f </z>\n", $ac_gearlocz_nose);
    print("   </location>\n");
    printf("   <static_friction>  %2.2f </static_friction>\n", $ac_gearstatic);
    printf("   <dynamic_friction> %2.2f </dynamic_friction>\n", $ac_geardynamic);
    printf("   <rolling_friction> %2.2f </rolling_friction>\n", $ac_gearrolling);
    printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_nose);
    printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_nose);
    printf("   <max_steer unit=\"DEG\"> %2.2f </max_steer>\n", $ac_gearmaxsteer);
    print("   <brake_group>NONE</brake_group>\n");
    print("   <retractable>$ac_retract</retractable>\n");
    print("  </contact>\n\n");

   }

    print("  <contact type=\"BOGEY\" name=\"LEFT_MAIN\">\n");
    print("   <location unit=\"IN\">\n");
    printf("     <x> %6.2f </x>\n", $ac_gearlocx_main);
    printf("     <y> %6.2f </y>\n", -$ac_gearlocy_main);
    printf("     <z> %6.2f </z>\n", $ac_gearlocz_main);
    print("   </location>\n");
    printf("   <static_friction>  %2.2f </static_friction>\n", $ac_gearstatic);
    printf("   <dynamic_friction> %2.2f </dynamic_friction>\n", $ac_geardynamic);
    printf("   <rolling_friction> %2.2f </rolling_friction>\n", $ac_gearrolling);
    printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_main);
    printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_main);
    print("   <max_steer unit=\"DEG\">0</max_steer>\n");
    print("   <brake_group>LEFT</brake_group>\n");
    print("   <retractable>$ac_retract</retractable>\n");
    print("  </contact>\n\n");
  
    print("  <contact type=\"BOGEY\" name=\"RIGHT_MAIN\">\n");
    print("   <location unit=\"IN\">\n");
    printf("     <x> %6.2f </x>\n", $ac_gearlocx_main);
    printf("     <y> %6.2f </y>\n", $ac_gearlocy_main);
    printf("     <z> %6.2f </z>\n", $ac_gearlocz_main);
    print("   </location>\n");
    printf("   <static_friction>  %2.2f </static_friction>\n", $ac_gearstatic);
    printf("   <dynamic_friction> %2.2f </dynamic_friction>\n", $ac_geardynamic);
    printf("   <rolling_friction> %2.2f </rolling_friction>\n", $ac_gearrolling);
    printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_main);
    printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_main);
    print("   <max_steer unit=\"DEG\">0</max_steer>\n");
    print("   <brake_group>RIGHT</brake_group>\n");
    print("   <retractable>$ac_retract</retractable>\n");
    print("  </contact>\n\n");

   if ($ac_geartype == 1) {  // if this is a taildragger

    print("  <contact type=\"BOGEY\" name=\"TAIL\">\n");
    print("   <location unit=\"IN\">\n");
    printf("     <x> %6.2f </x>\n", $ac_gearlocx_tail);
    printf("     <y> %6.2f </y>\n", $ac_gearlocy_tail);
    printf("     <z> %6.2f </z>\n", $ac_gearlocz_tail);
    print("   </location>\n");
    printf("   <static_friction>  %2.2f </static_friction>\n", $ac_gearstatic);
    printf("   <dynamic_friction> %2.2f </dynamic_friction>\n", $ac_geardynamic);
    printf("   <rolling_friction> %2.2f </rolling_friction>\n", $ac_gearrolling);
    printf("   <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_tail);
    printf("   <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_geardamp_tail);
    printf("   <max_steer unit=\"DEG\"> %2.2f </max_steer>\n", $ac_gearmaxsteer);
    print("   <brake_group>NONE</brake_group>\n");
    print("   <retractable>$ac_retract</retractable>\n");
    print("  </contact>\n\n");

   }

    print("  <contact type=\"STRUCTURE\" name=\"LEFT_WING\">\n");
    print("    <location unit=\"IN\">\n");
    printf("     <x> %6.2f </x>\n", $ac_cglocx);
    printf("     <y> %6.2f </y>\n", -$ac_halfspan);
    printf("     <z> %6.2f </z>\n", $ac_cglocz);
    print("    </location>\n");
    printf("    <static_friction>  %2.2f </static_friction>\n", 1.0);
    printf("    <dynamic_friction> %2.2f </dynamic_friction>\n", 1.0);
    printf("    <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_main);
    printf("    <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_gearspring_main);
    print("  </contact>\n\n");

    print("  <contact type=\"STRUCTURE\" name=\"RIGHT_WING\">\n");
    print("    <location unit=\"IN\">\n");
    printf("     <x> %6.2f </x>\n", $ac_cglocx);
    printf("     <y> %6.2f </y>\n", $ac_halfspan);
    printf("     <z> %6.2f </z>\n", $ac_cglocz);
    print("    </location>\n");
    printf("    <static_friction>  %2.2f </static_friction>\n", 1.0);
    printf("    <dynamic_friction> %2.2f </dynamic_friction>\n", 1.0);
    printf("    <spring_coeff unit=\"LBS/FT\">      %8.2f </spring_coeff>\n", $ac_gearspring_main);
    printf("    <damping_coeff unit=\"LBS/FT/SEC\"> %8.2f </damping_coeff>\n", $ac_gearspring_main);
    print("  </contact>\n\n");  
 }
print(" </ground_reactions>\n\n");


//***** PROPULSION ***************************************

$ac_engine_name = $ac_name . '_engine';
$ac_prop_name = $ac_name . '_prop';

print(" <propulsion>\n\n");
if($ac_type == 0) { // if glider, do nothing here
 }
 else {
 for($i=0; $i<$ac_numengines; $i++) {

  print("   <engine file=\"$ac_engine_name\">\n");
  print("    <location unit=\"IN\">\n");
  printf("      <x> %6.2f </x>\n", $ac_englocx[$i]);
  printf("      <y> %6.2f </y>\n", $ac_englocy[$i]);
  printf("      <z> %6.2f </z>\n", $ac_englocz[$i]);
  print("    </location>\n");
  print("    <orient unit=\"DEG\">\n");
  printf("      <pitch> %2.2f </pitch>\n", $ac_engpitch[$i]);
  print("      <roll>  0.00 </roll>\n");
  printf("      <yaw>   %2.2f </yaw>\n", $ac_engyaw[$i]);
  print("    </orient>\n");
  print("    <feed>$ac_engfeed[$i]</feed>\n");

  if($ac_enginetype == 0) {
    print("    <thruster file=\"$ac_prop_name\">\n");
    }
    else {
    print("    <thruster file=\"direct\">\n");
    }
  print("     <location unit=\"IN\">\n");
  printf("       <x> %6.2f </x>\n", $ac_thrusterlocx[$i]);
  printf("       <y> %6.2f </y>\n", $ac_thrusterlocy[$i]);
  printf("       <z> %6.2f </z>\n", $ac_thrusterlocz[$i]);
  print("     </location>\n");
  print("     <orient unit=\"DEG\">\n");
  printf("       <pitch> %2.2f </pitch>\n", $ac_thrusterpitch[$i]);
  print("       <roll>  0.00 </roll>\n");
  printf("       <yaw>   %2.2f </yaw>\n", $ac_thrusteryaw[$i]);
  print("     </orient>\n");

  print("    </thruster>\n");
  print("  </engine>\n\n");
  }

 //***** FUEL TANKS **************************************

 for($i=0; $i<($ac_numengines + 1); $i++) {
  print("  <tank type=\"FUEL\" number=\"$i\">\n");
  print("     <location unit=\"IN\">\n");
  printf("       <x> %6.2f </x>\n", $ac_tanklocx);
  printf("       <y> %6.2f </y>\n", $ac_tanklocy);
  printf("       <z> %6.2f </z>\n", $ac_tanklocz);
  print("     </location>\n");
  printf("     <capacity unit=\"LBS\"> %6.2f </capacity>\n", $ac_tankcapacity);
  printf("     <contents unit=\"LBS\"> %6.2f </contents>\n", $ac_tankcontents);
  print("  </tank>\n\n");
 }
}
print(" </propulsion>\n\n");


//***** FLIGHT CONTROL SYSTEM ***************************

print(" <flight_control name=\"FCS: $ac_name\">\n\n");

print("  <channel name=\"Pitch\">\n\n");

print("   <summer name=\"Pitch Trim Sum\">\n");
print("      <input>fcs/elevator-cmd-norm</input>\n");
print("      <input>fcs/pitch-trim-cmd-norm</input>\n");
print("      <clipto>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </clipto>\n");
print("   </summer>\n\n");

print("   <aerosurface_scale name=\"Elevator Control\">\n");
print("      <input>fcs/pitch-trim-sum</input>\n");
print("      <range>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </range>\n");
print("      <output>fcs/elevator-pos-rad</output>\n");
print("   </aerosurface_scale>\n\n");

print("   <aerosurface_scale name=\"elevator normalization\">\n");
print("      <input>fcs/elevator-pos-rad</input>\n");
print("      <domain>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </domain>\n");
print("      <range>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </range>\n");
print("      <output>fcs/elevator-pos-norm</output>\n");
print("   </aerosurface_scale>\n\n");

print("  </channel>\n\n");
print("  <channel name=\"Roll\">\n\n");

print("   <summer name=\"Roll Trim Sum\">\n");
print("      <input>fcs/aileron-cmd-norm</input>\n");
print("      <input>fcs/roll-trim-cmd-norm</input>\n");
print("      <clipto>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </clipto>\n");
print("   </summer>\n\n");

print("   <aerosurface_scale name=\"Left Aileron Control\">\n");
print("      <input>fcs/roll-trim-sum</input>\n");
print("      <range>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </range>\n");
print("      <output>fcs/left-aileron-pos-rad</output>\n");
print("   </aerosurface_scale>\n\n");

print("   <aerosurface_scale name=\"Right Aileron Control\">\n");
print("      <input>fcs/roll-trim-sum</input>\n");
print("      <range>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </range>\n");
print("      <output>fcs/right-aileron-pos-rad</output>\n");
print("   </aerosurface_scale>\n\n");

print("   <aerosurface_scale name=\"left aileron normalization\">\n");
print("      <input>fcs/left-aileron-pos-rad</input>\n");
print("      <domain>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </domain>\n");
print("      <range>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </range>\n");
print("      <output>fcs/left-aileron-pos-norm</output>\n");
print("   </aerosurface_scale>\n\n");

print("   <aerosurface_scale name=\"right aileron normalization\">\n");
print("      <input>fcs/right-aileron-pos-rad</input>\n");
print("      <domain>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </domain>\n");
print("      <range>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </range>\n");
print("      <output>fcs/right-aileron-pos-norm</output>\n");
print("   </aerosurface_scale>\n\n");

print("  </channel>\n\n");
if($ac_yawdamper == 1) {
  print("  <property value=\"1\">fcs/yaw-damper-enable</property>\n");
}
print("  <channel name=\"Yaw\">\n\n");

print("   <summer name=\"Rudder Command Sum\">\n");
print("      <input>fcs/rudder-cmd-norm</input>\n");
print("      <input>fcs/yaw-trim-cmd-norm</input>\n");
print("      <clipto>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </clipto>\n");
print("   </summer>\n\n");

if($ac_yawdamper == 1) {
  print("   <scheduled_gain name=\"Yaw Damper Rate\">\n");
  print("      <input>velocities/r-aero-rad_sec</input>\n");
  print("      <table>\n");
  print("        <independentVar lookup=\"row\">velocities/ve-kts</independentVar>\n");
  print("         <tableData>\n");
  print("            30     0.00\n");
  print("            60     2.00\n");
  print("         </tableData>\n");
  print("      </table>\n");
  print("      <gain>fcs/yaw-damper-enable</gain>\n");
  print("   </scheduled_gain>\n\n");

  print("   <summer name=\"Rudder Sum\">\n");
  print("      <input>fcs/rudder-command-sum</input>\n");
  print("      <input>fcs/yaw-damper-rate</input>\n");
  print("      <clipto>\n");
  print("        <min> -1.1 </min>\n");
  print("        <max>  1.1 </max>\n");
  print("      </clipto>\n");
  print("   </summer>\n\n");

  print("   <aerosurface_scale name=\"Rudder Control\">\n");
  print("      <input>fcs/rudder-sum</input>\n");
  print("      <domain>\n");
  print("        <min> -1.1 </min>\n");
  print("        <max>  1.1 </max>\n");
  print("      </domain>\n");
  print("      <range>\n");
  print("        <min> -0.35 </min>\n");
  print("        <max>  0.35 </max>\n");
  print("      </range>\n");
  print("      <output>fcs/rudder-pos-rad</output>\n");
  print("   </aerosurface_scale>\n\n"); 
  }
  else {
   print("   <aerosurface_scale name=\"Rudder Control\">\n");
   print("      <input>fcs/rudder-command-sum</input>\n");
   print("      <range>\n");
   print("        <min> -0.35 </min>\n");
   print("        <max>  0.35 </max>\n");
   print("      </range>\n");
   print("      <output>fcs/rudder-pos-rad</output>\n");
   print("   </aerosurface_scale>\n\n"); 
  }

print("   <aerosurface_scale name=\"rudder normalization\">\n");
print("      <input>fcs/rudder-pos-rad</input>\n");
print("      <domain>\n");
print("        <min> -0.35 </min>\n");
print("        <max>  0.35 </max>\n");
print("      </domain>\n");
print("      <range>\n");
print("        <min> -1 </min>\n");
print("        <max>  1 </max>\n");
print("      </range>\n");
print("      <output>fcs/rudder-pos-norm</output>\n");
print("   </aerosurface_scale>\n\n");
  
print("  </channel>\n\n");
print("  <channel name=\"Flaps\">\n");

print("   <kinematic name=\"Flaps Control\">\n");
print("     <input>fcs/flap-cmd-norm</input>\n");
print("     <traverse>\n");
print("       <setting>\n");
print("          <position>  0 </position>\n");
print("          <time>      0 </time>\n");
print("       </setting>\n");
print("       <setting>\n");
print("          <position> 15 </position>\n");
print("          <time>      4 </time>\n");
print("       </setting>\n");
print("       <setting>\n");
print("          <position> 30 </position>\n");
print("          <time>      3 </time>\n");
print("       </setting>\n");
print("     </traverse>\n");
print("     <output>fcs/flap-pos-deg</output>\n");
print("   </kinematic>\n\n");

print("   <aerosurface_scale name=\"flap normalization\">\n");
print("      <input>fcs/flap-pos-deg</input>\n");
print("      <domain>\n");
print("        <min>  0 </min>\n");
print("        <max> 30 </max>\n");
print("      </domain>\n");
print("      <range>\n");
print("        <min> 0 </min>\n");
print("        <max> 1 </max>\n");
print("      </range>\n");
print("      <output>fcs/flap-pos-norm</output>\n");
print("   </aerosurface_scale>\n\n");

print("  </channel>\n\n");
print("  <channel name=\"Landing Gear\">\n");

if($ac_gearretract == 1) {
  print("   <kinematic name=\"Gear Control\">\n");
  print("     <input>gear/gear-cmd-norm</input>\n");
  print("     <traverse>\n");
  print("       <setting>\n");
  print("          <position> 0 </position>\n");
  print("          <time>     0 </time>\n");
  print("       </setting>\n");
  print("       <setting>\n");
  print("          <position> 1 </position>\n");
  print("          <time>     5 </time>\n");
  print("       </setting>\n");
  print("     </traverse>\n");
  print("     <output>gear/gear-pos-norm</output>\n");
  print("   </kinematic>\n\n");
}

print("  </channel>\n\n");
print("  <channel name=\"Speedbrake\">\n");

print("   <kinematic name=\"Speedbrake Control\">\n");
print("     <input>fcs/speedbrake-cmd-norm</input>\n");
print("     <traverse>\n");
print("       <setting>\n");
print("          <position> 0 </position>\n");
print("          <time>     0 </time>\n");
print("       </setting>\n");
print("       <setting>\n");
print("          <position> 1 </position>\n");
print("          <time>     1 </time>\n");
print("       </setting>\n");
print("     </traverse>\n");
print("     <output>fcs/speedbrake-pos-norm</output>\n");
print("   </kinematic>\n\n");

print("  </channel>\n\n");

print(" </flight_control>\n\n");

//***** AERODYNAMICS ******************************************


print(" <aerodynamics>\n\n");

//print("  <pedigree>\n");
//print("   <author>Aeromatic version $version</author>\n");
//print("  </pedigree>\n\n");


print("  <axis name=\"LIFT\">\n\n");

// build a lift curve with four points
print("    <function name=\"aero/force/Lift_alpha\">\n");
print("      <description>Lift due to alpha</description>\n");
print("      <product>\n");
print("          <property>aero/qbar-psf</property>\n");
print("          <property>metrics/Sw-sqft</property>\n");
print("          <table>\n");
print("            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>\n");
print("            <tableData>\n");
$point = -($ac_CLalpha * 0.2) + $ac_CL0;
printf("              -0.20 %4.3f\n", $point);
printf("               0.00  %4.3f\n", $ac_CL0);
$alpha = ($ac_CLmax - $ac_CL0) / $ac_CLalpha;
printf("               %3.2f  %4.3f\n", $alpha, $ac_CLmax);
$point = $ac_CLmax - (0.6 * $alpha * $ac_CLalpha);
printf("               0.60  %4.3f\n", $point);
print("            </tableData>\n");
print("          </table>\n");
print("      </product>\n");
print("    </function>\n\n");

$ac_dCLflap_per_deg = $ac_dCLflaps / 30.0;
print("    <function name=\"aero/force/Lift_flap\">\n"); 
print("       <description>Delta Lift due to flaps</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>fcs/flap-pos-deg</property>\n");
printf("           <value> %6.5f </value>\n", $ac_dCLflap_per_deg);
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/force/Lift_speedbrake\">\n"); 
print("       <description>Delta Lift due to speedbrake</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>fcs/speedbrake-pos-norm</property>\n");
print("           <value>$ac_dCLspeedbrake</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/force/Lift_elevator\">\n"); 
print("       <description>Lift due to Elevator Deflection</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>fcs/elevator-pos-rad</property>\n");
print("           <value>$ac_CLde</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("  </axis>\n\n");

//***** DRAG ******************************************************

print("  <axis name=\"DRAG\">\n\n");

print("    <function name=\"aero/force/Drag_basic\">\n"); 
print("       <description>Drag at zero lift</description>\n");
print("       <product>\n");
print("          <property>aero/qbar-psf</property>\n");
print("          <property>metrics/Sw-sqft</property>\n");
print("          <table>\n");
print("            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>\n");
print("            <tableData>\n");
print( "             -1.57    1.500\n");
$ac_CD02 = $ac_CD0 * 1.3;
printf("             -0.26    %4.3f\n", $ac_CD02);   
printf("              0.00    %4.3f\n", $ac_CD0);
printf("              0.26    %4.3f\n", $ac_CD02);   
print( "              1.57    1.500\n");
print("            </tableData>\n");
print("          </table>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/force/Drag_induced\">\n"); 
print("       <description>Induced drag</description>\n");
print("         <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>aero/cl-squared</property>\n");
print("           <value>$ac_K</value>\n");
print("         </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/force/Drag_mach\">\n"); 
print("       <description>Drag due to mach</description>\n");
print("        <product>\n");
print("          <property>aero/qbar-psf</property>\n");
print("          <property>metrics/Sw-sqft</property>\n");
print("          <table>\n");
print("            <independentVar lookup=\"row\">velocities/mach</independentVar>\n");
print("            <tableData>\n");
print("                0.00      0.000\n");
print("                $ac_Mcrit      0.000\n");
print("                1.10      0.023\n");
print("                1.80      0.015\n");
print("            </tableData>\n");
print("          </table>\n");
print("        </product>\n");
print("    </function>\n\n");

$ac_CDflaps_per_deg = $ac_CDflaps / 30.0;
print("    <function name=\"aero/force/Drag_flap\">\n"); 
print("       <description>Drag due to flaps</description>\n");
print("         <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>fcs/flap-pos-deg</property>\n");
printf("           <value> %6.5f </value>\n", $ac_CDflaps_per_deg);
print("         </product>\n");
print("    </function>\n\n");

if($ac_gearretract == 1) {
  print("    <function name=\"aero/force/Drag_gear\">\n"); 
  print("       <description>Drag due to gear</description>\n");
  print("         <product>\n");
  print("           <property>aero/qbar-psf</property>\n");
  print("           <property>metrics/Sw-sqft</property>\n");
  print("           <property>gear/gear-pos-norm</property>\n");
  print("           <value>$ac_CDgear</value>\n");
  print("         </product>\n");
  print("    </function>\n\n");
}

print("    <function name=\"aero/force/Drag_speedbrake\">\n"); 
print("       <description>Drag due to speedbrakes</description>\n");
print("         <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>fcs/speedbrake-pos-norm</property>\n");
print("           <value>$ac_CDspeedbrake</value>\n");
print("         </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/force/Drag_beta\">\n"); 
print("       <description>Drag due to sideslip</description>\n");
print("       <product>\n");
print("          <property>aero/qbar-psf</property>\n");
print("          <property>metrics/Sw-sqft</property>\n");
print("          <table>\n");
print("            <independentVar lookup=\"row\">aero/beta-rad</independentVar>\n");
print("            <tableData>\n");
print( "              -1.57    1.230\n");
$ac_CDb26 = $ac_CDbeta * 0.25;  // CD at beta of 0.26 radians
printf("              -0.26    %4.3f\n", $ac_CDb26);   
printf("               0.00    0.000\n");
printf("               0.26    %4.3f\n", $ac_CDb26);   
print( "               1.57    1.230\n");
print("            </tableData>\n");
print("          </table>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/force/Drag_elevator\">\n"); 
print("       <description>Drag due to Elevator Deflection</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <abs><property>fcs/elevator-pos-norm</property></abs>\n");
print("           <value>$ac_CDde</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("  </axis>\n\n");

//***** SIDE *************************************************

print("  <axis name=\"SIDE\">\n\n");

print("    <function name=\"aero/force/Side_beta\">\n");
print("       <description>Side force due to beta</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>aero/beta-rad</property>\n");
print("           <value>$ac_CYbeta</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("  </axis>\n\n");

//***** ROLL ************************************************

print("  <axis name=\"ROLL\">\n\n");

print("    <function name=\"aero/moment/Roll_beta\">\n");
print("       <description>Roll moment due to beta</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>aero/beta-rad</property>\n");
print("           <value>$ac_Clbeta</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Roll_damp\">\n");
print("       <description>Roll moment due to roll rate</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>aero/bi2vel</property>\n");
print("           <property>velocities/p-aero-rad_sec</property>\n");
print("           <value>$ac_Clp</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Roll_yaw\">\n");
print("       <description>Roll moment due to yaw rate</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>aero/bi2vel</property>\n");
print("           <property>velocities/r-aero-rad_sec</property>\n");
print("           <value>$ac_Clr</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Roll_aileron\">\n");
print("       <description>Roll moment due to aileron</description>\n");
print("       <product>\n");
print("          <property>aero/qbar-psf</property>\n");
print("          <property>metrics/Sw-sqft</property>\n");
print("          <property>metrics/bw-ft</property>\n");
print("          <property>fcs/left-aileron-pos-rad</property>\n");
print("          <value>$ac_Clda</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Roll_rudder\">\n");
print("       <description>Roll moment due to rudder</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>fcs/rudder-pos-rad</property>\n");
print("           <value>$ac_Cldr</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("  </axis>\n\n");

//***** PITCH *****************************************

print("  <axis name=\"PITCH\">\n\n");

print("    <function name=\"aero/moment/Pitch_alpha\">\n");
print("       <description>Pitch moment due to alpha</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/cbarw-ft</property>\n");
print("           <property>aero/alpha-rad</property>\n");
print("           <value>$ac_Cmalpha</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Pitch_elevator\">\n");
print("       <description>Pitch moment due to elevator</description>\n");
print("       <product>\n");
print("          <property>aero/qbar-psf</property>\n");
print("          <property>metrics/Sw-sqft</property>\n");
print("          <property>metrics/cbarw-ft</property>\n");
print("          <property>fcs/elevator-pos-rad</property>\n");
print("          <table>\n");
print("            <independentVar lookup=\"row\">velocities/mach</independentVar>\n");
print("            <tableData>\n");
printf("              0.0     %4.3f\n", $ac_Cmde);
$ac_Cmde4 = $ac_Cmde * 0.25;
printf("              2.0     %4.3f\n", $ac_Cmde4);
print("            </tableData>\n");
print("          </table>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Pitch_damp\">\n");
print("       <description>Pitch moment due to pitch rate</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/cbarw-ft</property>\n");
print("           <property>aero/ci2vel</property>\n");
print("           <property>velocities/q-aero-rad_sec</property>\n");
print("           <value>$ac_Cmq</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Pitch_alphadot\">\n");
print("       <description>Pitch moment due to alpha rate</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/cbarw-ft</property>\n");
print("           <property>aero/ci2vel</property>\n");
print("           <property>aero/alphadot-rad_sec</property>\n");
print("           <value>$ac_Cmadot</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("  </axis>\n\n");

//***** YAW ***********************************************

print("  <axis name=\"YAW\">\n\n");

print("    <function name=\"aero/moment/Yaw_beta\">\n");
print("       <description>Yaw moment due to beta</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>aero/beta-rad</property>\n");
print("           <value>$ac_Cnbeta</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Yaw_damp\">\n");
print("       <description>Yaw moment due to yaw rate</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>aero/bi2vel</property>\n");
print("           <property>velocities/r-aero-rad_sec</property>\n");
print("           <value>$ac_Cnr</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Yaw_rudder\">\n");
print("       <description>Yaw moment due to rudder</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>fcs/rudder-pos-rad</property>\n");
print("           <value>$ac_Cndr</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("    <function name=\"aero/moment/Yaw_aileron\">\n");
print("       <description>Adverse yaw</description>\n");
print("       <product>\n");
print("           <property>aero/qbar-psf</property>\n");
print("           <property>metrics/Sw-sqft</property>\n");
print("           <property>metrics/bw-ft</property>\n");
print("           <property>fcs/left-aileron-pos-rad</property>\n");
print("           <value>$ac_Cnda</value>\n");
print("       </product>\n");
print("    </function>\n\n");

print("  </axis>\n\n");
print(" </aerodynamics>\n\n");
print(" <external_reactions>\n");
print(" </external_reactions>\n\n");
print("</fdm_config>\n");

?>
