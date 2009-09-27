<?php

$version = 0.82;

//****************************************************
//                                                   *
// This is the CGI back-end for the Aero-Matic gen-  *
// erator. The front-end is aeromatic.html.          *
//                                                   *
// June 2003, David P. Culp, davidculp2@comcast.net  *
//                                                   * 
//****************************************************
// Updated: 16 Oct 2003, DPC, new SimTurbine support
// Updated: 20 Dec 2004, DPC, new XML version 2.0

header("Content-type: text/plain");


//***** GET DATA FROM USER ***************************

$ac_enginename      = $_POST['ac_enginename'];
$ac_enginetype      = $_POST['ac_enginetype'];
$ac_enginepower     = $_POST['ac_enginepower'];
$ac_engineunits     = $_POST['ac_engineunits'];
$ac_augmented       = $_POST['ac_augmented'];
$ac_injected        = $_POST['ac_injected'];

//***** CONVERT TO ENGLISH UNITS *********************

if($ac_engineunits == 1) {
  $ac_enginepower *= 1.341;
  }
if($ac_engineunits == 3) {
  $ac_enginepower *= 0.2248;
  }



//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************

print("<?xml version=\"1.0\"?>\n");
print("<!--\n  File:     $ac_enginename.xml\n");
print("  Author:   Aero-Matic v $version\n\n");
print("  Inputs:\n");
print("    name:           $ac_enginename\n");
switch($ac_enginetype) {
  case 0: print("    type:           piston\n"); break;
  case 1: print("    type:           turbine\n"); break;
  case 2: print("    type:           turboprop\n"); break;
  case 3: print("    type:           rocket\n"); break;
  }  
switch($ac_enginetype) {
  case 0: print("    power:          $ac_enginepower hp\n"); break;
  case 1: print("    thrust:         $ac_enginepower lb\n"); break;
  case 2: print("    power:          $ac_enginepower hp\n"); break;
  case 3: print("    thrust:         $ac_enginepower lb\n"); break;
  }  
if($ac_augmented)
  print("    augmented?      yes\n");
else
  print("    augmented?      no\n");
if($ac_injected)
  print("    injected?       yes\n");
else
  print("    injected?       no\n");
print("-->\n\n"); 

switch ($ac_enginetype) {
  case 0: MakePiston(); break;
  case 1: MakeTurbine(); break;
  case 2: MakeTurboprop(); break;
  case 3: MakeRocket(); break;
  }

//************************************************
//*  MakePiston()                                *
//************************************************
function MakePiston() {

  global $ac_enginename, $ac_enginepower;
  $displacement = $ac_enginepower * 1.9;

  print("<piston_engine name=\"$ac_enginename\">\n");
  print("  <minmp unit=\"INHG\">          6.0 </minmp>\n");
  print("  <maxmp unit=\"INHG\">         28.5 </maxmp>\n");
  printf("  <displacement unit=\"IN3\"> %3.2f </displacement>\n", $displacement);
  printf("  <maxhp>        %3.2f </maxhp>\n", $ac_enginepower);
  print("  <cycles>         4.0 </cycles>\n");
  print("  <idlerpm>      700.0 </idlerpm>\n");
  print("  <maxrpm>      2800.0 </maxrpm>\n");
  print("  <maxthrottle>    1.0 </maxthrottle><!-- Deprecated -->\n");
  print("  <minthrottle>    0.1 </minthrottle><!-- Deprecated -->\n");
  print("  <sparkfaildrop>  0.1 </sparkfaildrop>\n");
  print("  <volumetric-efficiency> 0.85 </volumetric-efficiency>\n");
  print("<!-- Optional items that affect engine performance -->\n");
  print(" <!-- Defining <bsfc> over-rides the built-in horsepower calculations -->\n");
  print(" <!--<bsfc>           0.45 </bsfc>-->\n");
  print(" <!--<stroke unit=\"IN\">  4.375  </stroke>-->\n");
  print(" <!--<bore unit=\"IN\">    5.125 </bore>-->\n");
  print(" <!--<cylinders>         4.0  </cylinders>-->\n");
  print(" <!--<compression-ratio> 8.0 </compression-ratio>-->\n");
  print("</piston_engine>\n");
  }


