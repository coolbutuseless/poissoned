# poissoned 0.1.3.9000

* Fix DOI
* Fix referencing to be be "Author (Year)" in DESCRIPTION
* Add package doc
* Bug fix for initialization points for small canvases
* Add instructions to install from CRAN

# poissoned 0.1.3  2024-10-19

* Rewrite in C
* Include 3D version

# poissoned 0.1.2

* Random initialisation point is now confined to the central area to avoid 
  some edge cases.  This fixes some tiling artefacts.
* Fixed up an error in how corner points were being mirrored as boundary points.
  This fixes some tiling artefacts.

# poissoned 0.1.1

* Added arguments `xinit`, `yinit` to allow the user to specify the initial seed point
* Added argument `keep_idx` to allow the user to keep the order in which the points
were generated.

# poissoned 0.1.0

* Initial release
