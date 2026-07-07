// -*- coding: utf-8 -*-
// Marc B. Reynolds, 2026
// Public Domain under http://unlicense.org, see link for details.
//
// This header include compile-time and runtime checks for compiler 
// options that "break" floating-point portability. This optionally 
// includes options that are "correct" but hurt performance supporting
// features that the vast majority of user shouldn't want.
// 
// USAGE: include this file in any source file (and/or any header
// which contains inline functions which need protecting) and:
// ∙ any "bad" compiler options that can be detected by a predefined
//   macro will trigger a static_assert with a message.  These
//   can be disabled by defining FP_CC_COMPILE_TIME_DISABLED
//   before including this header.
// ∙ at initialization time (before 'main' is executed) each
//   translation unit which includes this header run a series
//   of runtime checks.
// 
// LIMITATIONS:
// ∙ There's no automagic way (I can think of) to expand the name
//   of the source file so need to define FP_CC_FILE for output
//   to not report "(unknown)"  (__FILE__ expands to this header)
// ∙ runtime checks (obviously) expand by the compiler settings
//   at the point this header is included. So can't catch if they
//   are modified in the source file (pragmas, etc) at some later
//   point in the file. Main "concern" here is attempting to
//   protect some "inline" function (in header to "user"). Note
//   there's "no" solution fot this if options are modified by pragmas
//   in effectively hostile source code (other than don't provide
//   inline functions that bad options can murder).
// ∙ lowest optimization levels might trigger errors and warnings.
//   GCC: as of 14.2.0 with -O0 -fno-math-errno the no math errno
//   gets ignored and we would get a warning if it wasn't short
//   circuited below.
// ∙ Currently with GCC the init time functions are called before
//   the one in `crtfastmath` so it will not detect flushing
//   denormals to zero since the control word has not been yet
//   changed.
// ∙ In MSVC the runtime checks will not happen if global optimizations
//   have been enabled (/GL). The compiler thinks they are
//   unreachable and are removed. (can't think of a zero user friction
//   way to correct this).
// ∙ many more!
//
// NOTES:
// ∙ assumes at least C99 
// ∙ there are purposefully redundant compile & runtime checks
//   (lack of faith in compilers, evolution of compilers and myself)
// 
//────────────────────────────────────────────────────────────────────────
// SUPPORTED CHECKS and configuration
// ∙ compile time checks can be disable by: #define FP_CC_COMPILE_TIME_DISABLED
//   Mainly for internal dev (checking runtime checks are catching) but
//   since this is "fail fast" can be used to discover all offending TUs
//   once.
//
// ∙ checks that math errno has been turned off. This is an anti-feature
//   that should have been disabled by default decades ago. IEEE-754 won.
//   This is like (by default) assuming bytes aren't 8-bits or that signed
//   numbers aren't 2s-complement. Disabled by default for MSVC since
//   it doesn't provide an option (other than fastmath equiv)
//     ∙ GCC/clang: -fno-math-errno
//     ∙ compile time & runtime
//     ∙ disable both with FP_CC_ERRNO_SKIP
//   NOTE: purely performance related. The biggest concern about math
//   errno enabled is hardware sqrt.
//
// ∙ floating point exceptions: check disabled by default
//     ∙ to warn if disabled: #define FP_CC_EXCEPTIONS 0 
//       to warn if enable:   #define FP_CC_EXCEPTIONS 1
//     ∙ GCC/clang: -fno-trapping-math (disable) clang's default
//       GCC/clang: -ftrapping-math    (enable)  GCC's default
//     ∙ compile time & optional runtime
//   NOTE: purely performance related. IMHO should be disabled since
//   it's almost unused and useless on hardware from the past few
//   decades. 
//
// ∙ checks that automatic fusing is disabled. FMAs are the best addition
//   IMHO to floating point since the OG IEEE-754 spec. And they are an
//   ice-pick in the eye if you allow the compiler change what you wrote.
//     ∙ GCC/clang: -ffp-contract=off
// ∙ finite math
//     ∙ compile time & runtime
// ∙ associative transforms
//     ∙ compile time & runtime
// ∙ flushing denormals
//     ∙ runtime
// ∙ reciprocals
//     ∙ runtime
// ∙ respect infinities
// ∙ respect NaNs
// ∙ respect signed zeroes
//
//────────────────────────────────────────────────────────────────────────
// Some compiler references:
// GCC
// clang
// ∙ fp-model & options: https://clang.llvm.org/docs/UsersManual.html#controlling-floating-point-behavior
// ∙ pragmas: https://clang.llvm.org/docs/LanguageExtensions.html#extensions-to-specify-floating-point-flags
// MSVC
// ∙ option pragmas: https://learn.microsoft.com/en-us/cpp/preprocessor/float-control
// ∙ fenv_access & fp_contract : see sidebar from previous

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <errno.h>