//************************************************
//*  MakeTurbine()                               *
//************************************************
function MakeTurbine() {

  global $ac_enginename, $ac_enginepower,
         $ac_augmented, $ac_injected;

  if ($ac_augmented) {
    $ac_maxthrust = $ac_enginepower * 1.5;
  } else {
    $ac_maxthrust = $ac_enginepower;
  }

  print("<turbine_engine name=\"$ac_enginename\">\n");  
  printf("  <milthrust>%8.1f </milthrust>\n", $ac_enginepower);
  if ($ac_augmented) {
  printf("  <maxthrust>%8.1f </maxthrust>\n", $ac_maxthrust);
  }
  print("  <bypassratio>     1.0 </bypassratio>\n");
  print("  <tsfc>            0.8 </tsfc>\n");
  if ($ac_augmented) {
  print("  <atsfc>           1.7 </atsfc>\n");
  }
  print("  <bleed>           0.03</bleed>\n");
  print("  <idlen1>         30.0 </idlen1>\n");
  print("  <idlen2>         60.0 </idlen2>\n");
  print("  <maxn1>         100.0 </maxn1>\n");
  print("  <maxn2>         100.0 </maxn2>\n");
  if($ac_augmented == 1) {
    print("  <augmented>         1 </augmented>\n");
    print("  <augmethod>         1 </augmethod>\n");
  }else {
    print("  <augmented>         0 </augmented>\n");
  }
  if($ac_injected == 1) {
    print("  <injected>          1 </injected>\n");
  }else {
    print("  <injected>          0 </injected>\n");
  }
  print("\n");

  print("  <function name=\"IdleThrust\">\n");
  print("   <table>\n");
  print("    <independentVar lookup=\"row\">velocities/mach</independentVar>\n");  
  print("    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n");  
  print("    <tableData>\n");
  print("         -10000     0     10000   20000   30000   40000   50000   60000\n");
  print("     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.1467  0\n");
  print("     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.1342  0\n");
  print("     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.1203  0\n");
  print("     0.6  0.0     0.0     0.0     0.0     0.0276  0.0718  0.1073  0\n");
  print("     0.8  0.0     0.0     0.0     0.0     0.0474  0.0868  0.0900  0\n");
  print("     1.0  0.0     0.0     0.0     0.0     0.0     0.0552  0.0800  0\n");
  print("    </tableData>\n");
  print("   </table>\n");
  print("  </function>\n\n");

  print("  <function name=\"MilThrust\">\n");
  print("   <table>\n");
  print("    <independentVar lookup=\"row\">velocities/mach</independentVar>\n");  
  print("    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n");  
  print("    <tableData>\n");
  print("          -10000       0   10000   20000   30000   40000   50000   60000\n");
  print("     0.0   1.2600  1.0000  0.7400  0.5340  0.3720  0.2410  0.1490  0\n");
  print("     0.2   1.1710  0.9340  0.6970  0.5060  0.3550  0.2310  0.1430  0\n");
  print("     0.4   1.1500  0.9210  0.6920  0.5060  0.3570  0.2330  0.1450  0\n");
  print("     0.6   1.1810  0.9510  0.7210  0.5320  0.3780  0.2480  0.1540  0\n");
  print("     0.8   1.2580  1.0200  0.7820  0.5820  0.4170  0.2750  0.1700  0\n");
  print("     1.0   1.3690  1.1200  0.8710  0.6510  0.4750  0.3150  0.1950  0\n");
  print("     1.2   1.4850  1.2300  0.9750  0.7440  0.5450  0.3640  0.2250  0\n");
  print("     1.4   1.5941  1.3400  1.0860  0.8450  0.6280  0.4240  0.2630  0\n");
  print("    </tableData>\n");
  print("   </table>\n");
  print("  </function>\n\n");

 if ($ac_augmented) {
  print("  <function name=\"AugThrust\">\n");
  print("   <table>\n");
  print("    <independentVar lookup=\"row\">velocities/mach</independentVar>\n");  
  print("    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n");  
  print("    <tableData>\n");
  print("           -10000       0   10000   20000   30000   40000   50000   60000\n");
  print("     0.0    1.1816  1.0000  0.8184  0.6627  0.5280  0.3756  0.2327  0\n");
  print("     0.2    1.1308  0.9599  0.7890  0.6406  0.5116  0.3645  0.2258  0\n");
  print("     0.4    1.1150  0.9474  0.7798  0.6340  0.5070  0.3615  0.2240  0\n");
  print("     0.6    1.1284  0.9589  0.7894  0.6420  0.5134  0.3661  0.2268  0\n");
  print("     0.8    1.1707  0.9942  0.8177  0.6647  0.5309  0.3784  0.2345  0\n");
  print("     1.0    1.2411  1.0529  0.8648  0.7017  0.5596  0.3983  0.2467  0\n");
  print("     1.2    1.3287  1.1254  0.9221  0.7462  0.5936  0.4219  0.2614  0\n");
  print("     1.4    1.4365  1.2149  0.9933  0.8021  0.6360  0.4509  0.2794  0\n");
  print("     1.6    1.5711  1.3260  1.0809  0.8700  0.6874  0.4860  0.3011  0\n");
  print("     1.8    1.7301  1.4579  1.1857  0.9512  0.7495  0.5289  0.3277  0\n");
  print("     2.0    1.8314  1.5700  1.3086  1.0474  0.8216  0.5786  0.3585  0\n");
  print("     2.2    1.9700  1.6900  1.4100  1.2400  0.9100  0.6359  0.3940  0\n");
  print("     2.4    2.0700  1.8000  1.5300  1.3400  1.0000  0.7200  0.4600  0\n");
  print("     2.6    2.2000  1.9200  1.6400  1.4400  1.1000  0.8000  0.5200  0\n");
  print("    </tableData>\n");
  print("   </table>\n");
  print("  </function>\n\n");
 }

 if ($ac_injected) {
  print("  <function name=\"WaterFactor\">\n");
  print("   <table>\n");
  print("    <independentVar lookup=\"row\">velocities/mach</independentVar>\n");  
  print("    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n");  
  print("    <tableData>\n");
  print("            0       50000\n");
  print("     0.0    1.2000  1.2000\n");
  print("     1.0    1.2000  1.2000\n");
  print("    </tableData>\n");
  print("   </table>\n");
  print("  </function>\n\n");

 }

  print("</turbine_engine>\n");
  }


