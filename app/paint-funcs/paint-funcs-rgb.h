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

#ifndef __PAINT_FUNCS_RGB_H__
#define __PAINT_FUNCS_RGB_H__

#include "paint_funcsP.h"

void update_tile_rowhints_rgb (Tile *tile,
			       guint  ymin,
			       guint  ymax);

/*  Paint functions  */

void  color_pixels_rgb     (guchar *dest, const guchar *color,
			    guint w);

void  blend_pixels_rgb     (const guchar *src1,
			    const guchar *src2,
			    guchar *dest, 
			    guint blend, guint w);

void  shade_pixels_rgb     (const guchar *src, guchar *dest,
			    const guchar *color,
			    guint blend, guint w);

void  extract_alpha_pixels_rgb (const guchar *src,
				const guchar *mask,
				guchar *dest,
				guint w);

void  darken_pixels_rgb    (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha1);

void  lighten_pixels_rgb   (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  hsv_only_pixels_rgb_rgb (const guchar *src1,
			       const guchar *src2,
			       guchar *dest, guint mode, guint length);

void  color_only_pixels_rgb_rgb (const guchar *src1,
				 const guchar *src2,
				 guchar *dest, guint mode, guint length);

void  multiply_pixels_rgb  (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  divide_pixels_rgb    (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  screen_pixels_rgb    (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  overlay_pixels_rgb   (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  add_pixels_rgb        (const guchar *src1,
			     const guchar *src2,
	 		     guchar *dest, guint length,
			     guint bytes2, guint has_alpha2);

void  subtract_pixels_rgb  (const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  difference_pixels_rgb(const guchar *src1,
			    const guchar *src2,
	 		    guchar *dest, guint length,
			    guint bytes2, guint has_alpha2);

void  dissolve_pixels_rgb  (const guchar *src,
			    guchar *dest, guint x, guint y,
			    guint opacity, guint length);

void  flatten_pixels_rgb   (const guchar *src, guchar *dest,
			    const guchar *bg, guint length);


/*  apply the mask data to the alpha channel of the pixel data  */
void  apply_mask_to_alpha_channel_rgb    (guchar *src,
					   const guchar *mask,
					   guint opacity, guint length);

/*  combine the mask data with the alpha channel of the pixel data  */
void  combine_mask_and_alpha_channel_rgb (guchar *src,
					   const guchar *mask,
					   guint opacity, guint length);


/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels_rgb         (const guchar *src,
					   guchar *dest,
					   guint length);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_pixels_rgb                 (const guchar *src,
					  guchar *dest,
					  const guchar *mask,
					  guint opacity, const guint *affect,
					  guint length);

/*  combine an RGB or GRAY image with an RGB or GRAYA image
 *  destination is intensity-alpha...
 */
void  combine_pixels_rgb_rgb             (const guchar *src1,
					   const guchar *src2,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, const guint *affect,
					   guint mode_affect, guint length);

/*  combine an RGB or GRAYA image with an RGB or GRAYA image
 *  destination is of course intensity-alpha...
 */
void  combine_pixels_rgb_rgb             (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    guint                 opacity,
					    const guint          *affect,
					    guint                 mode_affect,
					    guint                 length);

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void  combine_pixels_rgb_channel_mask       (const guchar *src,
					      const guchar *channel,
					      guchar       *dest,
					      const guchar *col,
					      guint                  opacity,
					      guint                  length);

void  combine_pixels_rgb_channel_selection  (const guchar *src,
					      const guchar *channel,
					      guchar       *dest,
					      const guchar *col,
					      guint                  opacity,
					      guint                  length);

/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void  behind_pixels_rgb                    (const guchar *src1,
					     const guchar *src2,
					     guchar       *dest,
					     const guchar *mask,
					     guint                  opacity,
					     const guint           *affect,
					     guint                  length,
					     guint                  bytes2,
					     guint                  has_alpha2);

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void  replace_pixels_rgb                (const guchar *src1,
					  const guchar *src2,
					  guchar       *dest,
					  const guchar *mask,
					  guint                  opacity,
					  const guint           *affect,
					  guint                  length,
					  guint                  bytes2,
					  guint                  has_alpha2);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void  erase_pixels_rgb                  (const guchar *src1,
					  const guchar *src2,
					  guchar       *dest,
					  const guchar *mask,
					  guint                  opacity,
					  const guint           *affect,
					  guint                  length);

/*  extract information from intensity pixels based on
 *  a mask.
 */
void  extract_from_pixels_rgb           (guchar       *src,
					 guchar       *dest,
					 const guchar *mask,
					 const guchar *bg,
					 gboolean      cut,
					 guint	       length);


void apply_layer_mode_rgb_rgb (apply_combine_layer_info *info);

void apply_layer_mode_rgb_rgba (apply_combine_layer_info *info);

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

#endif  /*  __PAINT_FUNCS_RGB_H__  */
