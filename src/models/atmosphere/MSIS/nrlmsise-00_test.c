/* -------------------------------------------------------------------- */
/* ---------  N R L M S I S E - 0 0    M O D E L    2 0 0 1  ---------- */
/* -------------------------------------------------------------------- */

/* This file is part of the NRLMSISE-00  C source code package - release
 * 20041227
 *
 * The NRLMSISE-00 model was developed by Mike Picone, Alan Hedin, and
 * Doug Drob. They also wrote a NRLMSISE-00 distribution package in 
 * FORTRAN which is available at
 * http://uap-www.nrl.navy.mil/models_web/msis/msis_home.htm
 *
 * Dominik Brodowski implemented and maintains this C version. You can
 * reach him at mail@brodo.de. See the file "DOCUMENTATION" for details,
 * and check http://www.brodo.de/english/pub/nrlmsise/index.html for
 * updated releases of this package.
 */



/* ------------------------------------------------------------------- */
/* ------------------------------ INCLUDES --------------------------- */
/* ------------------------------------------------------------------- */

#include <stdio.h>
#include "nrlmsise-00.h"



/* ------------------------------------------------------------------- */
/* ----------------------------- TEST_GTD7 --------------------------- */
/* ------------------------------------------------------------------- */

void test_gtd7(void) {
  	struct nrlmsise_output output[17];
	struct nrlmsise_input input[17];
  	struct nrlmsise_flags flags;
	struct ap_array aph;
	int i;
	int j;
	/* input values */
  	for (i=0;i<7;i++)
		aph.a[i]=100;
	flags.switches[0]=0;
  	for (i=1;i<24;i++)
  		flags.switches[i]=1;
	for (i=0;i<17;i++) {
		input[i].doy=172;
		input[i].year=0; /* without effect */
  		input[i].sec=29000;
		input[i].alt=400;
		input[i].g_lat=60;
		input[i].g_long=-70;
		input[i].lst=16;
		input[i].f107A=150;
		input[i].f107=150;
		input[i].ap=4;
	}
	input[1].doy=81;
	input[2].sec=75000;
	input[2].alt=1000;
	input[3].alt=100;
	input[10].alt=0;
	input[11].alt=10;
	input[12].alt=30;
	input[13].alt=50;
	input[14].alt=70;
	input[16].alt=100;
	input[4].g_lat=0;
	input[5].g_long=0;
	input[6].lst=4;
	input[7].f107A=70;
	input[8].f107=180;
	input[9].ap=40;
	input[15].ap_a=&aph;
	input[16].ap_a=&aph;
	/* evaluate 0 to 14 */
  	for (i=0;i<15;i++)
  		gtd7(&input[i], &flags, &output[i]);
	/* evaluate 15 and 16 */
  	flags.switches[9]=-1;
  	for (i=15;i<17;i++)
  		gtd7(&input[i], &flags, &output[i]);
	/* output type 1 */
  	for (i=0;i<17;i++) {
		printf("\n");
		for (j=0;j<9;j++)
			printf("%E ",output[i].d[j]);
		printf("%E ",output[i].t[0]);
		printf("%E \n",output[i].t[1]);
		/* DL omitted */
  	}

	/* output type 2 */
	for (i=0;i<3;i++) {
		printf("\n");
		printf("\nDAY   ");
		for (j=0;j<5;j++)
			printf("         %3i",input[i*5+j].doy);
		printf("\nUT    ");
		for (j=0;j<5;j++)
			printf("       %5.0f",input[i*5+j].sec);
		printf("\nALT   ");
		for (j=0;j<5;j++)
			printf("        %4.0f",input[i*5+j].alt);
		printf("\nLAT   ");
		for (j=0;j<5;j++)
			printf("         %3.0f",input[i*5+j].g_lat);
		printf("\nLONG  ");
		for (j=0;j<5;j++)
			printf("         %3.0f",input[i*5+j].g_long);
		printf("\nLST   ");
		for (j=0;j<5;j++)
			printf("       %5.0f",input[i*5+j].lst);
		printf("\nF107A ");
		for (j=0;j<5;j++)
			printf("         %3.0f",input[i*5+j].f107A);
		printf("\nF107  ");
		for (j=0;j<5;j++)
			printf("         %3.0f",input[i*5+j].f107);
		printf("\n\n");
  		printf("\nTINF  ");
		for (j=0;j<5;j++)
			printf("     %7.2f",output[i*5+j].t[0]);
		printf("\nTG    ");
		for (j=0;j<5;j++)
			printf("     %7.2f",output[i*5+j].t[1]);
		printf("\nHE    ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[0]);
		printf("\nO     ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[1]);
		printf("\nN2    ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[2]);
		printf("\nO2    ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[3]);
		printf("\nAR    ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[4]);
		printf("\nH     ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[6]);
		printf("\nN     ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[7]);
		printf("\nANM 0 ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[8]);
		printf("\nRHO   ");
		for (j=0;j<5;j++)
			printf("   %1.3e",output[i*5+j].d[5]);
		printf("\n");
	}
	printf("\n");
}


/* ------------------------------------------------------------------- */
/* -------------------------------- MAIN ----------------------------- */
/* ------------------------------------------------------------------- */

int main(void) {
	test_gtd7();
	return 0;
}
