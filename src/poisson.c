
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
// Grid handling
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int *val;
  int nrow;
  int ncol;
  double cell_size;
} grid_t;


void init_grid(grid_t *grid, int ncol, int nrow, double cell_size) {
  grid->ncol = ncol;
  grid->nrow = nrow;
  grid->cell_size = cell_size;
  grid->val = malloc(ncol * nrow * sizeof(int));
  if (grid->val == NULL) {
    error("grid allocation failed");
  }
  for (int i = 0; i < ncol * nrow; i++) {
    grid->val[i] = -1;
  }
}

void free_grid(grid_t *grid) {
  if (grid == NULL) return;
  free(grid->val);
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))



bool valid_point(double x, double y, grid_t *grid, points_t *p, double r) {
  
  int col = (int)floor(x / grid->cell_size);
  int row = (int)floor(y / grid->cell_size);
  
  if (col >= grid->ncol || row >= grid->nrow) {
    error("valid_point invalid [%i, %i] (%.2f, %.2f)", col, row, x, y);
  }
  
  if (grid->val[row * grid->ncol + col] >= 0) {
    // Already a point here
    return false;
  }
  
  int min_row = MAX(0             , row - 1);
  int max_row = MIN(grid->nrow - 1, row + 1);
  
  int min_col = MAX(0             , col - 1);
  int max_col = MIN(grid->ncol - 1, col + 1);
  
  for (int this_row = min_row; this_row <= max_row; this_row++) {
    for (int this_col = min_col; this_col <= max_col; this_col++) {
      int point_idx = grid->val[this_row * grid->ncol + this_col];
      if (point_idx >= 0) {
        // There's a point at this grid square
        double this_x = p->x[point_idx];
        double this_y = p->y[point_idx];
        double dist = (x - this_x) * (x - this_x) + (y - this_y) * (y - this_y);
        if (dist < r *r) {
          return false;
        }
      }
      
      
    }
  }
  
  
  return true;
}


void set_grid(grid_t *grid, int point_idx, double x, double y) {
  
  if (x >= 400 || y >= 300) {
    error("WTF: (%.2f, %.2f)", x, y);
  }
  
  int col = (int)floor(x / grid->cell_size);
  int row = (int)floor(y / grid->cell_size);
  
  if (col >= grid->ncol || row >= grid->nrow) {
    error("set_grid invalid [%i, %i] (%.2f, %.2f)", col, row, x, y);
  }
  
  int grid_idx = row * grid->ncol + col;
  if (grid_idx >= grid->ncol * grid->nrow) {
    error("OOB: %i x %i =>  %i / %i\n", grid->nrow, grid->ncol, grid_idx, grid->nrow * grid->ncol);
  }
  if (grid->val[grid_idx] >= 0) {
    error("set_grid point already exists: [%i, %i] => %i", row, col, grid->val[grid_idx] );
  }
  
  grid->val[grid_idx] = point_idx;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Poisson
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP poisson2d_(SEXP w_, SEXP h_, SEXP r_, SEXP k_) {
  
  int nprotect = 0;
  
  int w     = asInteger(w_);
  int h     = asInteger(h_);
  double r  = asReal(r_);
  int k     = asInteger(k_);
  double cell_size = r/M_SQRT2;
  
  
  int ncol = (int)ceil(w / cell_size);
  int nrow = (int)ceil(h / cell_size);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise 
  //    Points list
  //    Grid structure
  //    Active list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  points_t p = { 0 };
  init_points(&p);
  
  grid_t grid = {0};
  init_grid(&grid, ncol, nrow, cell_size);
  
  active_t active = {0};
  init_active(&active);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set seed point
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rng_buffer_t rng;
  
  double xinit = w/2 + runif(&rng);
  double yinit = h/2 + runif(&rng);
  // Rprintf("(%.2f, %.2f) Init\n", xinit, yinit);
  
  int point_idx = add_point(&p, xinit, yinit);
  set_grid(&grid, point_idx, xinit, yinit);
  add_active(&active, point_idx);
  
  // Pick a random active site
  // Generate 'k' random points
  //   for each point
  //      if valid(point)
  //          add point to point list
  //          add point to grid
  //          add point to active list
  //   if no point was valid
  //      remove point from active list
  
  for (int ll = 0; ll < 2800; ll++) {
    if (active.idx == 0) break;
    int active_idx = 0;
    point_idx = random_active(&active, &active_idx);
    double x0 = p.x[point_idx];
    double y0 = p.y[point_idx];
    // Rprintf("Active [%i]   point [%i] (%.2f, %.2f)\n", active.idx, point_idx, x0, y0);
    
    bool found = false;
    for (int i = 0; i < k; i++) {
      double theta = 2 * M_PI * runif(&rng);
      double x = x0 + (r + 0.00001) * cos( theta );
      double y = y0 + (r + 0.00001) * sin( theta );
      
      if (x >= w || y >= h || x < 0 || y < 0) continue;
      
      bool valid = valid_point(x, y, &grid, &p, r);
      if (valid) {
        // Rprintf("(%.2f, %.2f) valid\n", x, y);
        int new_point_idx = add_point(&p, x, y);
        add_active(&active, new_point_idx);
        set_grid(&grid, new_point_idx, x, y);
        found = true;
        break;
      }
    }
    
    if (!found) {
      remove_active(&active, active_idx);
    }
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
  free_grid(&grid);
  UNPROTECT(nprotect);
  return res_;
}

