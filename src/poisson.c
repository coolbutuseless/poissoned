
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include "utils.h"


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Point Struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  double *x;
  double *y;
  double *z;
  int capacity;
  int idx;
} points_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Points init
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void init_points(points_t *p) {
  p->idx = 0;
  p->capacity = 32;
  p->x = malloc(p->capacity * sizeof(double));
  p->y = malloc(p->capacity * sizeof(double));
  p->z = malloc(p->capacity * sizeof(double));
  if (p->x == NULL || p->y == NULL || p->z == NULL) {
    error("Couldn't initialise points");
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Points add
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int add_point(points_t *p, double x, double y, double z) {
  
  if (p->idx >= p->capacity) {
    p->capacity *= 2;
    p->x = realloc(p->x, p->capacity * sizeof(double));
    p->y = realloc(p->y, p->capacity * sizeof(double));
    p->z = realloc(p->z, p->capacity * sizeof(double));
    if (p->x == NULL || p->y == NULL || p->z == NULL) {
      error("Couldn't reallocate points");
    }
  }
  
  p->x[p->idx] = x;
  p->y[p->idx] = y;
  p->z[p->idx] = z;
  p->idx++;
  
  return p->idx - 1;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Points free
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void free_points(points_t *p) {
  if (p == NULL) return;
  
  free(p->x);
  free(p->y);
  free(p->z);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active Struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int *list;
  int capacity;
  int idx;
} active_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active init
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void init_active(active_t *active) {
  active->capacity = 1024;
  active->idx = 0;
  active->list = calloc(active->capacity, sizeof(int));
  if (active->list == NULL) {
    error("Couldn't allocate 'active'");
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active free
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void free_active(active_t *active) {
  if (active == NULL) return;
  free(active->list);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active add
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active: remove member
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void remove_active(active_t *active, int active_idx) {
  
  if (active_idx >= active->idx) {
    error("Out of bounds");
  }
  
  // Move the last item into this position to be removed
  active->idx--;
  active->list[active_idx] = active->list[active->idx];
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Active: get random member
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
// Grid struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int *val;
  int nrow;
  int ncol;
  int nplanes;
  double cell_size;
} grid_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Grid: init
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void init_grid(grid_t *grid, int ncol, int nrow, int nplanes, double cell_size) {
  grid->ncol      = ncol;
  grid->nrow      = nrow;
  grid->nplanes   = nplanes;
  grid->cell_size = cell_size;
  grid->val = malloc(ncol * nrow * nplanes * sizeof(int));
  if (grid->val == NULL) {
    error("grid allocation failed");
  }
  for (int i = 0; i < ncol * nrow * nplanes; i++) {
    grid->val[i] = -1;
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Grid: free
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void free_grid(grid_t *grid) {
  if (grid == NULL) return;
  free(grid->val);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Grid: check if point is valid
//   i.e. doesn't already have a point at this grid location
//        is greater than 'r' from all nearby points
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool valid_point(double x, double y, double z, grid_t *grid, points_t *p, double r) {
  
  int col = (int)floor(x / grid->cell_size);
  int row = (int)floor(y / grid->cell_size);
  int pln = (int)floor(z / grid->cell_size);
  
  if (col >= grid->ncol || row >= grid->nrow || pln >= grid->nplanes || col < 0 || row < 0|| pln < 0) {
    error("valid_point invalid [%i, %i] (%.2f, %.2f)", col, row, x, y);
  }
  
  if (grid->val[(grid->ncol * grid->nrow) * pln + row * grid->ncol + col] >= 0) {
    // Already a point here
    return false;
  }
  
  int min_row = MAX(0             , row - 2);
  int max_row = MIN(grid->nrow - 1, row + 2);
  
  int min_col = MAX(0             , col - 2);
  int max_col = MIN(grid->ncol - 1, col + 2);
  
  int min_pln = MAX(0                , pln - 2);
  int max_pln = MIN(grid->nplanes - 1, pln + 2);
  
  for (int this_pln = min_pln; this_pln <= max_pln; this_pln++) {
    for (int this_row = min_row; this_row <= max_row; this_row++) {
      for (int this_col = min_col; this_col <= max_col; this_col++) {
        int point_idx = grid->val[this_pln * (grid->ncol * grid->nrow) + this_row * grid->ncol + this_col];
        if (point_idx >= 0) {
          // There's a point at this grid square
          double this_x = p->x[point_idx];
          double this_y = p->y[point_idx];
          double this_z = p->z[point_idx];
          double dist = 
            (x - this_x) * (x - this_x) + 
            (y - this_y) * (y - this_y) + 
            (z - this_z) * (z - this_z);
          if (dist < r * r) {
            return false;
          }
        }
        
      }
    }
  }
  
  return true;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Grid: add a point
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void set_grid(grid_t *grid, int point_idx, double x, double y, double z) {
  
  int col = (int)floor(x / grid->cell_size);
  int row = (int)floor(y / grid->cell_size);
  int pln = (int)floor(z / grid->cell_size);
  
  if (col >= grid->ncol || row >= grid->nrow || pln >= grid->nplanes || col < 0 || row < 0 || pln < 0) {
    error("set_grid invalid [%i, %i] (%.2f, %.2f)", col, row, x, y);
  }
  
  int grid_idx = pln * grid->ncol * grid->nrow + row * grid->ncol + col;
  if (grid_idx >= grid->ncol * grid->nrow * grid->nplanes) {
    error("OOB: %i x %i =>  %i / %i\n", grid->nrow, grid->ncol, grid_idx, grid->nrow * grid->ncol);
  }
  if (grid->val[grid_idx] >= 0) {
    error("set_grid point already exists: [%i, %i] => %i", row, col, grid->val[grid_idx] );
  }
  
  grid->val[grid_idx] = point_idx;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Poisson in 2d
// @param w,h dimensions of grid
// @param r minimum separation
// @param k points to try 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP poisson2d_(SEXP w_, SEXP h_, SEXP r_, SEXP k_, SEXP verbosity_) {
  
  int nprotect = 0;
  
  int verbosity = asInteger(verbosity_);
  
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
  init_grid(&grid, ncol, nrow, 1, cell_size);
  
  active_t active = {0};
  init_active(&active);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set seed point
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  GetRNGstate();
  double v1 = unif_rand();
  double v2 = unif_rand();
  PutRNGstate();
  double xinit = (double)w/2.0 + v1;
  double yinit = (double)h/2.0 + v2;
  // Rprintf("(%.2f, %.2f) Init [%.2f, %.2f]\n", xinit, yinit, v1, v2);
  
  int point_idx = add_point(&p, xinit, yinit, 0);
  set_grid(&grid, point_idx, xinit, yinit, 0);
  add_active(&active, point_idx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Pick a random active site
  // Generate 'k' random points
  //   for each point
  //      if valid(point)
  //          add point to point list
  //          add point to grid
  //          add point to active list
  //   if no point was valid
  //      remove point from active list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (active.idx > 0) {
    // if (active.idx == 0) break;
    int active_idx = 0;
    point_idx = random_active(&active, &active_idx);
    double x0 = p.x[point_idx];
    double y0 = p.y[point_idx];
    
    if (verbosity > 0) {
      Rprintf("Active [%i]   point [%i] (%.2f, %.2f)\n", active.idx, point_idx, x0, y0);
    }
    
    bool found = false;
    for (int i = 0; i < k; i++) {
      
      // Uniform random sampling on an annulus
      // Random point in annulus [r, 2r] around (x0, y0)
      GetRNGstate();
      double theta = 2 * M_PI * unif_rand(); //runif(&rng);
      double rand = unif_rand();
      PutRNGstate();
      double dist = sqrt(rand * (2*r * 2*r - r*r) + r*r);
      // double x = x0 + (r + 0.00001) * cos( theta );
      // double y = y0 + (r + 0.00001) * sin( theta );
      double x = x0 + dist * cos( theta );
      double y = y0 + dist * sin( theta );
      
      if (x >= w || y >= h || x < 0 || y < 0) continue;
      
      bool valid = valid_point(x, y, 0, &grid, &p, r);
      if (valid) {
        // Rprintf("(%.2f, %.2f) valid\n", x, y);
        int new_point_idx = add_point(&p, x, y, 0);
        add_active(&active, new_point_idx);
        set_grid(&grid, new_point_idx, x, y, 0);
        found = true;
        break;
      }
    }
    
    if (!found) {
      // No valid point was found around this seed point
      // remove it from the Active list 
      // i.e. consider it "done"
      remove_active(&active, active_idx);
    }
  }
  
  
  SEXP x_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  SEXP y_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  memcpy(REAL(x_), p.x, p.idx * sizeof(double));
  memcpy(REAL(y_), p.y, p.idx * sizeof(double));
  SEXP res_ = PROTECT(create_named_list(2, "x", x_, "y", y_)); nprotect++;
  set_df_attributes(res_);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  free_points(&p);
  free_active(&active);
  free_grid(&grid);
  UNPROTECT(nprotect);
  return res_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Poisson in 2d
// @param w,h dimensions of grid
// @param r minimum separation
// @param k points to try 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP poisson3d_(SEXP w_, SEXP h_, SEXP d_, SEXP r_, SEXP k_, SEXP verbosity_) {
  
  int nprotect = 0;
  
  int verbosity = asInteger(verbosity_);
  
  int w     = asInteger(w_);
  int h     = asInteger(h_);
  int d     = asInteger(d_);
  double r  = asReal(r_);
  int k     = asInteger(k_);
  double cell_size = r/sqrt(3);
  
  
  int ncol = (int)ceil(w / cell_size);
  int nrow = (int)ceil(h / cell_size);
  int npln = (int)ceil(d / cell_size);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise 
  //    Points list
  //    Grid structure
  //    Active list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  points_t p = { 0 };
  init_points(&p);
  
  grid_t grid = {0};
  init_grid(&grid, ncol, nrow, npln, cell_size);
  
  active_t active = {0};
  init_active(&active);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set seed point
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  GetRNGstate();
  double v1 = unif_rand();
  double v2 = unif_rand();
  double v3 = unif_rand();
  PutRNGstate();
  double xinit = (double)w/2.0 + v1;
  double yinit = (double)h/2.0 + v2;
  double zinit = (double)d/2.0 + v3;
  // Rprintf("(%.2f, %.2f) Init [%.2f, %.2f]\n", xinit, yinit, v1, v2);
  
  int point_idx = add_point(&p, xinit, yinit, zinit);
  set_grid(&grid, point_idx, xinit, yinit, zinit);
  add_active(&active, point_idx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Pick a random active site
  // Generate 'k' random points
  //   for each point
  //      if valid(point)
  //          add point to point list
  //          add point to grid
  //          add point to active list
  //   if no point was valid
  //      remove point from active list
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (active.idx > 0) {
    // if (active.idx == 0) break;
    int active_idx = 0;
    point_idx = random_active(&active, &active_idx);
    double x0 = p.x[point_idx];
    double y0 = p.y[point_idx];
    double z0 = p.z[point_idx];
    
    if (verbosity > 0) {
      Rprintf("Active [%i]   point [%i] (%.2f, %.2f, %.2f)\n", active.idx, point_idx, x0, y0, z0);
    }
    
    bool found = false;
    for (int i = 0; i < k; i++) {
      
      // Uniform random sampling on an annulus
      // Random point in annulus [r, 2r] around (x0, y0)
      GetRNGstate();
      double x = norm_rand();
      double y = norm_rand();
      double z = norm_rand();
      PutRNGstate();
      double len = sqrt(x*x + y*y + z*z);
      // double x = x0 + (r + 0.00001) * cos( theta );
      // double y = y0 + (r + 0.00001) * sin( theta );
      x = x/len * (r + 0.01) + x0;
      y = y/len * (r + 0.01) + y0;
      z = z/len * (r + 0.01) + z0;
      
      
      if (x >= w || y >= h || z >= d ||  x < 0 || y < 0 || z < 0) continue;
      
      bool valid = valid_point(x, y, z, &grid, &p, r);
      if (valid) {
        // Rprintf("(%.2f, %.2f) valid\n", x, y);
        int new_point_idx = add_point(&p, x, y, z);
        add_active(&active, new_point_idx);
        set_grid(&grid, new_point_idx, x, y, z);
        found = true;
        break;
      }
    }
    
    if (!found) {
      // No valid point was found around this seed point
      // remove it from the Active list 
      // i.e. consider it "done"
      remove_active(&active, active_idx);
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Copy points to R structure
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP x_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  SEXP y_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  SEXP z_ = PROTECT(allocVector(REALSXP, p.idx)); nprotect++;
  memcpy(REAL(x_), p.x, p.idx * sizeof(double));
  memcpy(REAL(y_), p.y, p.idx * sizeof(double));
  memcpy(REAL(z_), p.z, p.idx * sizeof(double));
  SEXP res_ = PROTECT(create_named_list(3, "x", x_, "y", y_, "z", z_)); nprotect++;
  set_df_attributes(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Tidy and return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  free_points(&p);
  free_active(&active);
  free_grid(&grid);
  UNPROTECT(nprotect);
  return res_;
}


