
<!-- README.md is generated from README.Rmd. Please edit that file -->

# poissoned

<!-- badges: start -->

<!-- badges: end -->

`poissoned` is an Rstats implementation of the poisson disk sampling
algorithm from [Bridson’s paper - Fast Poisson Disk Sampling in
Arbitrary
Dimensions](https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf)

Notes:

  - only implemented the 2d case
  - adjusted the algorithm in order to ensure repeatable/tileable
    samples with minimal artifacts at the joins
  - similar in purpose to [Will Chase’s](https://twitter.com/W_R_Chase)
    package
    [{poissondisc}](https://github.com/will-r-chase/poissondisc).
    However, `poissoned` has a a completely different implementation
    which ends up being much faster.

## Installation

You can install from
[GitHub](https://github.com/coolbutuseless/poissoned) with:

``` r
# install.packages("devtools")
devtools::install_github("coolbutuseless/poissoned")
```

## Basic Usage

``` r
library(poissoned)

set.seed(1)
points <- poissoned::poisson_disc(ncols = 50, nrows = 35, cell_size = 10, verbose = TRUE)
#> poisson_disc(): 500x350, minimum distance = 14.14

ggplot(points) +
  geom_point(aes(x, y)) +
  theme_bw() +
  coord_fixed() +
  theme(
    panel.grid = element_blank(),
    axis.title = element_blank()
  ) 
```

<img src="man/figures/README-example-1.png" width="100%" />

# Tileability

The points generated from `poissoned::poisson_disc()` are tileable in
that there should be minimal artifacts at the joins.

In the graph below, the initial set of points (highlighted in blue) were
manually replicated and offset eight times.

<img src="man/figures/README-unnamed-chunk-2-1.png" width="100%" />
