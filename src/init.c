
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

SEXP poisson2d_(SEXP w_, SEXP h_, SEXP r_, SEXP k_);
SEXP poisson3d_(SEXP w_, SEXP h_, SEXP d_, SEXP r_, SEXP k_);

static const R_CallMethodDef CEntries[] = {
  {"poisson2d_", (DL_FUNC) &poisson2d_, 4},
  {"poisson3d_", (DL_FUNC) &poisson3d_, 5},
  {NULL , NULL, 0}
};


void R_init_poissoned(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}



