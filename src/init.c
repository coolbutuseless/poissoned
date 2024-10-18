
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

SEXP poisson_(void);

static const R_CallMethodDef CEntries[] = {
  {"poisson_", (DL_FUNC) &poisson_, 1},
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



