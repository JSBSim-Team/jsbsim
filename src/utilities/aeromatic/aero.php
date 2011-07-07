<?php

$version = 0.71;

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
  case 0: $ac_wingloading = 7.0; break;   // glider
  case 1: $ac_wingloading = 14.0; break;  // light single
  case 2: $ac_wingloading = 29.0; break;  // light twin
  case 3: $ac_wingloading = 45.0; break;  // WW2 fighter, racer
  case 4: $ac_wingloading = 95.0; break;  // single-eng jet fighter
  case 5: $ac_wingloading = 100.0; break; // 2-eng jet fighter
  case 6: $ac_wingloading = 110.0; break; // 2-eng jet transport
  case 7: $ac_wingloading = 110.0; break; // 3-eng jet transport
  case 8: $ac_wingloading = 110.0; break; // 4-eng jet transport
  case 9: $ac_wingloading = 57.0; break;  // prop transport
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

$ac_rawixx = ($ac_weight / 32.2)* pow(($Rx * $ac_wingspan / 2), 2);
$ac_rawiyy = ($ac_weight / 32.2)* pow(($Ry * $ac_length / 2), 2);
$ac_rawizz = ($ac_weight / 32.2)* pow(($Rz * (($ac_wingspan + $ac_length)/2) / 2), 2);
// assume 4 degree angle between longitudinal and inertial axes
// $ac_rawixz = abs($ac_rawizz - $ac_rawixx) * 0.06975647;


// increase moments to make up for lack of control feel
$ac_ixx = $ac_rawixx * 1.5;
$ac_iyy = $ac_rawiyy * 1.5;
$ac_izz = $ac_rawizz * 1.5;
$ac_ixz = 0;


//***** EMPTY WEIGHT *********************************

// estimate empty weight
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
  case 0: $ac_eyeptlocy = 0; break;
  case 1: $ac_eyeptlocy = -18; break;
  case 2: $ac_eyeptlocy = -18; break;
  case 3: $ac_eyeptlocy = 0; break;
  case 4: $ac_eyeptlocy = 0; break;
  case 5: $ac_eyeptlocy = 0; break;
  case 6: $ac_eyeptlocy = -30; break;
  case 7: $ac_eyeptlocy = -30; break;
  case 8: $ac_eyeptlocy = -32; break;
  case 9: $ac_eyeptlocy = -24; break;
  }

switch($ac_type) {
  case 0: $ac_eyeptlocz = 9; break;
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
  case 1: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 2: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 3: $ac_gearlocy_main = $ac_wingspan * 0.15 * 12; break;
  case 4: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 5: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 6: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 7: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 8: $ac_gearlocy_main = $ac_wingspan * 0.09 * 12; break;
  case 9: $ac_gearlocy_main = $ac_wingspan * 0.11 * 12; break;
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

$ac_geardamp_main = $ac_weight * 0.2;
$ac_geardamp_nose = $ac_weight * 0.1;
$ac_geardamp_tail = $ac_weight * 0.8;

$ac_geardynamic = 0.5;
$ac_gearstatic  = 0.8;
$ac_gearrolling = 0.02;
if($ac_type == 0) $ac_gearrolling = 0.5;  // glider

$ac_gearsteerable_nose = 'STEERABLE';
$ac_gearsteerable_main = 'FIXED';
$ac_gearsteerable_tail = 'CASTERED';
$ac_gearmaxsteer = 5;
if($ac_gearretract == 0)
  $ac_retract = 'FIXED';
else
  $ac_retract = 'RETRACT';

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
      $ac_englocy[$i] = $ac_wingspan * -2.0;       // span/-2/3*12
      $ac_englocz[$i] = -40; 
     }    
  for($j=$i; $j<$halfcount+$remainder; $j++) {     //center
      $ac_englocx[$j] = $ac_cglocx;
      $ac_englocy[$j] = 0;
      $ac_englocz[$j] = -20; 
     }    
  for($k=$j; $k<$ac_numengines; $k++) {            //right wing
      $ac_englocx[$k] = $ac_cglocx;
      $ac_englocy[$k] = $ac_wingspan * 2.0;        // span/2/3*12
      $ac_englocz[$k] = -40; 
     }    
  }

