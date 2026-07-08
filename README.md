This is just a stub overview. See the comments in the individual headers for details.

A proof-of-concept level attempt at some low friction protections against "hostile" C/C++ compiler floating-point optimization options.

Say you have a sub-system or library for which you want somewhere between:  *"please don't murder my float point code"* up to *bit-exact* results then the first stumbling blocks (other than non or semi complaint hardware) are any unfortunate compiler option.

Ideally I should be pointing to descriptions of the various options here.

<br>

# `fp_compiler_hammer.h`

This header does the best it can for a given compiler using pragmas to (unconditionally) set floating point options
to the desired set. This example header is fixed for the set of options that IMHO are reasonable.

<br>

# `fp_compiler_check.h`

This header perform some compile-time check that `static_assert` out for fast fails and attempts to perform runtime checks at init time and issues warning to `stderr`.
