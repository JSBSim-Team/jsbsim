/**
 * \file Math.cpp
 * \brief Implementation for GeographicLib::Math class
 *
 * Copyright (c) Charles Karney (2015-2021) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#include <GeographicLib/Math.hpp>

#if defined(_MSC_VER)
// Squelch warnings about constant conditional expressions
#  pragma warning (disable: 4127)
#endif

namespace GeographicLib {

  using namespace std;

  void Math::dummy() {
    static_assert(GEOGRAPHICLIB_PRECISION >= 1 && GEOGRAPHICLIB_PRECISION <= 5,
                  "Bad value of precision");
  }

  int Math::digits() {
#if GEOGRAPHICLIB_PRECISION != 5
    return numeric_limits<real>::digits;
#else
    return numeric_limits<real>::digits();
#endif
  }

  int Math::set_digits(int ndigits) {
#if GEOGRAPHICLIB_PRECISION != 5
    (void)ndigits;
#else
    mpfr::mpreal::set_default_prec(ndigits >= 2 ? ndigits : 2);
#endif
    return digits();
  }

  int Math::digits10() {
#if GEOGRAPHICLIB_PRECISION != 5
    return numeric_limits<real>::digits10;
#else
    return numeric_limits<real>::digits10();
#endif
  }

  int Math::extra_digits() {
    return
      digits10() > numeric_limits<double>::digits10 ?
      digits10() - numeric_limits<double>::digits10 : 0;
  }

  template<typename T> T Math::hypot(T x, T y) {
    using std::hypot; return hypot(x, y);
  }

  template<typename T> T Math::expm1(T x) {
    using std::expm1; return expm1(x);
  }

  template<typename T> T Math::log1p(T x) {
    using std::log1p; return log1p(x);
  }

  template<typename T> T Math::asinh(T x) {
    using std::asinh; return asinh(x);
  }

  template<typename T> T Math::atanh(T x) {
    using std::atanh; return atanh(x);
  }

  template<typename T> T Math::copysign(T x, T y) {
    using std::copysign; return copysign(x, y);
  }

  template<typename T> T Math::cbrt(T x) {
    using std::cbrt; return cbrt(x);
  }

  template<typename T> T Math::remainder(T x, T y) {
    using std::remainder; return remainder(x, y);
  }

  template<typename T> T Math::remquo(T x, T y, int* n) {
    using std::remquo; return remquo(x, y, n);
  }

  template<typename T> T Math::round(T x) {
    using std::round; return round(x);
  }

  template<typename T> long Math::lround(T x) {
    using std::lround; return lround(x);
  }

  template<typename T> T Math::fma(T x, T y, T z) {
    using std::fma; return fma(x, y, z);
  }

  template<typename T> T Math::sum(T u, T v, T& t) {
    GEOGRAPHICLIB_VOLATILE T s = u + v;
    GEOGRAPHICLIB_VOLATILE T up = s - v;
    GEOGRAPHICLIB_VOLATILE T vpp = s - up;
    up -= u;
    vpp -= v;
    t = -(up + vpp);
    // u + v =       s      + t
    //       = round(u + v) + t
    return s;
  }

  template<typename T> T Math::AngRound(T x) {
    static const T z = 1/T(16);
    if (x == 0) return 0;
    GEOGRAPHICLIB_VOLATILE T y = abs(x);
    // The compiler mustn't "simplify" z - (z - y) to y
    y = y < z ? z - (z - y) : y;
    return x < 0 ? -y : y;
  }

  template<typename T> void Math::sincosd(T x, T& sinx, T& cosx) {
    // In order to minimize round-off errors, this function exactly reduces
    // the argument to the range [-45, 45] before converting it to radians.
    using std::remquo;
    T r; int q = 0;
    // N.B. the implementation of remquo in glibc pre 2.22 were buggy.  See
    // https://sourceware.org/bugzilla/show_bug.cgi?id=17569
    // This was fixed in version 2.22 on 2015-08-05
    r = remquo(x, T(90), &q);   // now abs(r) <= 45
    r *= degree<T>();
    // g++ -O turns these two function calls into a call to sincos
    T s = sin(r), c = cos(r);
    switch (unsigned(q) & 3U) {
    case 0U: sinx =  s; cosx =  c; break;
    case 1U: sinx =  c; cosx = -s; break;
    case 2U: sinx = -s; cosx = -c; break;
    default: sinx = -c; cosx =  s; break; // case 3U
    }
    // Set sign of 0 results.  -0 only produced for sin(-0)
    if (x != 0) { sinx += T(0); cosx += T(0); }
  }

  template<typename T> T Math::sind(T x) {
    // See sincosd
    using std::remquo;
    T r; int q = 0;
    r = remquo(x, T(90), &q); // now abs(r) <= 45
    r *= degree<T>();
    unsigned p = unsigned(q);
    r = p & 1U ? cos(r) : sin(r);
    if (p & 2U) r = -r;
    if (x != 0) r += T(0);
    return r;
  }

  template<typename T> T Math::cosd(T x) {
    // See sincosd
    using std::remquo;
    T r; int q = 0;
    r = remquo(x, T(90), &q); // now abs(r) <= 45
    r *= degree<T>();
    unsigned p = unsigned(q + 1);
    r = p & 1U ? cos(r) : sin(r);
    if (p & 2U) r = -r;
    return T(0) + r;
  }

  template<typename T> T Math::tand(T x) {
    static const T overflow = 1 / sq(numeric_limits<T>::epsilon());
    T s, c;
    sincosd(x, s, c);
    return c != 0 ? s / c : (s < 0 ? -overflow : overflow);
  }

  template<typename T> T Math::atan2d(T y, T x) {
    // In order to minimize round-off errors, this function rearranges the
    // arguments so that result of atan2 is in the range [-pi/4, pi/4] before
    // converting it to degrees and mapping the result to the correct
    // quadrant.
    int q = 0;
    if (abs(y) > abs(x)) { swap(x, y); q = 2; }
    if (x < 0) { x = -x; ++q; }
    // here x >= 0 and x >= abs(y), so angle is in [-pi/4, pi/4]
    T ang = atan2(y, x) / degree<T>();
    switch (q) {
      // Note that atan2d(-0.0, 1.0) will return -0.  However, we expect that
      // atan2d will not be called with y = -0.  If need be, include
      //
      //   case 0: ang = 0 + ang; break;
      //
      // and handle mpfr as in AngRound.
    case 1: ang = (y >= 0 ? 180 : -180) - ang; break;
    case 2: ang =  90 - ang; break;
    case 3: ang = -90 + ang; break;
    default: break;
    }
    return ang;
  }

  template<typename T> T Math::atand(T x)
  { return atan2d(x, T(1)); }

  template<typename T> T Math::eatanhe(T x, T es)  {
    using std::atanh;
    return es > T(0) ? es * atanh(es * x) : -es * atan(es * x);
  }

  template<typename T> T Math::taupf(T tau, T es) {
    // Need this test, otherwise tau = +/-inf gives taup = nan.
    using std::isfinite; using std::hypot;
    if (isfinite(tau)) {
      T tau1 = hypot(T(1), tau),
        sig = sinh( eatanhe(tau / tau1, es ) );
      return hypot(T(1), sig) * tau - sig * tau1;
    } else
      return tau;
  }

  template<typename T> T Math::tauf(T taup, T es) {
    using std::hypot;
    static const int numit = 5;
    // min iterations = 1, max iterations = 2; mean = 1.95
    static const T tol = sqrt(numeric_limits<T>::epsilon()) / 10;
    static const T taumax = 2 / sqrt(numeric_limits<T>::epsilon());
    T e2m = T(1) - sq(es),
      // To lowest order in e^2, taup = (1 - e^2) * tau = _e2m * tau; so use
      // tau = taup/e2m as a starting guess. Only 1 iteration is needed for
      // |lat| < 3.35 deg, otherwise 2 iterations are needed.  If, instead, tau
      // = taup is used the mean number of iterations increases to 1.999 (2
      // iterations are needed except near tau = 0).
      //
      // For large tau, taup = exp(-es*atanh(es)) * tau.  Use this as for the
      // initial guess for |taup| > 70 (approx |phi| > 89deg).  Then for
      // sufficiently large tau (such that sqrt(1+tau^2) = |tau|), we can exit
      // with the intial guess and avoid overflow problems.  This also reduces
      // the mean number of iterations slightly from 1.963 to 1.954.
      tau = abs(taup) > 70 ? taup * exp(eatanhe(T(1), es)) : taup/e2m,
      stol = tol * max(T(1), abs(taup));
    if (!(abs(tau) < taumax)) return tau; // handles +/-inf and nan
    for (int i = 0; i < numit || GEOGRAPHICLIB_PANIC; ++i) {
      T taupa = taupf(tau, es),
        dtau = (taup - taupa) * (1 + e2m * sq(tau)) /
        ( e2m * hypot(T(1), tau) * hypot(T(1), taupa) );
      tau += dtau;
      if (!(abs(dtau) >= stol))
        break;
    }
    return tau;
  }

    template<typename T> bool Math::isfinite(T x) {
      using std::isfinite; return isfinite(x);
    }

    template<typename T> T Math::NaN() {
#if defined(_MSC_VER)
      return numeric_limits<T>::has_quiet_NaN ?
        numeric_limits<T>::quiet_NaN() :
        (numeric_limits<T>::max)();
#else
      return numeric_limits<T>::has_quiet_NaN ?
        numeric_limits<T>::quiet_NaN() :
        numeric_limits<T>::max();
#endif
    }

    template<typename T> bool Math::isnan(T x) {
      using std::isnan; return isnan(x);
    }

  template<typename T> T Math::infinity() {
#if defined(_MSC_VER)
      return numeric_limits<T>::has_infinity ?
        numeric_limits<T>::infinity() :
        (numeric_limits<T>::max)();
#else
      return numeric_limits<T>::has_infinity ?
        numeric_limits<T>::infinity() :
        numeric_limits<T>::max();
#endif
    }

  /// \cond SKIP
  // Instantiate
#define GEOGRAPHICLIB_MATH_INSTANTIATE(T)                               \
  template T    GEOGRAPHICLIB_EXPORT Math::hypot    <T>(T, T);          \
  template T    GEOGRAPHICLIB_EXPORT Math::expm1    <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::log1p    <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::asinh    <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::atanh    <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::cbrt     <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::remainder<T>(T, T);          \
  template T    GEOGRAPHICLIB_EXPORT Math::remquo   <T>(T, T, int*);    \
  template T    GEOGRAPHICLIB_EXPORT Math::round    <T>(T);             \
  template long GEOGRAPHICLIB_EXPORT Math::lround   <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::copysign <T>(T, T);          \
  template T    GEOGRAPHICLIB_EXPORT Math::fma      <T>(T, T, T);       \
  template T    GEOGRAPHICLIB_EXPORT Math::sum      <T>(T, T, T&);      \
  template T    GEOGRAPHICLIB_EXPORT Math::AngRound <T>(T);             \
  template void GEOGRAPHICLIB_EXPORT Math::sincosd  <T>(T, T&, T&);     \
  template T    GEOGRAPHICLIB_EXPORT Math::sind     <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::cosd     <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::tand     <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::atan2d   <T>(T, T);          \
  template T    GEOGRAPHICLIB_EXPORT Math::atand    <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::eatanhe  <T>(T, T);          \
  template T    GEOGRAPHICLIB_EXPORT Math::taupf    <T>(T, T);          \
  template T    GEOGRAPHICLIB_EXPORT Math::tauf     <T>(T, T);          \
  template bool GEOGRAPHICLIB_EXPORT Math::isfinite <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::NaN      <T>();              \
  template bool GEOGRAPHICLIB_EXPORT Math::isnan    <T>(T);             \
  template T    GEOGRAPHICLIB_EXPORT Math::infinity <T>();

  // Instantiate with the standard floating type
  GEOGRAPHICLIB_MATH_INSTANTIATE(float)
  GEOGRAPHICLIB_MATH_INSTANTIATE(double)
#if GEOGRAPHICLIB_HAVE_LONG_DOUBLE
  // Instantiate if long double is distinct from double
  GEOGRAPHICLIB_MATH_INSTANTIATE(long double)
#endif
#if GEOGRAPHICLIB_PRECISION > 3
  // Instantiate with the high precision type
  GEOGRAPHICLIB_MATH_INSTANTIATE(Math::real)
#endif

#undef GEOGRAPHICLIB_MATH_INSTANTIATE

  // Also we need int versions for Utility::nummatch
  template int GEOGRAPHICLIB_EXPORT Math::NaN     <int>();
  template int GEOGRAPHICLIB_EXPORT Math::infinity<int>();
  /// \endcond

} // namespace GeographicLib
