/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * gaussian_blur_region no longer does combination; arguments changed
 *   -- scott@poverty.bloomington.in.us, 16Oct97
 */

#ifndef __PAINT_FUNCS_H__
#define __PAINT_FUNCS_H__

#include "pixel_region.h"
#include "gimpimageF.h"

/*  Called initially to setup accelerated rendering features  */
void  paint_funcs_setup     (void);
void  paint_funcs_free      (void);


/*  Paint functions  */

void  color_pixels          (unsigned char *dest, const unsigned char *color,
			     int w, int bytes);

void  blend_pixels          (const unsigned char *src1,
			     const unsigned char *src2,
			     unsigned char *dest, 
			     int blend, int w, int bytes, int has_alpha);

void  shade_pixels          (const unsigned char *src, unsigned char *dest,
			     const unsigned char *color,
			     int blend, int w, int bytes, int has_alpha);

void  extract_alpha_pixels  (const unsigned char *src,
			     const unsigned char *mask,
			     unsigned char *dest,
			     int w, int bytes);

void  darken_pixels         (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  lighten_pixels        (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  hsv_only_pixels       (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int mode, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  color_only_pixels     (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int mode, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  multiply_pixels       (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  divide_pixels         (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  screen_pixels         (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  overlay_pixels        (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  add_pixels             (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  subtract_pixels       (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  difference_pixels     (const unsigned char *src1,
			     const unsigned char *src2,
	 		     unsigned char *dest, int length,
			     int bytes1, int bytes2,
			     int has_alpha1, int has_alpha2);

void  dissolve_pixels       (const unsigned char *src,
			     unsigned char *dest, int x, int y,
			     int opacity, int length, int sb, int db,
			     int has_alpha);

void  swap_pixels           (unsigned char *src, unsigned char *dest,
			     int length);

void  scale_pixels          (const unsigned char *src, unsigned char *dest,
			     int length, int scale);

void  add_alpha_pixels      (const unsigned char *src, unsigned char *dest,
			     int length, int bytes);

void  flatten_pixels        (const unsigned char *src, unsigned char *dest,
			     const unsigned char *bg, int length, int bytes);

void  gray_to_rgb_pixels    (const unsigned char *src, unsigned char *dest,
			     int length, int bytes);


/*  apply the mask data to the alpha channel of the pixel data  */
void  apply_mask_to_alpha_channel         (unsigned char *src,
					   const unsigned char *mask,
					   int opacity, int length, int bytes);

/*  combine the mask data with the alpha channel of the pixel data  */
void  combine_mask_and_alpha_channel      (unsigned char *src,
					   const unsigned char *mask,
					   int opacity, int length, int bytes);


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void  copy_gray_to_inten_a_pixels         (const unsigned char *src,
					   unsigned char *dest,
					   int length, int bytes);

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels              (const unsigned char *src,
					   unsigned char *dest,
					   int length, int bytes);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_pixels              (const unsigned char *src,
					   unsigned char *dest,
					   const unsigned char *cmap,
					   int length);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_a_pixels            (const unsigned char *src,
					   unsigned char *dest,
					   const unsigned char *mask,
					   const unsigned char *cmap,
					   int opacity, int length);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_pixels                (const unsigned char *src,
					   unsigned char *dest,
					   const unsigned char *mask,
					   int opacity, const int *affect,
					   int length, int bytes);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_a_pixels              (const unsigned char *src,
					   unsigned char *dest,
					   const unsigned char *mask,
					   int opacity, const int *affect,
					   int length, int bytes);

/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void  combine_indexed_and_indexed_pixels  (const unsigned char *src1,
					   const unsigned char *src2,
					   unsigned char *dest,
					   const unsigned char *mask,
					   int opacity, const int *affect,
					   int length, int bytes);

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void  combine_indexed_and_indexed_a_pixels (const unsigned char *src1,
					    const unsigned char *src2,
					    unsigned char       *dest,
					    const unsigned char *mask,
					    int                  opacity,
					    const int           *affect,
					    int                  length,
					    int                  bytes);

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void  combine_indexed_a_and_indexed_a_pixels(const unsigned char *src1,
					     const unsigned char *src2,
					     unsigned char       *dest,
					     const unsigned char *mask,
					     int                  opacity,
					     const int           *affect,
					     int                  length,
					     int                  bytes);

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void  combine_inten_a_and_indexed_a_pixels (const unsigned char *src1,
					    const unsigned char *src2,
					    unsigned char       *dest,
					    const unsigned char *mask,
					    const unsigned char *cmap,
					    int                  opacity,
					    int                  length,
					    int                  bytes);

/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_pixels       (const unsigned char *src1,
					    const unsigned char *src2,
					    unsigned char       *dest,
					    const unsigned char *mask,
					    int                  opacity,
					    const int           *affect,
					    int                  length,
					    int                  bytes);

/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_a_pixels    (const unsigned char *src1,
					   const unsigned char *src2,
					   unsigned char *dest,
					   const unsigned char *mask,
					   int opacity, const int *affect,
					   int length, int bytes);

/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void  combine_inten_a_and_inten_pixels    (const unsigned char *src1,
					   const unsigned char *src2,
					   unsigned char *dest,
					   const unsigned char *mask,
					   int opacity, const int *affect,
					   int mode_affect, int length,
					   int bytes);

/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void  combine_inten_a_and_inten_a_pixels   (const unsigned char *src1,
					    const unsigned char *src2,
					    unsigned char       *dest,
					    const unsigned char *mask,
					    int                 opacity,
					    const int          *affect,
					    int                 mode_affect,
					    int                 length,
					    int                 bytes);

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void  combine_inten_a_and_channel_mask_pixels(const unsigned char *src,
					      const unsigned char *channel,
					      unsigned char       *dest,
					      const unsigned char *col,
					      int                  opacity,
					      int                  length,
					      int                  bytes);

void  combine_inten_a_and_channel_selection_pixels(const unsigned char *src,
						 const unsigned char *channel,
						 unsigned char       *dest,
					         const unsigned char *col,
					         int                  opacity,
					         int                  length,
					         int                  bytes);

/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_inten_pixels                    (const unsigned char *src1,
					      const unsigned char *src2,
					      unsigned char       *dest,
					      const unsigned char *mask,
					      int                  opacity,
					      const int           *affect,
					      int                  length,
					      int                  bytes1,
					      int                  bytes2,
					      int                  has_alpha1,
					      int                  has_alpha2);

/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_indexed_pixels                (const unsigned char *src1,
					    const unsigned char *src2,
					    unsigned char       *dest,
					    const unsigned char *mask,
					    int                  opacity,
					    const int           *affect,
					    int                  length,
					    int                  bytes1,
					    int                  bytes2,
					    int                  has_alpha1,
					    int                  has_alpha2);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void  replace_inten_pixels                (const unsigned char *src1,
					   const unsigned char *src2,
					   unsigned char       *dest,
					   const unsigned char *mask,
					   int                  opacity,
					   const int           *affect,
					   int                  length,
					   int                  bytes1,
					   int                  bytes2,
					   int                  has_alpha1,
					   int                  has_alpha2);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void  replace_indexed_pixels                (const unsigned char *src1,
					     const unsigned char *src2,
					     unsigned char       *dest,
					     const unsigned char *mask,
					     int                  opacity,
					     const int           *affect,
					     int                  length,
					     int                  bytes1,
					     int                  bytes2,
					     int                  has_alpha1,
					     int                  has_alpha2);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void  erase_inten_pixels                  (const unsigned char *src1,
					   const unsigned char *src2,
					   unsigned char       *dest,
					   const unsigned char *mask,
					   int                  opacity,
					   const int           *affect,
					   int                  length,
					   int                  bytes);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */
void  erase_indexed_pixels                (const unsigned char *src1,
					   const unsigned char *src2,
					   unsigned char       *dest,
					   const unsigned char *mask,
					   int                  opacity,
					   const int           *affect,
					   int                  length,
					   int                  bytes);

/*  extract information from intensity pixels based on
 *  a mask.
 */
void  extract_from_inten_pixels           (unsigned char       *src,
					   unsigned char       *dest,
					   const unsigned char *mask,
					   const unsigned char *bg,
					   int                  cut,
					   int                  length,
					   int                  bytes,
					   int                  has_alpha);

/*  extract information from indexed pixels based on
 *  a mask.
 */
void  extract_from_indexed_pixels          (unsigned char *src,
					    unsigned char       *dest,
					    const unsigned char *mask,
					    const unsigned char *cmap,
					    const unsigned char *bg,
					    int                  cut,
					    int                  length,
					    int                  bytes,
					    int                  has_alpha);


/*  variable source to RGB color mapping
 *  src_type == 0  (RGB)
 *  src_type == 1  (GRAY)
 *  src_type == 2  (INDEXED)
 */
void
map_to_color                              (int                  src_type,
					   const unsigned char *cmap,
					   const unsigned char *src,
					   unsigned char       *rgb);


/*  RGB to index mapping functions...
 *
 *  Hash table lookup speeds up the standard
 *  least squares method
 */
int    map_rgb_to_indexed                 (const unsigned char *cmap,
					   int num_cols, GimpImage* gimage,
					   int r, int g, int b);


/*  Region functions  */
void  color_region                        (PixelRegion *dest, 
					   const unsigned char *color);


void  blend_region                        (PixelRegion *, PixelRegion *,
					   PixelRegion *, int);

void  shade_region                        (PixelRegion *, PixelRegion *,
					   unsigned char *, int);

void  copy_region                         (PixelRegion *, PixelRegion *);

void  add_alpha_region                    (PixelRegion *, PixelRegion *);

void  flatten_region                      (PixelRegion *, PixelRegion *,
					   unsigned char *);

void  extract_alpha_region                (PixelRegion *, PixelRegion *,
					   PixelRegion *);

void  extract_from_region                 (PixelRegion *, PixelRegion *,
					   PixelRegion *, unsigned char *,
					   unsigned char *, int, int, int);


/*  The types of convolutions  */
#define NORMAL     0   /*  Negative numbers truncated  */
#define ABSOLUTE   1   /*  Absolute value              */
#define NEGATIVE   2   /*  add 127 to values           */

void  convolve_region                     (PixelRegion *, PixelRegion *,
					   int *, int, int, int);

void  multiply_alpha_region               (PixelRegion *);

void  separate_alpha_region               (PixelRegion *);

void  gaussian_blur_region                (PixelRegion *, double, double);

void  border_region                       (PixelRegion *, gint16, gint16);

void  scale_region                        (PixelRegion *, PixelRegion *);

void  scale_region_no_resample            (PixelRegion *, PixelRegion *);

void  subsample_region                    (PixelRegion *, PixelRegion *,
					   int);

float shapeburst_region                   (PixelRegion *, PixelRegion *);

void thin_region                           (PixelRegion *, gint16 xradius,
					    gint16 yradius, int edge_lock);

void fatten_region                         (PixelRegion *, 
					    gint16 xradius, gint16 yradius);

void  swap_region                         (PixelRegion *, PixelRegion *);


/*  Apply a mask to an image's alpha channel  */
void  apply_mask_to_region                (PixelRegion *, PixelRegion *, int);

/*  Combine a mask with an image's alpha channel  */
void  combine_mask_and_region             (PixelRegion *, PixelRegion *, int);

/*  Copy a gray image to an intensity-alpha region  */
void  copy_gray_to_region                 (PixelRegion *, PixelRegion *);

/*  Lay down the groundwork for layer construction...
 *  This includes background images for indexed or non-alpha
 *  images, floating selections, selective display of intensity
 *  channels, and display of arbitrary mask channels
 */
#define INITIAL_CHANNEL_MASK         0
#define INITIAL_CHANNEL_SELECTION    1
#define INITIAL_INDEXED              2
#define INITIAL_INDEXED_ALPHA        3
#define INITIAL_INTENSITY            4
#define INITIAL_INTENSITY_ALPHA      5

void  initial_region                      (PixelRegion *, PixelRegion *,
					   PixelRegion *, unsigned char *,
					   int, int, int *, int);

/*  Combine two source regions with the help of an optional mask
 *  region into a destination region.  This is used for constructing
 *  layer projections, and for applying image patches to an image
 */
#define COMBINE_INDEXED_INDEXED           0
#define COMBINE_INDEXED_INDEXED_A         1
#define COMBINE_INDEXED_A_INDEXED_A       2
#define COMBINE_INTEN_A_INDEXED_A         3
#define COMBINE_INTEN_A_CHANNEL_MASK      4
#define COMBINE_INTEN_A_CHANNEL_SELECTION 5
#define COMBINE_INTEN_INTEN               6
#define COMBINE_INTEN_INTEN_A             7
#define COMBINE_INTEN_A_INTEN             8
#define COMBINE_INTEN_A_INTEN_A           9

/*  Non-conventional combination modes  */
#define BEHIND_INTEN                      20
#define BEHIND_INDEXED                    21
#define REPLACE_INTEN                     22
#define REPLACE_INDEXED                   23
#define ERASE_INTEN                       24
#define ERASE_INDEXED                     25
#define NO_COMBINATION                    26


void  combine_regions                     (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   unsigned char *, int,
					   int, int *, int);

void  combine_regions_replace             (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   unsigned char *,
					   int, int *, int);


/*  Color conversion routines  */
void  rgb_to_hsv            (int *, int *, int *);
void  hsv_to_rgb            (int *, int *, int *);
void  rgb_to_hls            (int *, int *, int *);
int   rgb_to_l              (int, int, int);
void  hls_to_rgb            (int *, int *, int *);

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

/*  Layer Modes  */
typedef enum {  /*< chop=_MODE >*/
  NORMAL_MODE,
  DISSOLVE_MODE,
  BEHIND_MODE,
  MULTIPLY_MODE,      /*< nick=MULTIPLY/BURN >*/
  SCREEN_MODE,
  OVERLAY_MODE,
  DIFFERENCE_MODE,
  ADDITION_MODE,
  SUBTRACT_MODE,
  DARKEN_ONLY_MODE,
  LIGHTEN_ONLY_MODE,
  HUE_MODE,
  SATURATION_MODE,
  COLOR_MODE,
  VALUE_MODE,
  DIVIDE_MODE,        /*< nick=DIVIDE/DODGE >*/
  ERASE_MODE,
  REPLACE_MODE,
} LayerModeEffects;

/*  Applying layer modes...  */

int   apply_layer_mode         (unsigned char *, unsigned char *,
				unsigned char **, int, int, int,
				int, int, int, int, int, int, int *);

int   apply_indexed_layer_mode (unsigned char *, unsigned char *,
				unsigned char **, int, int, int);

#endif  /*  __PAINT_FUNCS_H__  */
