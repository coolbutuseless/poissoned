
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate Poisson disk samples in 2D
#'
#' @param w,h width and height of region
#' @param r minimum distance between points
#' @param k number of sample points to generate at each iteration. default 30
#' @param verbosity Verbosity level. default: 0
#'
#' @return data.frame with x and y coordinates. Points are returned in 
#'     the order in which they were generated.
#' @examples
#' pts <- poisson2d(w = 40, h = 40, r = 1)
#' plot(pts, asp = 1, ann = FALSE, axes = FALSE, pch = 19)
#' @importFrom stats runif
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
poisson2d <- function(w = 10, h = 10, r = 2, k = 30L, verbosity = 0L) {
 .Call(poisson2d_, w, h, r, k, verbosity) 
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Generate Poisson disk samples in 3D
#'
#' @param w,h,d width and height and depth of region
#' @param r minimum distance between points
#' @param k number of sample points to generate at each iteration. default 30
#' @param verbosity Verbosity level. default: 0
#'
#' @return data.frame with x, y and z coordinates. Points are returned in 
#'     the order in which they were generated.
#' @examples
#' poisson3d(w = 10, h = 10, d = 10, r = 5)
#' @importFrom stats runif
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
poisson3d <- function(w = 10, h = 10, d = 10, r = 4, k = 30L, verbosity = 0L) {
  .Call(poisson3d_, w, h, d, r, k, verbosity) 
}