// wing and tail engines
if ($ac_enginelayout == 4) {
  $halfcount = intval( $ac_numengines / 2 );
  $remainder = $ac_numengines - ($halfcount * 2);
  for($i=0; $i<$halfcount; $i++) {                 //left wing
      $ac_englocx[$i] = $ac_cglocx;
      $ac_englocy[$i] = $ac_wingspan * -2.0;       // span/-2/3*12
      $ac_englocz[$i] = -40; 
     }    
  for($j=$i; $j<$halfcount+$remainder; $j++) {     //center
      $ac_englocx[$j] = $ac_length - 60;
      $ac_englocy[$j] = 0;
      $ac_englocz[$j] = 60; 
     }    
  for($k=$j; $i<$ac_numengines; $i++) {            //right wing
      $ac_englocx[$k] = $ac_cglocx;
      $ac_englocy[$k] = $ac_wingspan * 2.0;        // span/2/3*12 
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
  case 0: $ac_thrustertype = 'prop'; break;
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
switch($ac_type) {
  case 0: $ac_tankcapacity = 0; break;
  case 1: $ac_tankcapacity = 20; break;
  case 2: $ac_tankcapacity = 50; break;
  case 3: $ac_tankcapacity = 200; break;
  case 4: $ac_tankcapacity = 500; break;
  case 5: $ac_tankcapacity = 700; break;
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
  case 1: $ac_dCLspeedbrake = 0.00; break;
  case 2: $ac_dCLspeedbrake = 0.00; break;
  case 3: $ac_dCLspeedbrake = 0.00; break;
  case 4: $ac_dCLspeedbrake = 0.00; break;
  case 5: $ac_dCLspeedbrake = 0.00; break;
  case 6: $ac_dCLspeedbrake = -0.10; break;
  case 7: $ac_dCLspeedbrake = -0.09; break;
  case 8: $ac_dCLspeedbrake = -0.08; break;
  case 9: $ac_dCLspeedbrake = 0.00; break;
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
  case 3: $ac_K = 0.045; break;
  case 4: $ac_K = 0.090; break;
  case 5: $ac_K = 0.090; break;
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
  case 3: $ac_Mcrit = 0.73; break;
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
  case 0: $ac_Cnda = -0.02; break;
  case 1: $ac_Cnda = -0.01; break;
  case 2: $ac_Cnda = -0.01; break;
  case 3: $ac_Cnda = -0.003; break;
  case 4: $ac_Cnda = 0.0; break;
  case 5: $ac_Cnda = 0.0; break;
  case 6: $ac_Cnda = 0.0; break;
  case 7: $ac_Cnda = 0.0; break;
  case 8: $ac_Cnda = 0.0; break;
  case 9: $ac_Cnda = -0.008; break;
  }


//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************

print("<FDM_CONFIG NAME=\"$ac_name\" VERSION=\"1.65\" RELEASE=\"ALPHA\">\n");
print("<!--\n  File:     $ac_name.xml\n");
print("  Author:   Aero-Matic v $version\n\n");
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
print("    K:             $ac_K\n"); 
print("\n-->\n\n"); 

//***** METRICS **********************************

print(" <METRICS>\n");
printf("   AC_WINGAREA  %2.2f\n", $ac_wingarea);
printf("   AC_WINGSPAN  %2.2f\n", $ac_wingspan);
printf("   AC_CHORD     %2.2f\n", $ac_wingchord);
printf("   AC_HTAILAREA %2.2f\n", $ac_htailarea);
printf("   AC_HTAILARM  %2.2f\n", $ac_htailarm);
printf("   AC_VTAILAREA %2.2f\n", $ac_vtailarea);
printf("   AC_LV        %2.2f\n", $ac_vtailarm);
printf("   AC_IXX       %1.0f\n", $ac_ixx);
printf("   AC_IYY       %1.0f\n", $ac_iyy);
printf("   AC_IZZ       %1.0f\n", $ac_izz);
printf("   AC_IXZ       %1.0f\n", $ac_ixz);
printf("   AC_EMPTYWT   %1.0f\n", $ac_emptyweight);
printf("   AC_CGLOC     %2.1f %2.1f %2.1f\n", $ac_cglocx, $ac_cglocy, $ac_cglocz);
printf("   AC_AERORP    %2.1f %2.1f %2.1f\n", $ac_aerorpx, $ac_aerorpy, $ac_aerorpz);
printf("   AC_EYEPTLOC  %2.1f %2.1f %2.1f\n", $ac_eyeptlocx, $ac_eyeptlocy, $ac_eyeptlocz);
print( "   AC_VRP       0 0 0\n");
print(" </METRICS>\n");

//***** LANDING GEAR ******************************

print(" <UNDERCARRIAGE>\n");

if($ac_type == 0) {  // if this is a glider
   print("  AC_GEAR LEFT_MLG  $ac_gearlocx_main -$ac_gearlocy_main $ac_gearlocz_main ");
   print("$ac_gearspring_main $ac_geardamp_main $ac_geardynamic $ac_gearstatic ");
   print("$ac_gearrolling $ac_gearsteerable_main NONE 0 $ac_retract\n");

   print("  AC_GEAR RIGHT_MLG  $ac_gearlocx_main $ac_gearlocy_main $ac_gearlocz_main ");
   print("$ac_gearspring_main $ac_geardamp_main $ac_geardynamic $ac_gearstatic ");
   print("$ac_gearrolling $ac_gearsteerable_main NONE 0 $ac_retract\n");

   print("  AC_GEAR NOSE_LG   $ac_gearlocx_nose $ac_gearlocy_nose $ac_gearlocz_nose ");
   print("$ac_gearspring_nose $ac_geardamp_nose $ac_geardynamic $ac_gearstatic ");
   print("$ac_gearrolling $ac_gearsteerable_nose NONE $ac_gearmaxsteer $ac_retract\n");

   print("  AC_GEAR LEFT_WING $ac_cglocx -$ac_halfspan $ac_cglocz ");
   print("$ac_gearspring_nose $ac_geardamp_nose $ac_geardynamic $ac_gearstatic ");
   print("$ac_gearrolling $ac_gearsteerable_main NONE 0 $ac_retract\n");

   print("  AC_GEAR RIGHT_WING $ac_cglocx -$ac_halfspan $ac_cglocz ");
   print("$ac_gearspring_nose $ac_geardamp_nose $ac_geardynamic $ac_gearstatic ");
   print("$ac_gearrolling $ac_gearsteerable_main NONE 0 $ac_retract\n");
 }
 else {
   if ($ac_geartype == 0) {  // if this is a tricycle gear
     printf("  AC_GEAR NOSE_LG   %2.1f %2.1f %2.1f", $ac_gearlocx_nose, $ac_gearlocy_nose, $ac_gearlocz_nose);
     printf(" %2.1f %2.1f %2.1f %2.1f", $ac_gearspring_nose, $ac_geardamp_nose, $ac_geardynamic, $ac_gearstatic);
     printf(" %2.2f $ac_gearsteerable_nose NONE %2.1f $ac_retract\n", $ac_gearrolling, $ac_gearmaxsteer);
   }

   printf("  AC_GEAR LEFT_MLG  %2.1f %2.1f %2.1f", $ac_gearlocx_main, -$ac_gearlocy_main, $ac_gearlocz_main);
   printf(" %2.1f %2.1f %2.1f %2.1f", $ac_gearspring_main, $ac_geardamp_main, $ac_geardynamic, $ac_gearstatic);
   printf(" %2.2f $ac_gearsteerable_main LEFT  0 $ac_retract\n", $ac_gearrolling);

   printf("  AC_GEAR RIGHT_MLG %2.1f %2.1f %2.1f", $ac_gearlocx_main, $ac_gearlocy_main, $ac_gearlocz_main);
   printf(" %2.1f %2.1f %2.1f %2.1f", $ac_gearspring_main, $ac_geardamp_main, $ac_geardynamic, $ac_gearstatic);
   printf(" %2.2f $ac_gearsteerable_main RIGHT 0 $ac_retract\n", $ac_gearrolling);

   if ($ac_geartype == 1) {  // if this is a taildragger
     printf("  AC_GEAR TAIL_LG  %2.1f %2.1f %2.1f", $ac_gearlocx_tail, $ac_gearlocy_tail, $ac_gearlocz_tail);
     printf(" %2.1f %2.1f %2.1f %2.1f", $ac_gearspring_tail, $ac_geardamp_tail, $ac_geardynamic, $ac_gearstatic);
     printf(" %2.2f $ac_gearsteerable_tail NONE 0 $ac_retract\n", $ac_gearrolling);
   }
 }

print(" </UNDERCARRIAGE>\n");

//***** PROPULSION ***************************************

print(" <PROPULSION>\n");
if($ac_type == 0) { // if glider, do nothing here
 }
 else {
 for($i=0; $i<$ac_numengines; $i++) {
  print("  <AC_ENGINE FILE=\"$ac_name");
  print("_engine\">\n");
  print("    XLOC $ac_englocx[$i]\n");
  print("    YLOC $ac_englocy[$i]\n");
  print("    ZLOC $ac_englocz[$i]\n");
  print("    PITCH $ac_engpitch[$i]\n");
  print("    YAW $ac_engyaw[$i]\n");
  print("    FEED $ac_engfeed[$i]\n");
  if($ac_enginetype == 0) {
    print("    <AC_THRUSTER FILE=\"$ac_name");
    print("_prop\">\n");
    }
    else {
    print("    <AC_THRUSTER FILE=\"direct\">\n");
    }
  print("      XLOC $ac_thrusterlocx[$i]\n");
  print("      YLOC $ac_thrusterlocy[$i]\n");
  print("      ZLOC $ac_thrusterlocz[$i]\n");
  print("      PITCH $ac_thrusterpitch[$i]\n");
  print("      YAW $ac_thrusteryaw[$i]\n");
  print("    </AC_THRUSTER>\n");
  print("  </AC_ENGINE>\n");
  }

 //***** FUEL TANKS **************************************

 for($i=0; $i<($ac_numengines + 1); $i++) {
  print("  <AC_TANK TYPE=\"FUEL\" NUMBER=\"$i\">\n");
  print("    XLOC $ac_tanklocx \n");
  print("    YLOC $ac_tanklocy \n");
  print("    ZLOC $ac_tanklocz \n");
  print("    RADIUS $ac_tankradius \n");
  printf("    CAPACITY %5.1f\n", $ac_tankcapacity);
  printf("    CONTENTS %5.1f\n", $ac_tankcontents);
  print("  </AC_TANK>\n");
 }
}
print(" </PROPULSION>\n");

//***** FLIGHT CONTROL SYSTEM ***************************

print(" <FLIGHT_CONTROL NAME=\"$ac_name\">\n");
print("   <COMPONENT NAME=\"Pitch Trim Sum\" TYPE=\"SUMMER\">\n");
print("      INPUT   fcs/elevator-cmd-norm\n");
print("      INPUT   fcs/pitch-trim-cmd-norm\n");
print("      CLIPTO  -1 1\n");
print("   </COMPONENT>\n");
print("   <COMPONENT NAME=\"Elevator Control\" TYPE=\"AEROSURFACE_SCALE\">\n");
print("      INPUT   fcs/pitch-trim-sum\n");
print("      MIN     -0.350\n");
print("      MAX      0.300\n");
print("      OUTPUT  fcs/elevator-pos-rad\n");
print("   </COMPONENT>\n");

print("   <COMPONENT NAME=\"Roll Trim Sum\" TYPE=\"SUMMER\">\n");
print("      INPUT   fcs/aileron-cmd-norm\n");
print("      INPUT   fcs/roll-trim-cmd-norm\n");
print("      CLIPTO  -1 1\n");
print("   </COMPONENT>\n");
print("   <COMPONENT NAME=\"Left Aileron Control\" TYPE=\"AEROSURFACE_SCALE\">\n");
print("      INPUT   fcs/roll-trim-sum\n");
print("      MIN    -0.35\n");
print("      MAX     0.35\n");
print("      OUTPUT  fcs/left-aileron-pos-rad\n");
print("   </COMPONENT>\n");
print("   <COMPONENT NAME=\"Right Aileron Control\" TYPE=\"AEROSURFACE_SCALE\">\n");
print("      INPUT  -fcs/roll-trim-sum\n");
print("      MIN    -0.35\n");
print("      MAX     0.35\n");
print("      OUTPUT  fcs/right-aileron-pos-rad\n");
print("   </COMPONENT>\n");

print("   <COMPONENT NAME=\"Rudder Command Sum\" TYPE=\"SUMMER\">\n");
print("      INPUT   fcs/rudder-cmd-norm\n");
print("      INPUT   fcs/yaw-trim-cmd-norm\n");
print("      CLIPTO  -1 1\n");
print("   </COMPONENT>\n");

if($ac_yawdamper == 1) {
  print("   <COMPONENT NAME=\"Yaw Damper Rate\" TYPE=\"SCHEDULED_GAIN\">\n");
  print("      INPUT        velocities/r-aero-rad_sec\n");
  print("      SCHEDULED_BY aero/qbar-psf\n");
  print("      ROWS         2\n");
  print("       3.00        0.00\n");
  print("      11.00        2.00\n");
  print("   </COMPONENT>\n");
  print("   <COMPONENT NAME=\"Yaw Damper Beta\" TYPE=\"SCHEDULED_GAIN\">\n");
  print("      INPUT        aero/beta-rad\n");
  print("      SCHEDULED_BY aero/qbar-psf\n");
  print("      ROWS         2\n");
  print("       3.00        0.00\n");
  print("      11.00        0.00\n");
  print("   </COMPONENT>\n");
  print("   <COMPONENT NAME=\"Yaw Damper Sum\" TYPE=\"SUMMER\">\n");
  print("      INPUT        fcs/yaw-damper-beta\n");
  print("      INPUT        fcs/yaw-damper-rate\n");
  print("      CLIPTO      -0.2 0.2\n");
  print("   </COMPONENT>\n");
  print("   <COMPONENT NAME=\"Yaw Damper Final\" TYPE=\"SCHEDULED_GAIN\">\n");
  print("      INPUT        fcs/yaw-damper-sum\n");
  print("      SCHEDULED_BY aero/qbar-psf\n");
  print("      ROWS         2\n");
  print("      2.99         0.0\n");
  print("      3.00         1.0\n");
  print("   </COMPONENT>\n");
  print("   <COMPONENT NAME=\"Rudder Sum\" TYPE=\"SUMMER\">\n");
  print("      INPUT   fcs/rudder-command-sum\n");
  print("      INPUT   fcs/yaw-damper-final\n");
  print("      CLIPTO  -1 1\n");
  print("   </COMPONENT>\n");
  print("   <COMPONENT NAME=\"Rudder Control\" TYPE=\"AEROSURFACE_SCALE\">\n");
  print("      INPUT   fcs/rudder-sum\n");
  print("      MIN     -0.35\n");
  print("      MAX      0.35\n");
  print("      OUTPUT  fcs/rudder-pos-rad\n");
  print("   </COMPONENT>\n"); 
  }
  else {
   print("   <COMPONENT NAME=\"Rudder Control\" TYPE=\"AEROSURFACE_SCALE\">\n");
   print("      INPUT   fcs/rudder-command-sum\n");
   print("      MIN     -0.35\n");
   print("      MAX      0.35\n");
   print("      OUTPUT  fcs/rudder-pos-rad\n");
   print("   </COMPONENT>\n"); 
  }

print("   <COMPONENT NAME=\"Flaps Control\" TYPE=\"KINEMAT\">\n");
print("     INPUT   fcs/flap-cmd-norm\n");
print("     DETENTS 3\n");
print("             0   0\n");
print("             15  4\n");
print("             30  3\n");
print("     OUTPUT  fcs/flap-pos-deg\n");
print("   </COMPONENT>\n");

if($ac_gearretract == 1) {
  print("   <COMPONENT NAME=\"Gear Control\" TYPE=\"KINEMAT\">\n");
  print("     INPUT   gear/gear-cmd-norm\n");
  print("     DETENTS 2\n");
  print("             0   0\n");
  print("             1   5\n");
  print("     OUTPUT  gear/gear-pos-norm\n");
  print("   </COMPONENT>\n");
}

print("   <COMPONENT NAME=\"Speedbrake Control\" TYPE=\"KINEMAT\">\n");
print("     INPUT   fcs/speedbrake-cmd-norm\n");
print("     DETENTS 2\n");
print("             0   0\n");
print("             1   1\n");
print("     OUTPUT  fcs/speedbrake-pos-norm\n");
print("   </COMPONENT>\n");

print(" </FLIGHT_CONTROL>\n");

//***** AERODYNAMICS ******************************************

print(" <AERODYNAMICS>\n");

// build a lift curve with four points
print("  <AXIS NAME=\"LIFT\">\n");
print("    <COEFFICIENT NAME=\"CLalpha\" TYPE=\"VECTOR\">\n");
print("      Lift_due_to_alpha\n");
print("      4\n");
print("      aero/alpha-rad\n");
print("      aero/qbar-psf|metrics/Sw-sqft\n");
$point = -($ac_CLalpha * 0.2) + $ac_CL0;
printf("      -0.20 %4.3f\n", $point);
printf("       0.00 %4.3f\n", $ac_CL0);
$alpha = ($ac_CLmax - $ac_CL0) / $ac_CLalpha;
printf("     %3.2f %4.3f\n", $alpha, $ac_CLmax);
$point = $ac_CLmax - (0.6 * $alpha * $ac_CLalpha);
printf("       0.60 %4.3f\n", $point);
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"dCLflap\" TYPE=\"VALUE\">\n"); 
print("       Delta_Lift_due_to_flaps\n");
print("       aero/qbar-psf|metrics/Sw-sqft|fcs/flap-pos-norm\n");
print("       $ac_dCLflaps\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"dCLsb\" TYPE=\"VALUE\">\n"); 
print("       Delta_Lift_due_to_speedbrake\n");
print("       aero/qbar-psf|metrics/Sw-sqft|fcs/speedbrake-pos-norm\n");
print("       $ac_dCLspeedbrake\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"CLde\" TYPE=\"VALUE\">\n"); 
print("       Lift_due_to_Elevator_Deflection\n");
print("       aero/qbar-psf|metrics/Sw-sqft|fcs/elevator-pos-rad\n");
print("       $ac_CLde\n");
print("    </COEFFICIENT>\n");

print("  </AXIS>\n");

//***** DRAG ******************************************************

print("  <AXIS NAME=\"DRAG\">\n");

print("    <COEFFICIENT NAME=\"CD0\" TYPE=\"VECTOR\">\n"); 
print("       Drag_at_zero_lift\n");
print("       5\n");
print("       aero/alpha-rad\n");
print("       aero/qbar-psf|metrics/Sw-sqft\n");
print("       -1.57       1.500\n");
$ac_CD02 = $ac_CD0 * 2;
printf("       -0.26    %4.3f\n", $ac_CD02);   
printf("        0.00    %4.3f\n", $ac_CD0);
printf("        0.26    %4.3f\n", $ac_CD02);   
print("        1.57       1.500\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"CDi\" TYPE=\"VALUE\">\n"); 
print("       Induced_drag\n");
print("       aero/qbar-psf|metrics/Sw-sqft|aero/cl-squared-norm\n");
print("       $ac_K\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"CDmach\" TYPE=\"VECTOR\">\n"); 
print("       Drag_due_to_mach\n");
print("       4\n");
print("       velocities/mach-norm\n");
print("       aero/qbar-psf|metrics/Sw-sqft\n");
print("       0.0       0.000\n");
print("       $ac_Mcrit      0.000\n");
print("       1.1       0.023\n");
print("       1.8       0.015\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"CDflap\" TYPE=\"VALUE\">\n"); 
print("       Drag_due_to_flaps\n");
print("       aero/qbar-psf|metrics/Sw-sqft|fcs/flap-pos-norm\n");
print("       $ac_CDflaps\n");
print("    </COEFFICIENT>\n");

if($ac_gearretract == 1) {
  print("    <COEFFICIENT NAME=\"CDgear\" TYPE=\"VALUE\">\n"); 
  print("       Drag_due_to_gear\n");
  print("       aero/qbar-psf|metrics/Sw-sqft|gear/gear-pos-norm\n");
  print("       $ac_CDgear\n");
  print("    </COEFFICIENT>\n");
}

print("    <COEFFICIENT NAME=\"CDsb\" TYPE=\"VALUE\">\n"); 
print("       Drag_due_to_speedbrakes\n");
print("       aero/qbar-psf|metrics/Sw-sqft|fcs/speedbrake-pos-norm\n");
print("       $ac_CDspeedbrake\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"CDbeta\" TYPE=\"VECTOR\">\n"); 
print("       Drag_due_to_sideslip\n");
print("       5\n");
print("       aero/beta-rad\n");
print("       aero/qbar-psf|metrics/Sw-sqft\n");
print("       -1.57       1.230\n");
$ac_CDb26 = $ac_CDbeta * 0.25;  // CD at beta of 0.26 radians
printf("       -0.26    %4.3f\n", $ac_CDb26);   
printf("        0.00       0.00\n");
printf("        0.26    %4.3f\n", $ac_CDb26);   
print("        1.57       1.230\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"CDde\" TYPE=\"VALUE\">\n"); 
print("       Drag_due_to_Elevator_Deflection\n");
print("       aero/qbar-psf|metrics/Sw-sqft|fcs/elevator-pos-norm\n");
print("       $ac_CDflaps\n");
print("    </COEFFICIENT>\n");

print("  </AXIS>\n");

//***** SIDE *************************************************

print("  <AXIS NAME=\"SIDE\">\n");
print("    <COEFFICIENT NAME=\"CYb\" TYPE=\"VALUE\">\n");
print("       Side_force_due_to_beta\n");
print("       aero/qbar-psf|metrics/Sw-sqft|aero/beta-rad\n");
print("       $ac_CYbeta\n");
print("    </COEFFICIENT>\n");
print("  </AXIS>\n");

//***** ROLL ************************************************

print("  <AXIS NAME=\"ROLL\">\n");

print("    <COEFFICIENT NAME=\"Clb\" TYPE=\"VALUE\">\n");
print("       Roll_moment_due_to_beta\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|aero/beta-rad\n");
print("       $ac_Clbeta\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Clp\" TYPE=\"VALUE\">\n");
print("       Roll_moment_due_to_roll_rate\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|aero/bi2vel|velocities/p-aero-rad_sec\n");
print("       $ac_Clp\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Clr\" TYPE=\"VALUE\">\n");
print("       Roll_moment_due_to_yaw_rate\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|aero/bi2vel|velocities/r-aero-rad_sec\n");
print("       $ac_Clr\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Clda\" TYPE=\"VECTOR\">\n");
print("       Roll_moment_due_to_aileron\n");
print("       2\n");
print("       velocities/mach-norm\n"); 
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|fcs/left-aileron-pos-rad\n");
print("       0.0       $ac_Clda\n");
$ac_Clda3 = $ac_Clda * 0.333;
printf("       2.0    %4.3f\n", $ac_Clda3);
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cldr\" TYPE=\"VALUE\">\n");
print("       Roll_moment_due_to_rudder\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|fcs/rudder-pos-rad\n");
print("       $ac_Cldr\n");
print("    </COEFFICIENT>\n");

