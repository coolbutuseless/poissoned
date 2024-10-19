
test_that("basic poisson disk sampling works", {
  
  pts <- poisson2d(w = 10, h = 10, r = 4)
  expect_true(inherits(pts, 'data.frame'))
  expect_identical(colnames(pts), c('x', 'y'))
  
  pts <- poisson3d(w = 10, h = 10, d = 10, r = 5)
  expect_true(inherits(pts, 'data.frame'))
  expect_identical(colnames(pts), c('x', 'y', 'z'))
  
})
