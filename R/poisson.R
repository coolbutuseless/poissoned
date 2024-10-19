
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate poisson disc samples
#'
#' @param w,h width and height of region
#' @param r minimum distance between points
#' @param k number of sample points to generate at each iteration. default 30
#' @param verbosity Verbosity level. default: 0
#'
#' @return data.frame with x and y coordinates and the 'idx' order in which
#'         the points were added.
#' @examples
#' pts <- poisson2d(w = 10, h = 10, r = 2)
#' plot(pts)
#' @importFrom stats runif
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
poisson2d <- function(w = 400, h = 300, r = 10, k = 30L, verbosity = 0L) {
 .Call(poisson2d_, w, h, r, k, verbosity) 
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate poisson disc samples in 3D
#'
#' @param w,h,d width and height and depth of region
#' @param r minimum distance between points
#' @param k number of sample points to generate at each iteration. default 30
#' @param verbosity Verbosity level. default: 0
#'
#' @return data.frame with x and y coordinates and the 'idx' order in which
#'         the points were added.
#' @examples
#' poisson3d(w = 10, h = 10, d = 10, r = 5)
#' 
#' @importFrom stats runif
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
poisson3d <- function(w = 400, h = 300, d = 100, r = 10, k = 30L, verbosity = 0L) {
  .Call(poisson3d_, w, h, d, r, k, verbosity) 
}


if (FALSE) {
  
  pts <- poisson3d(10, 10, 10, 1)
  cols <- rainbow(length(pts$x))
  rgl::points3d(pts, color = cols)
  rgl::box3d()
  
}