print("  </AXIS>\n");

//***** PITCH *****************************************

print("  <AXIS NAME=\"PITCH\">\n");

print("    <COEFFICIENT NAME=\"Cmalpha\" TYPE=\"VALUE\">\n");
print("       Pitch_moment_due_to_alpha\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/cbarw-ft|aero/alpha-rad\n");
print("       $ac_Cmalpha\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cmde\" TYPE=\"VECTOR\">\n");
print("       Pitch_moment_due_to_elevator\n");
print("       2\n");
print("       velocities/mach-norm\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/cbarw-ft|fcs/elevator-pos-rad\n");
print("       0.0       $ac_Cmde\n");
$ac_Cmde4 = $ac_Cmde * 0.25;
printf("       2.0     %4.3f\n", $ac_Cmde4);
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cmq\" TYPE=\"VALUE\">\n");
print("       Pitch_moment_due_to_pitch_rate\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/cbarw-ft|aero/ci2vel|velocities/q-aero-rad_sec\n");
print("       $ac_Cmq\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cmadot\" TYPE=\"VALUE\">\n");
print("       Pitch_moment_due_to_alpha_rate\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/cbarw-ft|aero/ci2vel|aero/alphadot-rad_sec\n");
print("       $ac_Cmadot\n");
print("    </COEFFICIENT>\n");

print("  </AXIS>\n");

//***** YAW ***********************************************

print("  <AXIS NAME=\"YAW\">\n");

print("    <COEFFICIENT NAME=\"Cnb\" TYPE=\"VALUE\">\n");
print("       Yaw_moment_due_to_beta\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|aero/beta-rad\n");
print("       $ac_Cnbeta\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cnr\" TYPE=\"VALUE\">\n");
print("       Yaw_moment_due_to_yaw_rate\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|aero/bi2vel|velocities/r-aero-rad_sec\n");
print("       $ac_Cnr\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cndr\" TYPE=\"VALUE\">\n");
print("       Yaw_moment_due_to_rudder\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|fcs/rudder-pos-rad\n");
print("       $ac_Cndr\n");
print("    </COEFFICIENT>\n");

print("    <COEFFICIENT NAME=\"Cnda\" TYPE=\"VALUE\">\n");
print("       Adverse_yaw\n");
print("       aero/qbar-psf|metrics/Sw-sqft|metrics/bw-ft|fcs/left-aileron-pos-rad\n");
print("       $ac_Cnda\n");
print("    </COEFFICIENT>\n");

print("  </AXIS>\n");
print(" </AERODYNAMICS>\n");
print("</FDM_CONFIG>\n");

?>
