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

#ifndef __PAINT_FUNCS_INDEXED_H__
#define __PAINT_FUNCS_INDEXED_H__

#include "paint_funcsP.h"

void update_tile_rowhints_indexed (Tile *tile,
				   gint  ymin,
			           gint  ymax);

/*  Called initially to setup accelerated rendering features  */
void  paint_funcs_setup     (void);
void  paint_funcs_free      (void);

void paint_funcs_invalidate_color_hash_table (GimpImage* gimage,
					      gint       index);

/*  Pagint functions  */

void  color_pixels          (guchar *dest, const guchar *color,
			     gint w, gint bytes);

void  blend_pixels          (const guchar *src1,
			     const guchar *src2,
			     guchar *dest, 
			     gint blend, gint w, gint bytes, gint has_alpha);

void  shade_pixels          (const guchar *src, guchar *dest,
			     const guchar *color,
			     gint blend, gint w, gint bytes, gint has_alpha);

void  extract_alpha_pixels  (const guchar *src,
			     const guchar *mask,
			     guchar *dest,
			     gint w, gint bytes);

void  darken_pixels         (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  lighten_pixels        (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  hsv_only_pixels       (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint mode, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  color_only_pixels     (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint mode, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  multiply_pixels       (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  divide_pixels         (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  screen_pixels         (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  overlay_pixels        (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  add_pixels             (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  subtract_pixels       (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  difference_pixels     (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, gint length,
			     gint bytes1, gint bytes2,
			     gint has_alpha1, gint has_alpha2);

void  dissolve_pixels       (const guchar *src,
			     guchar *dest, gint x, gint y,
			     gint opacity, gint length, gint sb, gint db,
			     gint has_alpha);

void  swap_pixels           (guchar *src, guchar *dest,
			     gint length);

void  scale_pixels          (const guchar *src, guchar *dest,
			     gint length, gint scale);

void  add_alpha_pixels      (const guchar *src, guchar *dest,
			     gint length, gint bytes);

void  flatten_pixels        (const guchar *src, guchar *dest,
			     const guchar *bg, gint length, gint bytes);


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void  copy_gray_to_inten_a_pixels         (const guchar *src,
					   guchar *dest,
					   gint length, gint bytes);

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels              (const guchar *src,
					   guchar *dest,
					   gint length, gint bytes);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_pixels_indexed              (const guchar *src,
					   guchar *dest,
					   const guchar *cmap,
					   gint length);

/*  combine indexed images with an optional mask which
 *  is ginterpreted as binary...destination is indexed...
 */
void  combine_indexed_and_indexed_pixels  (const guchar *src1,
					   const guchar *src2,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, const gboolean *affect,
					   guint length);

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void  combine_indexed_and_indexed_a_pixels (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    guint                  opacity,
					    const gboolean        *affect,
					    guint                  length);

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void  combine_indexed_a_and_indexed_a_pixels(const guchar *src1,
					     const guchar *src2,
					     guchar       *dest,
					     const guchar *mask,
					     guint                  opacity,
					     const gboolean        *affect,
					     guint                  length);

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void  combine_guinten_a_and_indexed_a_pixels (const guchar *src1,
					      const guchar *src2,
					      guchar       *dest,
					      const guchar *mask,
					      const guchar *cmap,
					      guint                  opacity,
					      guint                  length);


/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_inten_pixels                    (const guchar *src1,
					      const guchar *src2,
					      guchar       *dest,
					      const guchar *mask,
					      gint                  opacity,
					      const gint           *affect,
					      gint                  length,
					      gint                  bytes1,
					      gint                  bytes2,
					      gint                  has_alpha1,
					      gint                  has_alpha2);

/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_indexed_pixels                (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    gint                  opacity,
					    const gint           *affect,
					    gint                  length,
					    gint                  bytes1,
					    gint                  bytes2,
					    gint                  has_alpha1,
					    gint                  has_alpha2);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constragints
 */
void  replace_inten_pixels                (const guchar *src1,
					   const guchar *src2,
					   guchar       *dest,
					   const guchar *mask,
					   gint                  opacity,
					   const gint           *affect,
					   gint                  length,
					   gint                  bytes1,
					   gint                  bytes2,
					   gint                  has_alpha1,
					   gint                  has_alpha2);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constragints
 */
void  replace_indexed_pixels                (const guchar *src1,
					     const guchar *src2,
					     guchar       *dest,
					     const guchar *mask,
					     gint                  opacity,
					     const gint           *affect,
					     gint                  length,
					     gint                  bytes1,
					     gint                  bytes2,
					     gint                  has_alpha1,
					     gint                  has_alpha2);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void  erase_inten_pixels                  (const guchar *src1,
					   const guchar *src2,
					   guchar       *dest,
					   const guchar *mask,
					   gint                  opacity,
					   const gint           *affect,
					   gint                  length,
					   gint                  bytes);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */
void  erase_indexed_pixels                (const guchar *src1,
					   const guchar *src2,
					   guchar       *dest,
					   const guchar *mask,
					   gint                  opacity,
					   const gint           *affect,
					   gint                  length,
					   gint                  bytes);


/*  extract information from indexed pixels based on
 *  a mask.
 */
void  extract_from_pixels_indexed          (guchar *src,
					    guchar       *dest,
					    const guchar *mask,
					    const guchar *cmap,
					    const guchar *bg,
					    gint                  cut,
					    gint                  length,
					    gint                  bytes,
					    gint                  has_alpha);


/*  variable source to RGB color mapping
 *  src_type == 0  (RGB)
 *  src_type == 1  (GRAY)
 *  src_type == 2  (INDEXED)
 */
void
map_to_color                              (gint                  src_type,
					   const guchar *cmap,
					   const guchar *src,
					   guchar       *rgb);


/*  RGB to index mapping functions...
 *
 *  Hash table lookup speeds up the standard
 *  least squares method
 */
gint   map_rgb_to_indexed                 (const guchar        *cmap,
					   gint                 num_cols,
					   const GimpImage     *gimage,
					   gint                 r,
					   gint                 g,
					   gint                 b);


/*  Region functions  */
void  color_region                        (PixelRegion *dest, 
					   const guchar *color);


void  blend_region                        (PixelRegion *, PixelRegion *,
					   PixelRegion *, gint);

void  shade_region                        (PixelRegion *, PixelRegion *,
					   guchar *, gint);

void  copy_region                         (PixelRegion *, PixelRegion *);

void  add_alpha_region                    (PixelRegion *, PixelRegion *);

void  flatten_region                      (PixelRegion *, PixelRegion *,
					   guchar *);

void  extract_alpha_region                (PixelRegion *, PixelRegion *,
					   PixelRegion *);

void  extract_from_region                 (PixelRegion *, PixelRegion *,
					   PixelRegion *, guchar *,
					   guchar *, gint, gint, gint);


void  convolve_region                     (PixelRegion *,
					   PixelRegion *,
					   gint *, gint, gint,
					   ConvolutionType);

void  multiply_alpha_region               (PixelRegion *);

void  separate_alpha_region               (PixelRegion *);

void  gaussian_blur_region                (PixelRegion *, double, double);

void  border_region                       (PixelRegion *, gint16, gint16);

void  scale_region                        (PixelRegion *, PixelRegion *);

void  scale_region_no_resample            (PixelRegion *, PixelRegion *);

void  subsample_region                    (PixelRegion *, PixelRegion *,
					   gint);

float shapeburst_region                   (PixelRegion *, PixelRegion *);

void thin_region                           (PixelRegion *, gint16 xradius,
					    gint16 yradius, gint edge_lock);

void fatten_region                         (PixelRegion *, 
					    gint16 xradius, gint16 yradius);

void  swap_region                         (PixelRegion *, PixelRegion *);


/*  Apply a mask to an image's alpha channel  */
void  apply_mask_to_region                (PixelRegion *, PixelRegion *, gint);

/*  Combine a mask with an image's alpha channel  */
void  combine_mask_and_region             (PixelRegion *, PixelRegion *, gint);

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

/*  Combine two source regions with the help of an optional mask
 *  region ginto a destination region.  This is used for constructing
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
#define ANTI_ERASE_INTEN                  26
#define ANTI_ERASE_INDEXED                27
#define NO_COMBINATION                    28

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

void  initial_region                      (PixelRegion *, PixelRegion *,
					   PixelRegion *, guchar *,
					   gint, LayerModeEffects, gint *, gint);

void  combine_regions                     (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   guchar *, gint,
					   LayerModeEffects,
					   gint *, gint);

void  combine_regions_replace             (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   guchar *,
					   gint, gint *, gint);


/*  Applying layer modes...  */

void apply_layer_mode_indexed_indexed (apply_combine_layer_info *info);

void apply_layer_mode_indexed_indexeda (apply_combine_layer_info *info);

#endif  /*  __PAINT_FUNCS_INDEXED_H__  */
