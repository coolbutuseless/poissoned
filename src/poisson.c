
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#define TPH_POISSON_IMPLEMENTATION
#include "tph_poisson.h"

SEXP poisson_(SEXP w_, SEXP h_, SEXP r_, SEXP k_, SEXP seed_) {
  
  int nprotect = 0;
  
  double w = asReal(w_);
  double h = asReal(h_);
  double r = asReal(r_);
  if (w <= 0 || h <= 0 || r <= 0) {
    error("'w', 'h' and 'r' should all be positive");
  }
  
  int k = asInteger(k_);
  if (k < 0) {
    error("'k' should not be negative");
  }
  uint64_t seed = (uint64_t)asInteger(seed_);
  
  
  
  // Configure arguments. 
  const tph_poisson_real bounds_min[2] = { w , h };
  const tph_poisson_real bounds_max[2] = {  w*2 ,  h*2 };
  
  const tph_poisson_args args = { 
    .bounds_min          = bounds_min,
    .bounds_max          = bounds_max,
    .radius              = r,
    .ndims               = INT32_C(2),
    .max_sample_attempts = (uint32_t)k,
    .seed                = seed 
  };
  
  
  
  // Using default allocator (libc malloc).
  const tph_poisson_allocator *alloc = NULL;
  
  // Create samples. 
  tph_poisson_sampling sampling = { 
    .internal = NULL, 
    .ndims    = INT32_C(0), 
    .nsamples = 0 
  };
  
  const int ret = tph_poisson_create(&args, alloc, &sampling);
  if (ret != TPH_POISSON_SUCCESS) {
    // No need to destroy sampling here! 
    error("Failed creating Poisson sampling! Error code: %d\n", ret);
  }
  
  // Retrieve samples.
  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  assert(samples != NULL);
  
  int n = sampling.nsamples / 2;
  
  // Create return object
  SEXP res_ = PROTECT(allocVector(VECSXP, 2)); nprotect++;
  SEXP nms_ = PROTECT(allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(nms_, 0, mkChar("x"));
  SET_STRING_ELT(nms_, 1, mkChar("y"));
  SEXP x_ = PROTECT(allocVector(REALSXP, n)); nprotect++;
  SEXP y_ = PROTECT(allocVector(REALSXP, n)); nprotect++;
  SET_VECTOR_ELT(res_, 0, x_);
  SET_VECTOR_ELT(res_, 1, y_);
  setAttrib(res_, R_NamesSymbol, nms_);
  
  double *x = REAL(x_);
  double *y = REAL(y_);
  
  for (size_t i = 0; i < n; i++) {
    x[i] = *samples++;
    y[i] = *samples++;
  }
  
  // Tidy and return 
  tph_poisson_destroy(&sampling);
  UNPROTECT(nprotect);
  return res_;
}