//************************************************
//*  MakeTurboprop()                             *
//************************************************
function MakeTurboprop() {

  global $ac_enginename, $ac_enginepower,
         $ac_augmented, $ac_injected;

// estimate thrust if given power
if(($ac_engineunits == 0) || ($ac_engineunits == 1)) {
  $ac_enginepower *= 2.24;
  }

  print("<turbine_engine name=\"$ac_enginename\">\n");  
  printf("  <milthrust>%8.1f </milthrust>\n", $ac_enginepower);
  print("  <bypassratio>     0.0  </bypassratio>\n");
  print("  <tsfc>            0.55 </tsfc>\n");
  print("  <bleed>           0.03 </bleed>\n");
  print("  <idlen1>         30.0  </idlen1>\n");
  print("  <idlen2>         60.0  </idlen2>\n");
  print("  <maxn1>         100.0  </maxn1>\n");
  print("  <maxn2>         100.0  </maxn2>\n");
  print("  <augmented>         0  </augmented>\n");
  print("  <injected>          0  </injected>\n\n");


  print("  <function name=\"IdleThrust\">\n");
  print("   <table>\n");
  print("    <independentVar lookup=\"row\">velocities/mach</independentVar>\n");  
  print("    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n");  
  print("    <tableData>\n");
  print("         -10000       0   10000   20000   30000   40000   50000\n");
  print("     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.0\n");
  print("     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.0\n");
  print("     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.0\n");
  print("     0.6  0.0     0.0     0.0     0.0276  0.0718  0.0430  0.0\n");
  print("     0.8  0.0     0.0     0.0     0.0     0.0174  0.0086  0.0\n");
  print("     1.0  0.0     0.0     0.0     0.0     0.0     0.0     0.0\n");
  print("   </tableData>\n");
  print("   </table>\n");
  print("  </function>\n\n");

  print("  <function name=\"MilThrust\">\n");
  print("   <table>\n");
  print("    <independentVar lookup=\"row\">velocities/mach</independentVar>\n");  
  print("    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>\n");  
  print("    <tableData>\n");
  print("         -10000       0   10000   20000   30000   40000   50000\n");
  print("     0.0  1.1260  1.0000  0.7400  0.5340  0.3720  0.2410  0.0\n");
  print("     0.2  1.1000  0.9340  0.6970  0.5060  0.3550  0.2310  0.0\n");
  print("     0.4  1.0000  0.6410  0.6120  0.4060  0.3570  0.2330  0.0\n");
  print("     0.6  0.4430  0.3510  0.2710  0.2020  0.1780  0.1020  0.0\n");
  print("     0.8  0.0240  0.0200  0.0160  0.0130  0.0110  0.0100  0.0\n");
  print("     1.0  0.0     0.0     0.0     0.0     0.0     0.0     0.0\n");
  print("    </tableData>\n");
  print("   </table>\n");
  print("  </function>\n\n");

  print("</turbine_engine>\n");
  }


//************************************************
//*  MakeRocket()                                *
//************************************************
function MakeRocket() {

  global $ac_enginename, $ac_enginepower;

  print("<rocket_engine name=\"$ac_enginename\">\n");
  print("  <shr>              1.23 </shr>\n");
  print("  <max_pc>       86556.00 </max_pc>\n");
  print("  <variance>         0.10 </variance>\n");
  print("  <prop_eff>         0.67 </prop_eff>\n");
  print("  <maxthrottle>      1.00 </maxthrottle>\n");
  print("  <minthrottle>      0.40 </minthrottle>\n");
  print("  <slfuelflowmax>   91.50 </slfuelflowmax>\n");
  print("  <sloxiflowmax>   105.20 </sloxiflowmax>\n");
  print("</rocket_engine>\n");
  }

?>
