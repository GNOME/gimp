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

void  color_pixels          (unsigned char *, unsigned char *,
			     int, int);

void  blend_pixels          (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int);

void  shade_pixels          (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int);

void  extract_alpha_pixels  (unsigned char *, unsigned char *,
			     unsigned char *, int, int);

void  darken_pixels         (unsigned char *, unsigned char *,
	 		     unsigned char *, int, int, int, int, int);

void  lighten_pixels        (unsigned char *, unsigned char *,
	 		     unsigned char *, int, int, int, int, int);

void  hsv_only_pixels       (unsigned char *, unsigned char *,
	 		     unsigned char *, int, int, int, int, int, int);

void  color_only_pixels     (unsigned char *, unsigned char *,
	 		     unsigned char *, int, int, int, int, int, int);

void  multiply_pixels       (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  divide_pixels         (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  screen_pixels         (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  overlay_pixels        (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  add_pixels            (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  subtract_pixels       (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  difference_pixels     (unsigned char *, unsigned char *,
			     unsigned char *, int, int, int, int, int);

void  dissolve_pixels       (unsigned char *, unsigned char *, int, int,
			     int, int, int, int, int);

void  swap_pixels           (unsigned char *, unsigned char *, int);

void  scale_pixels          (unsigned char *, unsigned char *,
			     int, int);

void  add_alpha_pixels      (unsigned char *, unsigned char *,
			     int, int);

void  flatten_pixels        (unsigned char *, unsigned char *,
			     unsigned char *, int, int);

void  gray_to_rgb_pixels    (unsigned char *, unsigned char *,
			     int, int);


/*  apply the mask data to the alpha channel of the pixel data  */
void  apply_mask_to_alpha_channel         (unsigned char *, unsigned char *,
					   int, int, int);

/*  combine the mask data with the alpha channel of the pixel data  */
void  combine_mask_and_alpha_channel      (unsigned char *, unsigned char *,
					   int, int, int);


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void  copy_gray_to_inten_a_pixels         (unsigned char *, unsigned char *,
					   int, int);

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels              (unsigned char *, unsigned char *,
					   int, int);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_pixels              (unsigned char *, unsigned char *,
					   unsigned char *, int);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_a_pixels            (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_pixels                (unsigned char *, unsigned char *,
					   unsigned char *, int,
					   int *, int, int);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_a_pixels              (unsigned char *, unsigned char *,
					   unsigned char *, int,
					   int *, int, int);

/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void  combine_indexed_and_indexed_pixels  (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int);

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void  combine_indexed_and_indexed_a_pixels(unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int);

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void  combine_indexed_a_and_indexed_a_pixels(unsigned char *, unsigned char *,
					     unsigned char *, unsigned char *,
					     int, int *, int, int);

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void  combine_inten_a_and_indexed_a_pixels(unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   unsigned char *, int, int, int);

/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_pixels      (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int);

/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_a_pixels    (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int);

/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void  combine_inten_a_and_inten_pixels    (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int, int);

/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void  combine_inten_a_and_inten_a_pixels  (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int, int);

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void  combine_inten_a_and_channel_mask_pixels       (unsigned char *, unsigned char *,
						     unsigned char *, unsigned char *,
						     int, int, int);

void  combine_inten_a_and_channel_selection_pixels  (unsigned char *, unsigned char *,
						     unsigned char *, unsigned char *,
						     int, int, int);

/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_inten_pixels                 (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int, int, int, int);

/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_indexed_pixels               (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int, int, int, int);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void  replace_inten_pixels                (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int, int, int, int);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void  replace_indexed_pixels                (unsigned char *, unsigned char *,
					     unsigned char *, unsigned char *,
					     int, int *, int, int, int, int, int);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void  erase_inten_pixels                  (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */
void  erase_indexed_pixels                (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int *, int, int);

/*  extract information from intensity pixels based on
 *  a mask.
 */
void  extract_from_inten_pixels           (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   int, int, int, int);

/*  extract information from indexed pixels based on
 *  a mask.
 */
void  extract_from_indexed_pixels         (unsigned char *, unsigned char *,
					   unsigned char *, unsigned char *,
					   unsigned char *, int, int, int, int);


/*  variable source to RGB color mapping
 *  src_type == 0  (RGB)
 *  src_type == 1  (GRAY)
 *  src_type == 2  (INDEXED)
 */
void
map_to_color                              (int, unsigned char *,
					   unsigned char *, unsigned char *);


/*  RGB to index mapping functions...
 *
 *  Hash table lookup speeds up the standard
 *  least squares method
 */
int    map_rgb_to_indexed                 (unsigned char *, int, GimpImage*,
					   int, int, int);


/*  Region functions  */
void  color_region                        (PixelRegion *, unsigned char *);


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

void  gaussian_blur_region                (PixelRegion *, double);

void  border_region                       (PixelRegion *, gint16);

void  scale_region                        (PixelRegion *, PixelRegion *);

void  scale_region_no_resample            (PixelRegion *, PixelRegion *);

void  subsample_region                    (PixelRegion *, PixelRegion *,
					   int);

float shapeburst_region                   (PixelRegion *, PixelRegion *);

void thin_region                           (PixelRegion *, gint16);

void fatten_region                         (PixelRegion *, gint16);

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
void  hls_to_rgb            (int *, int *, int *);

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

/*  Layer Modes  */
#define NORMAL_MODE        0
#define DISSOLVE_MODE      1
#define BEHIND_MODE        2
#define MULTIPLY_MODE      3
#define SCREEN_MODE        4
#define OVERLAY_MODE       5
#define DIFFERENCE_MODE    6
#define ADDITION_MODE      7
#define SUBTRACT_MODE      8
#define DARKEN_ONLY_MODE   9
#define LIGHTEN_ONLY_MODE  10
#define HUE_MODE           11
#define SATURATION_MODE    12
#define COLOR_MODE         13
#define VALUE_MODE         14
#define DIVIDE_MODE        15
#define ERASE_MODE         16
#define REPLACE_MODE       17

/*  Applying layer modes...  */

int   apply_layer_mode         (unsigned char *, unsigned char *,
				unsigned char **, int, int, int,
				int, int, int, int, int, int, int *);

int   apply_indexed_layer_mode (unsigned char *, unsigned char *,
				unsigned char **, int, int, int);

#endif  /*  __PAINT_FUNCS_H__  */