//────────────────────────────────────────────────────────────────────────
// per translation unit: compile time checks
// ∙ allow disabling compile time checks: mostly for internal development
//   (verify that runtime checks are catching bad options)
#if !defined(FP_CC_COMPILE_TIME_DISABLED)

// float point exceptions (sticky bits)
// Most people most of the time aren't going to care about this.
// ∙ by default: ignored
// ∙ if FP_CC_EXCEPTIONS is defined then:
//   0 = want disabled (error if enabled)
//   1 = want enabled  (error if disabled)
#if defined(FP_CC_EXCEPTIONS)
#if (FP_CC_EXCEPTIONS)
static_assert((math_errhandling & MATH_ERREXCEPT)!=0, "error: floating exceptions disabled");
#else
static_assert((math_errhandling & MATH_ERREXCEPT)==0, "error: floating exceptions enabled");
#endif
#endif


#if   defined(__GNUC__)

#if !defined(FP_CC_ERRNO_SKIP)
// Visual C doesn't provided an isolated disable (just with fast math)
static_assert((math_errhandling & MATH_ERRNO)==0, "error: math errno enabled");
#endif

#if defined(__FAST_MATH__)
static_assert(0, "error: -ffast-math is disallowed");
#endif

#if defined(__ASSOCIATIVE_MATH__)
static_assert(0, "error: -fassociative-math is disallowed");
#endif

#if (defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__)) 
static_assert(0, "error: -ffinite-math-only is disallowed");
#endif

#elif defined(_MSC_VER)
//────────────────────────────────────────────────────────────────────────
// Microsoft compiler specific. I need to re-examine fp:precise and fp:strict
// WRT which macro are defined.

#if defined(_M_FP_CONTRACT)
static_assert(0, "error: /fp:contract is disallowed");
#endif

#if defined(_M_FP_FAST)
static_assert(0, "error: /fp:fast is disallowed");
#endif

#else
  static_assert(0, "compiler needs some defines");
#endif

#else
//────────────────────────────────────────────────────────────────────────
// anything other than GCC/clang (compatiable) and MSC would go here
#endif


//────────────────────────────────────────────────────────────────────────
// runtime checks

#if defined(_MSC_VER)
  #define FP_CC_FUNC __declspec(noinline)
  #define fp_cc_retain(A,B)
#elif defined(__GNUC__)

#if !defined(FP_CC_INTERNAL_DEBUG)
// currently disabled to prevent "unused function" warnings. temp hack.
#define FP_CC_FUNC __attribute__((noinline))
#else
// used & retain are to handle the case where the function optimizes
// to trival (returns an input or constant) with both GCC and clang
// allow to "inline", the function is then is unreachable and not emitted.
// (dev aid for disasm inspection)
#define FP_CC_FUNC __attribute__((noinline,used,retain))
#endif


#if defined(FP_CC_INTERNAL_DEBUG)
// this attempts to prevent an inline and elimiation of a function that
// reduces to trival (another dev aid as per unused & retain above)
#define fp_cc_retain(A,B) do { __asm__ ("" : "+x" (A), "+x"(B)); } while (0)
#else
#define fp_cc_retain(A,B)
#endif

#else
  static_assert(0, "needs compiler define for noinline");
#endif

// "user" aid function: more verbose output from runtime checks
#if defined(FP_CC_VERBOSE)
#define FP_CC_DUMP(...) fprintf(stderr, __VA_ARGS__)
#else
#define FP_CC_DUMP(...)
#endif

// math errno is a curse that should have died decades ago.
// biggest concern in sqrt & sqrtf which can lower into a
// single hardware op. Visual-C adds insult to injury by not
// providing a seperate option for errno nor intrinsic sqrts
// for scalars w/o errno. Gotta roll your own.

