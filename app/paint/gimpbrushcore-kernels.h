/* gimpbrushcore-kernels.h
 *
 *   This file was generated using kernelgen as found in the tools dir.
 *   (threshold = 0.25)
 */

#ifndef __GIMP_BRUSH_CORE_KERNELS_H__
#define __GIMP_BRUSH_CORE_KERNELS_H__


#define KERNEL_WIDTH     3
#define KERNEL_HEIGHT    3
#define KERNEL_SUBSAMPLE 4


#ifdef __cplusplus

template <class T>
struct Kernel;

template <>
struct Kernel<guchar>
{
  using value_type  = guchar;
  using kernel_type = guint;
  using accum_type  = gulong;

  static constexpr kernel_type
  coeff (kernel_type x)
  {
    return x;
  }

  static constexpr value_type
  round (accum_type x)
  {
    return (x + 128) / 256;
  }
};

template <>
struct Kernel<gfloat>
{
  using value_type  = gfloat;
  using kernel_type = gfloat;
  using accum_type  = gfloat;

  static constexpr kernel_type
  coeff (kernel_type x)
  {
    return x / 256.0f;
  }

  static constexpr value_type
  round (accum_type x)
  {
    return x;
  }
};


/*  Brush pixel subsampling kernels  */
template <class T>
struct Subsample : Kernel<T>
{
  #define C(x) (Subsample::coeff (x))

  static constexpr typename Subsample::kernel_type kernel[5][5][9] =
  {
    {
      { C( 64), C( 64), C(  0), C( 64), C( 64), C(  0), C(  0), C(  0), C(  0), },
      { C( 25), C(103), C(  0), C( 25), C(103), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C(128), C(  0), C(  0), C(128), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C(103), C( 25), C(  0), C(103), C( 25), C(  0), C(  0), C(  0), },
      { C(  0), C( 64), C( 64), C(  0), C( 64), C( 64), C(  0), C(  0), C(  0), }
    },
    {
      { C( 25), C( 25), C(  0), C(103), C(103), C(  0), C(  0), C(  0), C(  0), },
      { C(  6), C( 44), C(  0), C( 44), C(162), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C( 50), C(  0), C(  0), C(206), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C( 44), C(  6), C(  0), C(162), C( 44), C(  0), C(  0), C(  0), },
      { C(  0), C( 25), C( 25), C(  0), C(103), C(103), C(  0), C(  0), C(  0), }
    },
    {
      { C(  0), C(  0), C(  0), C(128), C(128), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C(  0), C(  0), C( 50), C(206), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(256), C(  0), C(  0), C(  0), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(206), C( 50), C(  0), C(  0), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(128), C(128), C(  0), C(  0), C(  0), }
    },
    {
      { C(  0), C(  0), C(  0), C(103), C(103), C(  0), C( 25), C( 25), C(  0), },
      { C(  0), C(  0), C(  0), C( 44), C(162), C(  0), C(  6), C( 44), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(206), C(  0), C(  0), C( 50), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(162), C( 44), C(  0), C( 44), C(  6), },
      { C(  0), C(  0), C(  0), C(  0), C(103), C(103), C(  0), C( 25), C( 25), }
    },
    {
      { C(  0), C(  0), C(  0), C( 64), C( 64), C(  0), C( 64), C( 64), C(  0), },
      { C(  0), C(  0), C(  0), C( 25), C(103), C(  0), C( 25), C(103), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(128), C(  0), C(  0), C(128), C(  0), },
      { C(  0), C(  0), C(  0), C(  0), C(103), C( 25), C(  0), C(103), C( 25), },
      { C(  0), C(  0), C(  0), C(  0), C( 64), C( 64), C(  0), C( 64), C( 64), }
    }
  };

  #undef C
};

template <class T>
constexpr typename Subsample<T>::kernel_type Subsample<T>::kernel[5][5][9];

#endif /* __cplusplus */


#endif /* __GIMP_BRUSH_CORE_KERNELS_H__ */
