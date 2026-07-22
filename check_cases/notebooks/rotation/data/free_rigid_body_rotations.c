/* Exact free rigid body rotation.
 *
 * Numerical implementation of the exact dynamics of free rigid bodies
 * Ramses van Zon, Jeremy Schofield, Dec 15, 2006
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <gsl/gsl_sf_ellint.h>
#include <gsl/gsl_sf_elljac.h>

#define MACHPREC 3.E-16
#define MY_PI_2 1.57079632679489661923

double F(double x, double m)
{ return x*gsl_sf_ellint_RF(1.0-x*x, 1.0-m*x*x, 1.0, GSL_PREC_DOUBLE); }

void SNCNDN(double x, double m, double *s, double *c, double *d)
{  (void)gsl_sf_elljac_e(x, m, s, c, d); }

/* The following macro performs an in-situ matrix multiplication: */
#define RIGHTMULTMATRIX(M1, M2, x, y)\
x = M1[0][0]; y = M1[0][1];\
M1[0][0] *= M2[0][0]; M1[0][0] += y*M2[1][0]+M1[0][2]*M2[2][0];\
M1[0][1] *= M2[1][1]; M1[0][1] += x*M2[0][1]+M1[0][2]*M2[2][1];\
M1[0][2] *= M2[2][2]; M1[0][2] += x*M2[0][2]+y*M2[1][2];\
x = M1[1][0]; y = M1[1][1];\
M1[1][0] *= M2[0][0]; M1[1][0] += y*M2[1][0]+M1[1][2]*M2[2][0];\
M1[1][1] *= M2[1][1]; M1[1][1] += r*M2[0][1]+M1[1][2]*M2[2][1];\
M1[1][2] *= M2[2][2]; M1[1][2] += r*M2[0][2]+y*M2[1][2];\
x = M1[2][0]; y = M1[2][1];\
M1[2][0] *= M2[0][0]; M1[2][0] += y*M2[1][0]+M1[2][2]*M2[2][0];\
M1[2][1] *= M2[1][1]; M1[2][1] += x*M2[0][1]+M1[2][2]*M2[2][1];\
M1[2][2] *= M2[2][2]; M1[2][2] += x*M2[0][2]+y*M2[1][2];

/* A number of precomputed expressions are taken together in the
following structure: */
typedef struct {
    int orderFlag, NT;
    double I1, I2, I3, omega1m, omega2m, omega3m, omegap, freq, epsilon,
           A1, A2, L, m, *cr, *ci, B[3][3];
} Top;