#if !defined(FP_CC_ERRNO_SKIP)

#if defined(__GNUC__) && !defined(__FAST_MATH__)
  #if !defined(__clang__) && !defined(__OPTIMIZE__)
    // GCC (as of 14.2.0) ignores -fno-math-errno at -O0
    #define FP_CC_ERRNO_SKIP
  #endif

#elif defined(_MSC_VER)
  #define FP_CC_ERRNO_SKIP
#endif

#ifndef FP_CC_SQRTF
#define FP_CC_SQRTF sqrtf
#endif

#endif


static inline uint32_t fp_cc_to_bits(float x)
{
  uint32_t u; memcpy(&u, &x, 4); return u;
}

static inline float fp_cc_from_bits(uint32_t x)
{
  float f; memcpy(&f, &x, 4); return f;
}

// noinline since parameters are (mostly) constants so we must disallow
// optimizing the test away (see macro note above).
// note: the body of each must be sufficent to perform the undesired
// optimization.

static float FP_CC_FUNC fp_cc_sqrt(float x)
{
  fp_cc_retain(x,x);
  return FP_CC_SQRTF(x);
}

static float FP_CC_FUNC fp_cc_add(float a, float b)
{
  fp_cc_retain(a,b);
  return a+b;
}


//────────────────────────────────────────────────────────────────────────
// per translation unit: runtime checks

// can automagically determine base source file in GCC & clang.
// others need to define FP_CC_FILE to report something other than
// (unknown)
#ifndef FP_CC_FILE
#if defined(__GNUC__)
#define FP_CC_FILE __BASE_FILE__ 
#else
#define FP_CC_FILE "(unknown)"
#endif
#endif

typedef enum {
  fp_cc_ok             = 0,
  fp_cc_errno          = 1u << 0,
  fp_cc_exceptions     = 1u << 1,
  fp_cc_fused          = 1u << 2,
  fp_cc_associative    = 1u << 3,
  fp_cc_reciprocal     = 1u << 4,
  fp_cc_daz            = 1u << 5,
  fp_cc_no_signed_zero = 1u << 6,
  fp_cc_no_honor_nan   = 1u << 7,
  fp_cc_no_honor_inf   = 1u << 8,
} fp_cc_error_t;


// correct result is integer 0 helper
static uint32_t FP_CC_FUNC fp_cc_check_int_zero(uint32_t r, char* msg, fp_cc_error_t flag)
{
  if (r == 0) return fp_cc_ok;

  fprintf(stderr, "\n  error: %s", msg);

  return flag;
}


// correct result is non-zero helper
static uint32_t FP_CC_FUNC fp_cc_check_not_zero(float r, char* msg, fp_cc_error_t flag)
{
  if (!(r == 0.f)) return fp_cc_ok;

  fprintf(stderr, "\n  error: %s", msg);

  return flag;
}

//────────────────────────────────────────────────────────────────────────
// runtime check if math is setting errno by checking sqrtf
// (or provided replacement: looking at visual-c here)
// ∙ GCC, clang: "should" never trigger since it's covered by a compile time check
// ∙ MSC: going to get a compile time and run time warnings unless a
//   replacement has been provided or check is disabled.
static uint32_t FP_CC_FUNC fp_cc_check_errno(void)
{
#if !defined(FP_CC_ERRNO_SKIP)
  errno = 0;

  (void)fp_cc_sqrt(-1.f);
  
  if (errno == 0) return 0;
  
  fprintf(stderr, "\n  warning: sqrt setting errno");
  
  return fp_cc_errno;
#else
  return 0;
#endif  
}


//────────────────────────────────────────────────────────────────────────
// check if the control word has been modified to flush denormals
// on load and/or result

// add min_denorm to itself returns a denormal. if "flush on load" or "flush on result"
// is enabled then result will be zero. Notice can only catch if the control word has
// already be modified at the time of execution.
//
// NOTE: a false postive can occur if the compiler converts fp_cc_min_denormal to zero
static uint32_t FP_CC_FUNC fp_cc_check_daz(void)
{
  // (a,b) are bit patterns of min_denormal and 2*min_denormal
  uint32_t a = 1;
  uint32_t b = a+1;
  float    f;

  fp_cc_retain(b,a);

  // attempt to move a denormal into a register w/o a floating point operation
  // (frown..not useful as is).
  f  = fp_cc_from_bits(b);

  return fp_cc_check_not_zero(fp_cc_add(f,f),
                              "denormals flushing to zero",
                              fp_cc_daz);  
}


