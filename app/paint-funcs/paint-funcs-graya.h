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

#ifndef __PAINT_FUNCS_GRAYA_H__
#define __PAINT_FUNCS_GRAYA_H__

#include "paint_funcsP.h"

void update_tile_rowhints_graya (Tile *tile,
			         gint  ymin,
			         gint  ymax);


/*  Paint functions  */

void  color_pixels_graya     (guchar *dest, const guchar *color,
			      guint w);

void  blend_pixels_graya     (const guchar *src1,
			      const guchar *src2,
			      guchar *dest, 
			      gint blend, gint w);

void  shade_pixels_graya     (const guchar *src, guchar *dest,
			      const guchar *color,
			      gint blend, gint w);

void  extract_alpha_pixels_graya (const guchar *src,
			      const guchar *mask,
			      guchar *dest,
			      gint w);

void  darken_pixels_graya    (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha1);

void  lighten_pixels_graya   (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  hsv_only_pixels_graya_graya (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, guint mode, guint length);

void  color_only_pixels_graya_graya (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, guint mode, guint length);

void  multiply_pixels_graya  (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  divide_pixels_graya    (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  screen_pixels_graya    (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  overlay_pixels_graya   (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  add_pixels_graya        (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  subtract_pixels_graya  (const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  difference_pixels_graya(const guchar *src1,
			      const guchar *src2,
	 		      guchar *dest, gint length,
			      gint bytes2, gint has_alpha2);

void  dissolve_pixels_graya  (const guchar *src,
			      guchar *dest, guint x, guint y,
			      guint opacity, guint length);

void  flatten_pixels_graya   (const guchar *src, guchar *dest,
			      const guchar *bg, guint length);


/*  apply the mask data to the alpha channel of the pixel data  */
void  apply_mask_to_alpha_channel_graya    (guchar *src,
					    const guchar *mask,
					    gint opacity, gint length);

/*  combine the mask data with the alpha channel of the pixel data  */
void  combine_mask_and_alpha_channel_graya (guchar *src,
					    const guchar *mask,
					    gint opacity, gint length);


/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels_graya         (const guchar *src,
					    guchar *dest,
					    gint length);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_pixels_graya                 (const guchar *src,
					    guchar *dest,
					    const guchar *mask,
					    gint opacity, const gint *affect,
					    gint length);

/*  combine an RGB or GRAY image with an GRAYA or GRAYA image
 *  destination is intensity-alpha...
 */
void  combine_pixels_graya_rgb             (const guchar *src1,
					    const guchar *src2,
					    guchar *dest,
					    const guchar *mask,
					    gint opacity, const gint *affect,
					    gint mode_affect, gint length);

/*  combine an GRAYA or GRAYA image with an GRAYA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void  combine_pixels_graya_graya             (const guchar *src1,
					      const guchar *src2,
					      guchar       *dest,
					      const guchar *mask,
					      gint                 opacity,
					      const gint          *affect,
					      gint                 mode_affect,
					      gint                 length);
 
/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */

void  combine_graya_and_indexeda_pixels (apply_combine_layer_info *info);

void  combine_graya_and_channel_mask_pixels (apply_combine_layer_info *info);

void  combine_graya_and_channel_selection_pixels (apply_combine_layer_info *info);

/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_pixels_graya                    (const guchar *src1,
					      const guchar *src2,
					      guchar       *dest,
					      const guchar *mask,
					      gint                  opacity,
					      const gint           *affect,
					      gint                  length,
					      gint                  bytes2,
					      gint                  has_alpha2);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void  replace_pixels_graya                (const guchar *src1,
					   const guchar *src2,
					   guchar       *dest,
					   const guchar *mask,
					   gint                  opacity,
					   const gint           *affect,
					   gint                  length,
					   gint                  bytes2,
					   gint                  has_alpha2);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void  erase_pixels_graya                  (const guchar *src1,
					   const guchar *src2,
					   guchar       *dest,
					   const guchar *mask,
					   gint                  opacity,
					   const gint           *affect,
					   gint                  length);

/*  extract information from intensity pixels based on
 *  a mask.
 */
void  extract_from_pixels_graya           (guchar       *src,
					   guchar       *dest,
					   const guchar *mask,
					   const guchar *bg,
					   gint                  cut,
					   gint                  length);

void apply_layer_mode_graya_gray (apply_combine_layer_info *info);

void apply_layer_mode_graya_graya (apply_combine_layer_info *info);

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

#endif  /*  __PAINT_FUNCS_GRAYA_H__  */
