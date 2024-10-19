
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate Poisson disk samples in 2D
#'
#' @param w,h width and height of region
#' @param r minimum distance between points
#' @param k number of sample points to generate at each iteration. default 30
#' @param verbosity Verbosity level. default: 0
#'
#' @return data.frame with x and y coordinates and the 'idx' order in which
#'         the points were added.
#'
#' @importFrom stats runif
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
poisson2d_r <- function(w = 400, h = 300, r = 10, k = 30L, verbosity = 0L) {
  
  
  width       <- w
  height      <- h
  min_dist    <- r
  min_dist_sq <- min_dist * min_dist
  cell_size   <- min_dist/sqrt(2)
  
  ncols <- ceiling(width  / cell_size)
  nrows <- ceiling(height / cell_size)
  
  if (verbosity > 0) {
    message("poisson2d(): ", width, "x", height, ", minimum distance = ", round(min_dist, 2))
  }
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Over allocate the grid so there are buffer rows around the outside
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  cols <- as.integer(ncols) + 7L
  rows <- as.integer(nrows) + 7L
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Step 0: Set up spatial search acceleration structure - "grid"
  # However, rather than use this grid as an index into points, I'm going to
  # create 2 grids and contain the coordinates for the point within the grid
  # structure.  This avoids a de-referencing step (more speed!).
  #
  # Either gridx or gridy can be used to determine if there's a point at this location.
  # An Inf indicates that there is not anything there.
  #
  # Create a buffer around the entire grid. This will avoid the need for
  # some out-of-bounds checks when searching neighbouring grid points
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gridx <- matrix(Inf, nrow = rows, ncol = cols)
  gridy <- matrix(Inf, nrow = rows, ncol = cols)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Keep track of point info
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  N  <- 0L
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Function to quantize a coordinate into grid scale
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gridify <- function(coord) {
    as.integer(coord/cell_size) + 4L
  }
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Initialise the grid with a seed point. The user may have supplied
  # their own seed points!
  # If we are generating a seed automatically, enforce it to be near the
  # centre to avoid some edge cases that aren't handled here (because this
  # first point is not (currently) mirrored for the boundary conditions)
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  x <- runif(1, 0.45 * width , 0.55 * width )
  y <- runif(1, 0.45 * height, 0.55 * height)
  
  xg <- gridify(x)
  yg <- gridify(y)
  
  gridx[cbind(yg, xg)] <- x
  gridy[cbind(yg, xg)] <- y
  N                    <- length(x)
  
  
  gidx <- (xg - 1L) * rows + yg
  actlist <- as.list(gidx)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Offsets to get all neighbour grid coordinates
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  xoff <- c(0L, 0L,  0L,   1L, 1L,  1L,   -1L, -1L, -1L)
  yoff <- c(0L, 1L, -1L,   0L, 1L, -1L,    0L,  1L, -1L)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # While there are points in the active list
  #  - pick a random active point
  #  - generate k candidate points a distance between [r, 2r] from this point
  #  - for each candidate point, see if it there are any existing points within 'min_dist'
  #     - if all candidate points have close neighbours then delete the selected
  #       random active point from the active list
  #     - otherwise, pick one of the candidate points with no close neighbours
  #       and add it to the active list
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while (length(actlist) > 0) {
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Pick a random grid index from the active list
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    actidx <- sample(length(actlist), 1L)
    idx    <- actlist[[actidx]]
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Extract the coords at this idx
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    x <- gridx[idx]
    y <- gridy[idx]
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # generate k points in the spherical annulus
    # between r and 2r around this point
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    mags <- runif(k, min_dist, 2 * min_dist)
    angs <- runif(k,        0, 2 * pi      )
    
    x    <- x + mags * cos(angs)
    y    <- y + mags * sin(angs)
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Note any candidate points outside the limits of the canvas.
    # These are not removed at this stage to try and keep some vector
    # handling consistently sized. Not sure if this makes a difference.
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    tvalid <- x > 0 & x < width & y > 0 & y < height
    
    xx <- rep(x, each = 9L)
    yy <- rep(y, each = 9L)
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # What are the grid locations of these candidate points?
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    xg   <- gridify(x)
    yg   <- gridify(y)
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # grid coordinates of all candidate points and all their neighbours
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    xgs <- rep(xg, each = 9L) + xoff
    ygs <- rep(yg, each = 9L) + yoff
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # The grid indices
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    idxs <- (xgs - 1L) * rows + ygs
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Get all the coordinates at these indices. If there's nothing there, then
    # the coordinates will be Inf
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    xc <- gridx[idxs]
    yc <- gridy[idxs]
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # For candidate point, determine if the distance to each neighbour is
    # greater than the minimum distances.  Operator in squared distances to
    # avoid the sqrt() operation.
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bool_sq <- ((xc - xx)^2L + (yc - yy)^2L) > min_dist_sq
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # For each of the coordinates which are valid, check its grid location,
    # and its 8 surrounding neighbours.  If all of them have a distance
    # greater than the minimum distance, then we can locate a point here
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    best_idx <- NA
    for (i in which(tvalid)) {
      res <- sum(bool_sq[1:9 + (i-1L) * 9L])
      if (res == 9L) {
        best_idx <- i
        break
      }
    }
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # If we don't have a best grid idx, it means that none of
    # the candidates was above the minimum distance from all its neighbours
    # The point at this grid index should be removed from the active list.
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (is.na(best_idx)) {
      actlist[[actidx]] <- NULL
      next
    }
    
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Otherwise, insert the candidate point in the grid and add it to the
    # active list.
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    xb <- x[best_idx]
    yb <- y[best_idx]
    xg <- gridify(xb)
    yg <- gridify(yb)
    
    gidx <- (xg - 1L) * rows + yg
    gridx [gidx] <- xb
    gridy [gidx] <- yb
    N            <- N + 1L
    
    actlist[[length(actlist) + 1L]] <- gidx
  } # End of while() loop
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Extract the data to return to the user
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  within_canvas <- gridx > 0 & gridx < width & gridy > 0 & gridy < height
  
  valid <- is.finite(gridx) & within_canvas
  
  res <- data.frame(
    x = gridx[valid],
    y = gridy[valid]
  )
  
  
  res
}