/* The following function performs some pre-calculations. Input: Ix, *
* Iy and Iz are the three principal inertial moments (it is assumed *
* that Ix<Iy<Iz or Ix>Iy>Iz), omegax, omegay and omegaz are the *
* angular velocities in the principal axis (i.e. body) frame, and A *
* is the initial attitude matrix. Returns a Top structure containing *
* all necessary precomputed parameters to efficiently calculate the *
* omega’s and A’s at later times (see the Evolution function below): */
Top Initialization(double Ix, double Iy, double Iz,
                   double omegax, double omegay, double omegaz, double A[3][3])
{
    double
        a = Ix*omegax, /* L1, angular momentum in x-direction */
        b = Iy*omegay, /* L2, angular momentum in y-direction */
        c = Iz*omegaz, /* L3, angular momentum in z-direction */
        d = a*a+b*b+c*c, /* L.L, norm squared of angular momentum */
        e = a*omegax+b*omegay+c*omegaz, /* 2 E */
        f, omega1, omega2, omega3, Kp, q, r, i, s, g, h;
    int n;
    Top R; /* will contain the precomputed numbers */
    R.L = sqrt(d); /* |L| */
    if ( (e>d/Iy && Ix<Iz) || (e<d/Iy && Ix>Iz) ) {
        /* Check if Jacobi-ordering is obeyed: */
        R.orderFlag = 1;/* Jacobi ordering ensured by swapping the order of x and z */
        R.I1 = Iz; /* directions and reversing the y direction */
        R.I2 = Iy;
        R.I3 = Ix;
        omega1 = omegaz;
        omega2 = -omegay;
        omega3 = omegax;
        f = hypot(b, c);

        /* Fill the matrix R.B with transpose of T1(0), using ordering: */
        R.B[0][0] = -f/R.L; R.B[0][1] = b*a/R.L/f; R.B[0][2] = a*c/R.L/f;
        R.B[1][0] = 0; R.B[1][1] = -c/f; R.B[1][2] = b/f;
        R.B[2][0] = a/R.L; R.B[2][1] = b/R.L; R.B[2][2] = c/R.L;
    } else {
        R.orderFlag = 0; /* Jacobi ordering correct */
        R.I1 = Ix;
        R.I2 = Iy;
        R.I3 = Iz;
        omega1 = omegax;
        omega2 = omegay;
        omega3 = omegaz;
        f = hypot(a, b);

        /* Fill the matrix R.B with the transpose of T1(0): */
        R.B[0][0] = a*c/R.L/f; R.B[0][1] = b*c/R.L/f; R.B[0][2] = -f/R.L;
        R.B[1][0] = -b/f; R.B[1][1] = a/f; R.B[1][2] = 0;
        R.B[2][0] = a/R.L; R.B[2][1] = b/R.L; R.B[2][2] = c/R.L;
    }
    RIGHTMULTMATRIX(R.B, A, r, i); /* calculate B */
    a = d-e*R.I3; /* compute four subexpressions */
    b = d-e*R.I1;
    c = R.I1-R.I3;
    d = R.I2-R.I3;
    R.omega1m = copysign(sqrt(a/R.I1/c), omega1);
    R.omega2m = -copysign(sqrt(a/R.I2/d), omega1);
    R.omega3m = copysign(sqrt(-b/R.I3/c), omega3);
    R.omegap = d*copysign(sqrt(b/(-d)/R.I1/R.I2/R.I3), omega3); /* prec. freq */
    R.m = a*(R.I2-R.I1)/(b*d); /* ellipic parameter m */
    R.epsilon = F(omega2/R.omega2m, R.m); /* initial phase */
    R.freq = MY_PI_2/F(1.0, R.m); /* frequency relative to precession */
    Kp = F(1.0, 1.0-R.m); /* K’, complementary quarter period */
    q = exp(-2.0*R.freq*Kp); /* elliptic nome */
    e = exp(R.freq*(copysign(Kp, omega3)-F(R.I3*R.omega3m/R.L, 1.0-R.m)));
    f = e*e; /* this f is equal to xi */
    R.A2 = R.L/R.I1+R.freq*R.omegap*(f+1)/(f-1); /* first term in A2 series */
    a = 1.0; /* a will be the 2n-th power of q */
    b = 1.0; /* b will be the nth power of xi */
    n = 1;

    do {
        a *= q*q; /* update a and b recursively */
        b *= f;
        c = -2.0*R.freq*R.omegap*a/(1-a)*(b-1/b); /* the next term in A2 */
        R.A2 += c; /* add a term of the series */
        n++;
    } while (fabs(c/R.A2)>MACHPREC && n<10000);/* stop if converged or n too big */

    /* determine upper bound on number of terms needed in theta function series: */
    R.NT = (int)(log(MACHPREC)/log(q)+0.5);
    R.cr = (double *)malloc(sizeof(double)*(R.NT+1)); /* allocate memory */
    R.ci = (double *)malloc(sizeof(double)*(R.NT+1));
    a = 1.0; /* a will be the 2n-th power of q */
    b = 1.0; /* b will be (-1)^n q^{n(n+1)} */
    R.cr[0] = (e+1/e); /* zeroth term in the series for real and imag part */
    R.ci[0] = -(e-1/e);
    s = sin(R.freq*R.epsilon); /* s = sin((2n+1)x), c = cos((2n+1)x) */
    c = cos(R.freq*R.epsilon);
    g = 2.0*c*s; /* sin(2x) */
    h = 2.0*c*c-1.0; /* cos(2x) */
    r = R.cr[0]*s; /* real part of the theta function, zeroth term */
    i = R.ci[0]*c; /* imaginary part of the theta function, zeroth term */
    for (n = 1; n <= R.NT; n++) {
        e *= f; /* e = xi^{n+1/2} */
        a *= q*q; /* update a and b recursively */
        b *= -a;
        R.cr[n] = b*(e+1/e); /* compute next coefficient of theta series */
        R.ci[n] = -b*(e-1/e);
        d = s; /* compute sin and cos recursively */
        s = h*s+g*c;
        c = h*c-g*d;
        r += R.cr[n]*s; /* add next terms */
        i += R.ci[n]*c;
        if ( (fabs(R.cr[n]) < MACHPREC) && (fabs(R.ci[n]) < MACHPREC) )
            R.NT = n-1; /* if converged, adjust NT */
    }
    R.A1 = atan2(i, r); /* compute arg(r, i) */
    return R; /* done, return all precomputed values */
}

