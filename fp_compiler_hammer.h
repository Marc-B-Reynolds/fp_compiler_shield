// -*- coding: utf-8 -*-
// Marc B. Reynolds, 2016-2026
// Public Domain under http://unlicense.org, see link for details.
//
// nuclear option: without warning (or mercy) forcefully modify what options we can.
// obviously doesn't protect against later modification in source.
// 
// NOTES:
// ∙ -ffast-math thing of linking with crtfastmath.o which stomps on the control word
//   cannot be handled in this manner (GCC & clang)
// ∙ clang doesn't provide all the tools we need (sadface). see below.
// ∙ This can't be done in a "nice" way when placed in a header including by a "user" which
//   contains inline functions which are effected by FP options. There's no rules for
//   what happens this situation.

#pragma once

// some modifications are redundant: play it safe and easier to eyeball it's covered.

#if defined(__GNUC__)
#if defined(__clang__)
// https://clang.llvm.org/docs/LanguageExtensions.html#extensions-to-specify-floating-point-flags
// 
// cannot cover all the cases with the current pragmas + actual behavior
// vs. -ffast-math on command line:
//   1. supposedly sets: -fno-fast-math -ffp-contract=on -fmath-errno
//      ∙ corrects: associative, reciprocal & signed zeroes ATM
//      ∙ doesn't:  respect infinites &  NaNs
//   2. pragma is ignored if -fp-contract=fast or equivalent on command line.
//      (checks-note) needs -ffp-contract=fast-honor-pragmas (LOL!) to be
//      respected. useless for our purposes here.

// triggering a static_assert when __FAST_MATH__ is defined doesn't catch
// all cases because it become undefined if any of it's suboptions are
// overriden on the command line. example: -ffast-math -fno-reciprocal-math
// will not have __FAST_MATH__ defined.

#if !defined(FP_CC_COMPILE_TIME_DISABLED)
#if defined(__FAST_MATH__)
static_assert(0, "error: -ffast-math is disallowed");
#endif
#endif

#pragma float_control(precise,on)            // see 1
#pragma clang fp reassociate(off)            //   covered by (1)
#pragma clang fp reciprocal(off)             //   covered by (1)
#pragma clang fp contract(off)               // see 2 
#pragma clang fp exceptions(ignore)

// other than control word can't count on fusing disabled,
// respecting inf & nans.

#else
#pragma GCC optimize ("no-fast-math")
#pragma GCC optimize ("no-unsafe-math-optimizations")
// ordering matters for set 'meta' options first (above)
#pragma GCC optimize ("no-math-errno")
#pragma GCC optimize ("fp-contract=off")
#pragma GCC optimize ("no-associative-math")
#pragma GCC optimize ("no-reciprocal-math")
#pragma GCC optimize ("signed-zeros")
#pragma GCC optimize ("no-signaling-nans")
#pragma GCC optimize ("no-trapping-math")

// again: can't handle any modification of the control word (flusing denormals)
#endif

#elif defined(_MSC_VER)
#pragma float_control(precise, on)
#pragma float_control(except, off)
#pragma fp_contract(off)
#endif
