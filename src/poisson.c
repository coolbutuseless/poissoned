
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Random numbers
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define RNGBUFSIZE 1024
typedef struct {
  double buf[RNGBUFSIZE];
  int idx;
} rng_buffer_t;

double runif(rng_buffer_t *rng) {
  
  if (rng->idx < 0 || rng->idx >= RNGBUFSIZE) {
    GetRNGstate();
    for (int i = 0; i < RNGBUFSIZE; i++) {
      rng->buf[i] = unif_rand();
    }
    PutRNGstate();
    
    
    rng->idx = 0;
  }
  
  double num = rng->buf[rng->idx];
  rng->idx++;
  return num;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Points
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  double *x;
  double *y;
  int capacity;
  int idx;
} points_t;


void init_points(points_t *p) {
  p->idx = 0;
  p->capacity = 32;
  p->x = malloc(p->capacity * sizeof(double));
  p->y = malloc(p->capacity * sizeof(double));
  if (p->x == NULL || p->y == NULL) {
    error("Couldn't initialise points");
  }
}

int add_point(points_t *p, double x, double y) {
  
  if (p->idx >= p->capacity) {
    p->capacity *= 2;
    p->x = realloc(p->x, p->capacity * sizeof(double));
    p->y = realloc(p->y, p->capacity * sizeof(double));
    if (p->x == NULL || p->y == NULL) {
      error("Couldn't reallocate points");
    }
  }
  
  p->x[p->idx] = x;
  p->y[p->idx] = y;
  p->idx++;
  
  
  return p->idx - 1;
}

void free_points(points_t *p) {
  if (p == NULL) return;
  
  free(p->x);
  free(p->y);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active list
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int *list;
  int capacity;
  int idx;
} active_t;

void init_active(active_t *active) {
  active->capacity = 1024;
  active->idx = 0;
  active->list = calloc(active->capacity, sizeof(int));
  if (active->list == NULL) {
    error("Couldn't allocate 'active'");
  }
}

void add_active(active_t *active, int point_idx) {
  
  if (active->idx >= active->capacity) {
    active->capacity *= 2;
    active->list = realloc(active->list, active->capacity * sizeof(int));
    if (active->list == NULL) {
      error("Coudln't reallocate active");
    }
  }
  
  active->list[active->idx] = point_idx;
  active->idx++;
}

void free_active(active_t *active) {
  if (active == NULL) return;
  free(active->list);
}


void remove_active(active_t *active, int active_idx) {
  
  if (active_idx >= active->idx) {
    error("Out of bounds");
  }
  
  // Move the last item into this position to be removed
  active->idx--;
  active->list[active_idx] = active->list[active->idx];
}


int random_active(active_t *active, int *active_idx) {
  
  if (active->idx == 0) {
    error("An attempt was made to sample from an empty active list");  
  }
  
  GetRNGstate();
  double rand = unif_rand();
  PutRNGstate();
  
  *active_idx = (int)floor(rand * active->idx);
  return active->list[*active_idx];
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Poisson
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP poisson2d_(SEXP w_, SEXP h_, SEXP r_, SEXP k_) {
  
  int nprotect = 0;
  
  int w     = asInteger(w_);
  int h     = asInteger(h_);
  double r  = asReal(r_);
  // double r2 = r * r;
  // int k     = asInteger(k_);
  double cell_size = r/M_SQRT2;
  
  
  int ncol = (int)ceil(w / cell_size);
  int nrow = (int)ceil(h / cell_size);
  
  Rprintf("cpoisson(): %i x %i  r = %.1f\n", w, h, r);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise an empty grid
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int *grid = malloc(ncol * nrow * sizeof(int));
  if (grid == NULL) error("Malloc failed: grid");
  for (int i = 0; i < ncol * nrow; i++) {
    grid[i] = -1;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise points
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  points_t p = { 0 };
  init_points(&p);
  
  for (int i = 0; i < 10; i++) {
    add_point(&p, (double)i, (double)(i + 1));
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise active list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  active_t active = {0};
  init_active(&active);
  add_active(&active, 0);
  add_active(&active, 1);
  add_active(&active, 2);
  add_active(&active, 3);
  add_active(&active, 4);
  
  for (int i = 0; i < 10; i++) {
    int active_idx = 0;
    int point_idx = random_active(&active, &active_idx);
    Rprintf("Sample: [%i] = %i\n", active_idx, point_idx);
  }  
  
  Rprintf("\n");
  remove_active(&active, 2);
  
  for (int i = 0; i < 10; i++) {
    int active_idx = 0;
    int point_idx = random_active(&active, &active_idx);
    Rprintf("Sample: [%i] = %i\n", active_idx, point_idx);
  }  
  
  Rprintf("\n");
  remove_active(&active, 0);
  
  for (int i = 0; i < 10; i++) {
    int active_idx = 0;
    int point_idx = random_active(&active, &active_idx);
    Rprintf("Sample: [%i] = %i\n", active_idx, point_idx);
  }  
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Copy points to R structure
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(allocVector(VECSXP, 2)); nprotect++;
  SEXP nms_ = PROTECT(allocVector(STRSXP, 2)); nprotect++;
  SET_STRING_ELT(nms_, 0, mkChar("x"));
  SET_STRING_ELT(nms_, 1, mkChar("y"));
  setAttrib(res_, R_NamesSymbol, nms_);
  
  SEXP x_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  SEXP y_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  SET_VECTOR_ELT(res_, 0, x_);
  SET_VECTOR_ELT(res_, 1, y_);
  
  memcpy(REAL(x_), p.x, p.idx * sizeof(double));
  memcpy(REAL(y_), p.y, p.idx * sizeof(double));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  free_points(&p);
  free_active(&active);
  UNPROTECT(nprotect);
  return res_;
}