/* The following function calculates the angular velocities and attitude *
* matrix at a time t. Input: R is a pointer to a Top structure containing *
* all necessary precomputed parameters to efficiently calculate the omega’s *
* and A’s, and t is a time. R should be generated by Initialize function. *
* Output: *omegax, *omegay and *omegaz are filled with the angular *
* velocities in the principal axis (i.e. body) frame at time t and A is *
* filled with the attitude matrix at time t. */
void Evolution(Top *R, double t, double *omegax, double *omegay, double *omegaz,
               double A[3][3])
{
    int n;
    double omega1, omega2, omega3, r, i, u, s, c, g, h, f;
    u = R->omegap*t+R->epsilon; /* compute argument of elliptic functions */
    SNCNDN(u, R->m, &omega2, &omega1, &omega3); /* compute sn, cn, dn */
    omega1 *= R->omega1m; /* multiply by the amplitudes */
    omega2 *= R->omega2m; /* of the respective ang. vel. */
    omega3 *= R->omega3m;

    if (R->orderFlag == 1) { /* check for Jacobi ordering */
        *omegax = omega3; /* if adjusted, invert x and z and reverse y */
        *omegay = -omega2;
        *omegaz = omega1;
        omega1 *= R->I1;
        omega2 *= R->I2;
        omega3 *= R->I3;
        f = hypot(omega1, omega2);
        /* compute T1(t), taking into account the ordering: */
        A[0][0] = -f/R->L; A[0][1] = 0; A[0][2] = omega3/R->L;
        A[1][0] = -omega2*omega3/R->L/f; A[1][1] = -omega1/f; A[1][2] = -omega2/R->L;
        A[2][0] = omega1*omega3/R->L/f; A[2][1] = -omega2/f; A[2][2] = omega1/R->L;
    } else {
        *omegax = omega1; /* no adjustment necessary */
        *omegay = omega2;
        *omegaz = omega3;
        omega1 *= R->I1;
        omega2 *= R->I2;
        omega3 *= R->I3;
        f = hypot(omega1, omega2);
        /* compute T1(t): */
        A[0][0] = omega1*omega3/R->L/f; A[0][1] = -omega2/f; A[0][2] = omega1/R->L;
        A[1][0] = omega2*omega3/R->L/f; A[1][1] = omega1/f; A[1][2] = omega2/R->L;
        A[2][0] = -f/R->L; A[2][1] = 0; A[2][2] = omega3/R->L;
    }

    s = sin(R->freq*u); /* s = sin((2n+1)x), c = cos((2n+1)x) with x = freq*u */
    c = cos(R->freq*u);
    g = 2.0*c*s; /* g = sin 2x, h = cos 2x, used in recursion */
    h = 2.0*c*c-1.0;
    r = R->cr[0]*s; /* zeroth term in series for the theta function */
    i = R->ci[0]*c;

    for (n = 1; n <= R->NT; n++) { /* compute series */
        u = s;
        s = h*s+g*c; /* computes sin((2n+1)x) and cos((2n+1)x recursively */
        c = h*c-g*u;
        r += R->cr[n]*s; /* next term in series for theta function */
        i += R->ci[n]*c;
    }

    s = sin(R->A1+R->A2*t); /* s = sin(A_1+A_2 t) */
    c = cos(R->A1+R->A2*t); /* c = cos(A_1+A_2 t) */
    u = s; /* use addition formula to compute s = sin psi and c = cos psi */
    s = s*r-c*i; /* where psi = A_1+A_2 t+arg(r, i) */
    c = c*r+u*i;
    u = hypot(r, i);
    s /= u;
    c /= u;

    for (n = 0; n<3; n++) { /* perform multiplication with T2’ */
        u = A[n][0];
        A[n][0] = A[n][0]*c-A[n][1]*s;
        A[n][1] = A[n][1]*c+u*s;
    }
    RIGHTMULTMATRIX(A, R->B, r, i); /* gives the final attitude matrix */
}

int main(void) {
    /* Initial conditions */
    double t_final = 10;
    double frequency = 60.0;
    double Ixx = 10.0, Iyy = 20.0, Izz = 26.0;
    double omegax = 1.0, omegay = 15.0, omegaz = 1.0;
    double Ti2b[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

    /* Initialization */
    double dt = 1.0 / frequency;
    Top R = Initialization(Ixx, Iyy, Izz, omegax, omegay, omegaz, Ti2b);

    for(double t=0.0; t<t_final; t+=dt) {
        Evolution(&R, t, &omegax, &omegay, &omegaz, Ti2b);
        printf("%f %e %e %e %e %e %e %e %e %e %e %e %e\n", t,
                                    omegax, omegay, omegaz,
                                    Ti2b[0][0], Ti2b[0][1], Ti2b[0][2],
                                    Ti2b[1][0], Ti2b[1][1], Ti2b[1][2],
                                    Ti2b[2][0], Ti2b[2][1], Ti2b[2][2]
                                );
    }

    free(R.cr);
    free(R.ci);
}
