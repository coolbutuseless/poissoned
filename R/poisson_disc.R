

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate tileable poisson disc samples
#'
#' @param ncols,nrows number of cells in x and y direction. Resulting point coordinate
#'        ranges will be x = [0, ncols*cell_size] and y = [0, nrows*cell_size]
#' @param cell_size length of side of an individual cell
#' @param k number of sample points to generate at each iteration. default 30
#' @param tileable enforce boundary conditions to make point set tileable. default: TRUE
#' @param keep_boundary keep the boundary condition points used for enforcing
#'        tileability. This is probably only useful for debugging.  default: FALSE
#' @param verbose default: FALSE
#'
#' @return data.frame with x and y coordinates
#'
#' @importFrom stats runif
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
poisson_disc <- function(ncols = 20L, nrows = 20L, cell_size = 10, k = 30L,
                         tileable = TRUE, keep_boundary = FALSE, verbose = FALSE) {

  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Output canvas size
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  width    <- cell_size * ncols
  height   <- cell_size * nrows


  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Use the squared distance as comparison to save on doing a sqrt() later
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  min_dist    <- cell_size * sqrt(2)
  min_dist_sq <- min_dist * min_dist

  if (verbose) {
    message("poisson_disc(): ", width, "x", height, ", minimum distance = ", round(min_dist, 2))
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
  # Keep note of where the border edge is (excluding the buffer) so that
  # we know when to replicate a point to outside the  opposite edge in order
  # to fake a toroidal coord system so we get a tileable set of poisson points
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  min_grid_x <- 4
  min_grid_y <- 4
  max_grid_x <- cols - 4
  max_grid_y <- rows - 4


  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Function to quantize a coordinate into grid scale
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  gridify <- function(coord) {
    as.integer(coord/cell_size) + 4L
  }


  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Initialise the grid with a seed point
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  x  <- runif(1, 0, width )
  y  <- runif(1, 0, height)

  xg <- gridify(x)
  yg <- gridify(y)

  gridx[yg, xg] <- x
  gridy[yg, xg] <- y

  actlist <- list()
  gidx <- (xg - 1L) * rows + yg
  actlist[[1]] <- gidx


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
    # between r and 2r aroudn this points
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
    # What are the grid locations of these candiate points?
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
    # the coordiates will be Inf
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
    # and it's 8 surrounding neighbours.  If all of them have a distance
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

    actlist[[length(actlist) + 1L]] <- gidx


    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Do extra work to make points tileable
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (tileable) {
      #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      # If the new point lies along the border of the grid
      # copy it to just outside to the opposite edge in order
      # to simulate toroidal-ness and make the resulting points tileable
      #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      if (xg == min_grid_x) {
        xt   <- xb + width
        yt   <- yb
        xgt  <- max_grid_x + 1L
        ygt  <- yg
        gidx <- (xgt - 1L) * rows + ygt
        gridx[gidx] <- xt
        gridy[gidx] <- yt
      } else if (xg == max_grid_x) {
        xt   <- xb - width
        yt   <- yb
        xgt  <- min_grid_x - 1L
        ygt  <- yg
        gidx <- (xgt - 1L) * rows + ygt
        gridx[gidx] <- xt
        gridy[gidx] <- yt
      }

      if (yg == min_grid_y) {
        xt   <- xb
        yt   <- yb + height
        xgt  <- xg
        ygt  <- max_grid_y + 1L
        gidx <- (xgt - 1L) * rows + ygt
        gridx[gidx] <- xt
        gridy[gidx] <- yt
      } else if (yg == max_grid_y) {
        xt  <- xb
        yt  <- yb - height
        xgt <- xg
        ygt <- min_grid_y - 1L
        gidx <- (xgt - 1L) * rows + ygt
        gridx[gidx] <- xt
        gridy[gidx] <- yt
      }


      #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      # Corner logic is a bit more complex. Only copy the first corner
      # point to all 4 outer corners.  So always check if there is
      # already something in the outer corners before proceeding
      #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      if ((xg == min_grid_x || xg == max_grid_x) &&
          (yg == min_grid_y || yg == max_grid_y)) {

        gidx <- (min_grid_x - 1L - 1L) * rows + (min_grid_y - 1L - 1L)

        if (is.infinite(gridx[gidx])) {
          xt <- xb %% cell_size
          yt <- yb %% cell_size

          gidx <- (min_grid_x - 1L - 1L) * rows + (min_grid_y - 1L - 1L)
          gridx[gidx] <- xt - cell_size
          gridy[gidx] <- yt - cell_size

          gidx <- (min_grid_x - 1L - 1L) * rows + (max_grid_y + 1L - 1L)
          gridx[gidx] <- xt - cell_size
          gridy[gidx] <- yt + height

          gidx <- (max_grid_x + 1L - 1L) * rows + (max_grid_y + 1L - 1L)
          gridx[gidx] <- xt + width
          gridy[gidx] <- yt + height

          gidx <- (max_grid_x + 1L - 1L) * rows + (min_grid_y - 1L - 1L)
          gridx[gidx] <- xt + width
          gridy[gidx] <- yt - cell_size
        }
      }
    } # End of if(tileable)

  } # End of while() loop


  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # extract just the points that aren't infinite and exclude any shadow points
  # outside of (0,0) to (width,height)
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (keep_boundary) {
    valid <- is.finite(gridx)
  } else {
    valid <- is.finite(gridx) & gridx > 0 & gridx < width & gridy > 0 & gridy < height
  }

  data.frame(
    x = gridx[valid],
    y = gridy[valid]
  )
}

