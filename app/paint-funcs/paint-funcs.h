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

#ifndef __PAINT_FUNCS_H__
#define __PAINT_FUNCS_H__


/*  Called initially to setup accelerated rendering features  */
void  paint_funcs_setup     (void);
void  paint_funcs_free      (void);


/*  Paint functions  */

void  color_pixels          (guchar *dest, const guchar *color,
			     guint w, guint bytes);

void  blend_pixels          (const guchar *src1,
			     const guchar *src2,
			     guchar *dest, 
			     guchar blend, guint w, 
			     guint bytes, guint has_alpha);

void  shade_pixels          (const guchar *src, guchar *dest,
			     const guchar *color,
			     guchar rblend, guint w, 
			     guint bytes, guint has_alpha);

void  extract_alpha_pixels  (const guchar *src,
			     const guchar *mask,
			     guchar *dest,
			     guint w, guint bytes);

void  swap_pixels           (guchar *src, guchar *dest,
			     guint length);

void  scale_pixels          (const guchar *src, guchar *dest,
			     guint length, int scale);

void  add_alpha_pixels      (const guchar *src, guchar *dest,
			     guint length, guint bytes);

void  flatten_pixels        (const guchar *src, guchar *dest,
			     const guchar *bg, guint length, guint bytes);

void  gray_to_rgb_pixels    (const guchar *src, guchar *dest,
			     guint length, guint bytes);


/*  apply the mask data to the alpha channel of the pixel data  */
void  apply_mask_to_alpha_channel         (guchar *src,
					   const guchar *mask,
					   guint opacity, guint length, guint bytes);

/*  combine the mask data with the alpha channel of the pixel data  */
void  combine_mask_and_alpha_channel      (guchar *src,
					   const guchar *mask,
					   guint opacity, guint length, guint bytes);


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void  copy_gray_to_inten_a_pixels         (const guchar *src,
					   guchar *dest,
					   guint length, guint bytes);

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels              (const guchar *src,
					   guchar *dest,
					   guint length, guint bytes);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_pixels              (const guchar *src,
					   guchar *dest,
					   const guchar *cmap,
					   guint length);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_a_pixels            (const guchar *src,
					   guchar *dest,
					   const guchar *mask,
					   const guchar *cmap,
					   guint opacity, guint length);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_pixels                (const guchar *src,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, const int *affect,
					   guint length, guint bytes);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_a_pixels              (const guchar *src,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, const int *affect,
					   guint length, guint bytes);

/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void  combine_indexed_and_indexed_pixels  (const guchar *src1,
					   const guchar *src2,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, const int *affect,
					   guint length, guint bytes);

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void  combine_indexed_and_indexed_a_pixels (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    guint	  opacity,
					    const int    *affect,
					    guint	  length,
					    guint	  bytes);

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void  combine_indexed_a_and_indexed_a_pixels(const guchar *src1,
					     const guchar *src2,
					     guchar       *dest,
					     const guchar *mask,
					     guint 	   opacity,
					     const int           *affect,
					     guint	   length,
					     guint	   bytes);

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void  combine_inten_a_and_indexed_a_pixels (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    const guchar *cmap,
					    guint	  opacity,
					    guint	  length,
					    guint	  bytes);

/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_pixels       (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    guint	  opacity,
					    const gboolean *affect,
					    guint	  length,
					    guint         bytes);

/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_a_pixels    (const guchar *src1,
					   const guchar *src2,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, 
					   const gboolean *affect,
					   guint length, guint bytes);

/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void  combine_inten_a_and_inten_pixels    (const guchar *src1,
					   const guchar *src2,
					   guchar *dest,
					   const guchar *mask,
					   guint opacity, const int *affect,
					   guint mode_affect, guint length,
					   guint bytes);

/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void  combine_inten_a_and_inten_a_pixels   (const guchar *src1,
					    const guchar *src2,
					    guchar       *dest,
					    const guchar *mask,
					    guint	  opacity,
					    const gboolean *affect,
					    guint         mode_affect,
					    guint	  length,
					    guint	  bytes);

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void  combine_inten_a_and_channel_mask_pixels(const guchar *src,
					      const guchar *channel,
					      guchar       *dest,
					      const guchar *col,
					      guint	    opacity,
					      guint	    length,
					      guint	    bytes);

void  combine_inten_a_and_channel_selection_pixels(const guchar *src,
						   const guchar *channel,
						   guchar       *dest,
						   const guchar *col,
						   guint	 opacity,
						   guint	 length,
						   guint	 bytes);

