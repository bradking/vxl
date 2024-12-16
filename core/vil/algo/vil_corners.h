// This is core/vil/algo/vil_corners.h
#ifndef vil_corners_h_
#define vil_corners_h_
//:
// \file
// \brief Estimate corner positions using Forstner/Harris approach
// \author Tim Cootes

#include <vil/vil_image_view.h>
#include <vil/algo/vil_sobel_3x3.h>

//: Compute Harris corner strength function given gradient images.
//  grad_i and grad_j are assumed to be the i and j gradient images (single
//  plane), such as produced by vil_sobel_3x3().  At each pixel compute
//  the Harris corner function: det(H)-k*sqr(trace(H)), where
//  H is the 2x2 matrix of second derivatives, generated by applying a Sobel
//  operator to the gradient images.
//
//  The local peaks of the output image correspond to corner candidates.
// \relatesalso vil_image_view
void
vil_corners(const vil_image_view<float> & grad_i,
            const vil_image_view<float> & grad_j,
            vil_image_view<float> & dest,
            double k = 0.04);

void
vil_corners(const vil_image_view<double> & grad_i,
            const vil_image_view<double> & grad_j,
            vil_image_view<double> & dest,
            double k = 0.04);

//: Compute corner strength using Rohr's recommended method
//  This computes the determinant of the matrix C=g.g'
//  after the elements of C have been smoothed.
//  g is the vector of first derivatives (gx,gy)'
//  It relies only on first derivatives.
// \relatesalso vil_image_view
void
vil_corners_rohr(const vil_image_view<float> & grad_i,
                 const vil_image_view<float> & grad_j,
                 vil_image_view<float> & dest);

//: Compute Harris corner strength function
//  At each pixel compute
//  the Harris corner function: det(H)-k*sqr(trace(H)), where
//  H is the 2x2 matrix of second derivatives, generated by applying a Sobel
//  operator twice.  The filters thus effectively have 5x5 support.
//
//  The local peaks of the output image correspond to corner candidates.
// \relatesalso vil_image_view
template <class T>
inline void
vil_corners(const vil_image_view<T> & src, vil_image_view<float> & dest, double k = 0.04)
{
  vil_image_view<float> grad_i, grad_j;
  vil_sobel_3x3(src, grad_i, grad_j);
  vil_corners(grad_i, grad_j, dest, k);
}

//: Compute corner strength using Karl Rohr's recommended method
//  This computes the determinant of the matrix C=g.g'
//  after the elements of C have been smoothed.
//  g is the vector of first derivatives (gx,gy)'
//  It relies only on first derivatives.
// \relatesalso vil_image_view
template <class T>
inline void
vil_corners_rohr(const vil_image_view<T> & src, vil_image_view<float> & dest)
{
  vil_image_view<float> grad_i, grad_j;
  vil_sobel_3x3(src, grad_i, grad_j);
  vil_corners_rohr(grad_i, grad_j, dest);
}


#endif // vil_corners_h_