//────────────────────────────────────────────────────────────────────────
// check if applying (non-error free) associative transforms

// computes the error term of (a+b)
static float FP_CC_FUNC fp_cc_add_error(float a, float b)
{
  fp_cc_retain(a,b);
  return b-((a+b)-a);
}

static uint32_t FP_CC_FUNC fp_cc_check_associative(void)
{
  static const float a = 1.f;
  static const float b = 0x1.6a09e6p-40f;

  float r = fp_cc_add_error(a,b);

  return fp_cc_check_not_zero(r, "associative transform", fp_cc_associative);
}


//────────────────────────────────────────────────────────────────────────
// check if transforming a*b+c info fma(a,b,c)

static float FP_CC_FUNC fp_cc_mms(float a, float b, float c, float d)
{
  fp_cc_retain(a,b);
  return a*b-c*d;
}

static uint32_t FP_CC_FUNC fp_cc_check_fused(void)
{
  // compute: ab-cd with ab==cd and both a and b are odd so ab != RN(ab)
  // without fusing result is zero. both possible fused version RN(ab,-RN(ab)) & RN(RN(ab)-ab)
  // return a non-zero result.

  static const float a = 0x1.6a09e6p-1f;
  static const float b = 0x1.6a09e6p-1f;  // just let b=a as well
  static const float w = 0x1.b3f548p-27f;

  float r = fp_cc_mms(a,b,a,b);

  if (r == 0.f) return 0;

  fprintf(stderr, "\n  error: %s", "compiler fused to FMA");

  // if fused either way as noted above then |r| == w 
  r = fabsf(r);

  // if we don't have 'w' then something else is rotten in Denmark.
  if (r != w)
    fprintf(stderr, "\n         unexpected answer: %a expected %a with fusing", r, w);
  
  return fp_cc_fused;
}


//────────────────────────────────────────────────────────────────────────
// check if transforming constant and/or repeated division(s) into product

static float FP_CC_FUNC fp_cc_div_pi(float a)
{
  fp_cc_retain(a,a);
  return a/0x1.921fb6p1f;
}
static uint32_t FP_CC_FUNC fp_cc_check_reciprocal(void)
{
  // divide 'x' by pi. should get 'c' if transformed we get 'w'
  // RN(x/RN(pi)) = c  & RN(x * RN(1/RN(pi))) = w
  static const float x = 0x1.000012p+0f;
  static const float c = 0x1.45f31ep-2f;
  static const float w = 0x1.45f31cp-2f;

  float r = fp_cc_div_pi(x);

  if (r == c) return 0;

  if (r == w)
    fprintf(stderr, "\n  error: %s", "reciprocal math enabled");
  else
    fprintf(stderr, "\n  error: %s", "reciprocal math unexpected result!!");
  
  return fp_cc_reciprocal;
}


//────────────────────────────────────────────────────────────────────────
// check for honoring infinities

static uint32_t FP_CC_FUNC fp_cc_lt_inf(float a)
{
  static const float inf = (float)(0x1.0p+1000);
  
  fp_cc_retain(a,a);
  
  return (uint32_t)(a < inf);
}

static uint32_t FP_CC_FUNC fp_cc_check_infinites(void)
{
  static const float a = 0x1.fffffep+127f;

  return fp_cc_check_int_zero(fp_cc_lt_inf(a+a),
                              "ignoring infinities",
                              fp_cc_no_honor_inf);                     
}


//────────────────────────────────────────────────────────────────────────
// check for honoring NaNs

static uint32_t FP_CC_FUNC fp_cc_is_ordered(float a)
{
  fp_cc_retain(a,a);

  return (uint32_t)(a == a);
}

static uint32_t FP_CC_FUNC fp_cc_check_nan(void)
{
  float    nan = fp_cc_from_bits(0x7fc00000);
  uint32_t t   = fp_cc_is_ordered(nan);
  
  return fp_cc_check_int_zero(t, "ignoring NaNs", fp_cc_no_honor_nan);
}