/*  extract information from intensity pixels based on
 *  a mask.
 */
void  extract_from_inten_pixels           (guchar       *src,
					   guchar       *dest,
					   const guchar *mask,
					   const guchar *bg,
					   guint         cut,
					   guint         length,
					   guint         bytes,
					   guint         has_alpha);

/*  extract information from indexed pixels based on
 *  a mask.
 */
void  extract_from_indexed_pixels         (guchar *src,
					   guchar       *dest,
					   const guchar *mask,
					   const guchar *cmap,
					   const guchar *bg,
					   guint         cut,
					   guint         length,
					   guint         bytes,
					   guint         has_alpha);


/*  variable source to RGB color mapping
 *  src_type == 0  (RGB)
 *  src_type == 1  (GRAY)
 *  src_type == 2  (INDEXED)
 */
void  map_to_color                        (guint                  src_type,
					   const guchar *cmap,
					   const guchar *src,
					   guchar       *rgb);

/*  Region functions  */
void  color_region                        (PixelRegion *dest, 
					   const guchar *color);


void  blend_region                        (PixelRegion *, PixelRegion *,
					   PixelRegion *, int);

void  shade_region                        (PixelRegion *, PixelRegion *,
					   guchar *, int);

void  copy_region                         (PixelRegion *, PixelRegion *);

void  add_alpha_region                    (PixelRegion *, PixelRegion *);

void  flatten_region                      (PixelRegion *, PixelRegion *,
					   guchar *);

void  extract_alpha_region                (PixelRegion *, PixelRegion *,
					   PixelRegion *);

void  extract_from_region                 (PixelRegion *, PixelRegion *,
					   PixelRegion *, guchar *,
					   guchar *, guint, guint, guint);


void  convolve_region                     (PixelRegion *,
					   PixelRegion *,
					   int *, int, int,
					   ConvolutionType);

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
void  apply_mask_to_region                (PixelRegion *, PixelRegion *, guint);

/*  Combine a mask with an image's alpha channel  */
void  combine_mask_and_region             (PixelRegion *, PixelRegion *, guint);

/*  Copy a gray image to an intensity-alpha region  */
void  copy_gray_to_region                 (PixelRegion *, PixelRegion *);

/*  Lay down the groundwork for layer construction...
 *  This includes background images for indexed or non-alpha
 *  images, floating selections, selective display of intensity
 *  channels, and display of arbitrary mask channels
 */
typedef enum /*< skip >*/
{
INITIAL_CHANNEL_MASK = 0,
INITIAL_CHANNEL_SELECTION,
INITIAL_INDEXED,
INITIAL_INDEXED_ALPHA,
INITIAL_INTENSITY,
INITIAL_INTENSITY_ALPHA,
} InitialMode;

/*  Combine two source regions with the help of an optional mask
 *  region into a destination region.  This is used for constructing
 *  layer projections, and for applying image patches to an image
 */
typedef enum /*< skip >*/ 
{
  NO_COMBINATION = 0,
  COMBINE_INDEXED_INDEXED,
  COMBINE_INDEXED_INDEXED_A,
  COMBINE_INDEXED_A_INDEXED_A,
  COMBINE_INTEN_A_INDEXED_A,
  COMBINE_INTEN_A_CHANNEL_MASK,
  COMBINE_INTEN_A_CHANNEL_SELECTION,
  COMBINE_INTEN_INTEN,
  COMBINE_INTEN_INTEN_A,
  COMBINE_INTEN_A_INTEN,
  COMBINE_INTEN_A_INTEN_A,

  /*  Non-conventional combination modes  */
  BEHIND_INTEN,
  BEHIND_INDEXED,
  REPLACE_INTEN,
  REPLACE_INDEXED,
  ERASE_INTEN,
  ERASE_INDEXED,
  ANTI_ERASE_INTEN,
  ANTI_ERASE_INDEXED,
  COLOR_ERASE_INTEN,
} CombinationMode;

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

void  initial_region                      (PixelRegion *, PixelRegion *,
					   PixelRegion *, guchar *,
					   int, LayerModeEffects, int *, 
					   CombinationMode);

void  combine_regions                     (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   guchar *, guint,
					   LayerModeEffects,
					   int *, CombinationMode);

void  combine_regions_replace             (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   guchar *,
					   guint, int *, CombinationMode);

#endif  /*  __PAINT_FUNCS_H__  */
