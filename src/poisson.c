
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

SEXP poisson_(void) {
  /* Configure arguments. */
  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-10, (tph_poisson_real)-10 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)10, (tph_poisson_real)10 };
  const tph_poisson_args args = { 
    .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)3,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };
  /* Using default allocator (libc malloc). */
  const tph_poisson_allocator *alloc = NULL;
  
  /* Create samples. */
  tph_poisson_sampling sampling = { .internal = NULL, .ndims = INT32_C(0), .nsamples = 0 };
  const int ret = tph_poisson_create(&args, alloc, &sampling);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy sampling here! */
    error("Failed creating Poisson sampling! Error code: %d\n", ret);
  }
  
  /* Retrieve samples. */
  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  assert(samples != NULL);
  
  /* Print first and last sample positions. */
  if (0) {
    Rprintf("samples[%td] = ( %.3f, %.3f )\n", 
            (ptrdiff_t)0, 
            (double)samples[0], 
                           (double)samples[1]);
    Rprintf("...\n");
    Rprintf("samples[%td] = ( %.3f, %.3f )\n",
            sampling.nsamples - 1,
            (double)samples[(sampling.nsamples - 1) * sampling.ndims],
                           (double)samples[(sampling.nsamples - 1) * sampling.ndims + 1]);
  }
  
  /* Free memory. */
  tph_poisson_destroy(&sampling);
  
  return R_NilValue;
}