//────────────────────────────────────────────────────────────────────────
// check for signed zeroes. if respected then (x+0.f) can't be reduced
// to 'x' since it flushes -0 to +0.

static float FP_CC_FUNC fp_cc_add_zero(float a)
{
  fp_cc_retain(a,a);

  return a+0.0f;
}

static uint32_t FP_CC_FUNC fp_cc_check_signed_zeroes(void)
{
  float    a = fp_cc_from_bits(0x80000000);
  float    r = fp_cc_add_zero(a);
  uint32_t t = fp_cc_to_bits(r);
  
  return fp_cc_check_int_zero(t, "ignoring signed zeroes", fp_cc_no_signed_zero);
}


//────────────────────────────────────────────────────────────────────────
//

// standards are great
#if defined(_MSC_VER)
#pragma float_control(push)
#pragma float_control(precise,on)
#pragma fenv_access(on)
#elif !defined(__GNUC__)
#pragma STDC FENV_ACCESS ON
#endif

#include <fenv.h>

#if defined(FP_CC_EXCEPTIONS) && defined(FP_CC_EXCEPTIONS_RUNTIME)
static float FP_CC_FUNC fp_cc_get_inf(void)
{
  return 1.f/0.f;
}
#endif

// runtime checking seems pointless but have it available if the compile-time
// check isn't working for some compiler.
static uint32_t FP_CC_FUNC fp_cc_check_exceptions(uint32_t e)
{
#if defined(FP_CC_EXCEPTIONS) && defined(FP_CC_EXCEPTIONS_RUNTIME)
  // clear all the sticky bits
  feclearexcept(FE_ALL_EXCEPT);

  // if sticky bits are disabled then the computation 1.f/0.f should
  // reliably be converted into a constant load the so divide-by-zero
  // bit won't be set in that case.
  // (this should be better hardened against removal)
  volatile float r = fp_cc_get_inf();

  (void)r;

  int t = fetestexcept(FE_DIVBYZERO);

#if (FP_CC_EXCEPTIONS == 0)
  if(!t) return e;
  
  fprintf(stderr, "%s%s", "\n  warning: ", "floating point exceptions enabled");
#else
  if(t) return e;
  
  fprintf(stderr, "%s%s", "\n  warning: ", "floating point exceptions disabled");
#endif  
  
  return e+1;
#else
  return e;
#endif  
}

#if defined(_MSC_VER)
#pragma fenv_access(off)
#pragma float_control(pop)
#elif !defined(__GNUC__)
#pragma STDC FENV_ACCESS OFF
#endif


//────────────────────────────────────────────────────────────────────────
// run all of the checks

static void fp_compiler_check(void)
{
  uint32_t errors = 0;

  FP_CC_DUMP("fp_compiler_check: %-50s ", FP_CC_FILE);

  errors |= fp_cc_check_errno();
  errors |= fp_cc_check_daz(); 
  errors |= fp_cc_check_fused();  
  errors |= fp_cc_check_associative();  
  errors |= fp_cc_check_reciprocal();  
  errors |= fp_cc_check_infinites();  
  errors |= fp_cc_check_nan();  
  errors |= fp_cc_check_signed_zeroes();
  errors |= fp_cc_check_exceptions(errors);  
  
  if (errors != 0) {
#if defined(__GNUC__)    
    uint32_t n = (uint32_t)__builtin_popcount(errors);
#elif defined(_MSC_VER)
    uint32_t n = (uint32_t)__popcnt(errors);
#endif
    fprintf(stderr, "\n  float point options violations: %u\n", n);
  }
  else {
    FP_CC_DUMP("  done\n");
  }
}


//────────────────────────────────────────────────────────────────────────
// crowbar calling 'fp_compiler_check' at init time per TU

#if defined(__GNUC__)
// execute the test as late as possible
static void __attribute__((constructor(65535))) fp_compiler_check_root(void) { fp_compiler_check(); }

#elif defined(_MSC_VER)

static void const (* const p_test_init)(void) = fp_compiler_check;

#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) static void (*p_test_init_section)(void) = fp_compiler_check;

#endif

#undef FP_CC_FUNC
