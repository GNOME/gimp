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
					   guint mode_affect, int length,
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
					    guint               mode_affect,
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
void  extract_from_indexed_pixels         (unsigned char *src,
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
void  map_to_color                        (int                  src_type,
					   const unsigned char *cmap,
					   const unsigned char *src,
					   unsigned char       *rgb);

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
typedef enum
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
typedef enum
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
} CombinationMode;

/* Opacities */
#define TRANSPARENT_OPACITY        0
#define OPAQUE_OPACITY             255

void  initial_region                      (PixelRegion *, PixelRegion *,
					   PixelRegion *, unsigned char *,
					   int, LayerModeEffects, int *, 
					   CombinationMode);

void  combine_regions                     (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   unsigned char *, guint,
					   LayerModeEffects,
					   int *, CombinationMode);

void  combine_regions_replace             (PixelRegion *, PixelRegion *,
					   PixelRegion *, PixelRegion *,
					   unsigned char *,
					   guint, int *, CombinationMode);

#endif  /*  __PAINT_FUNCS_H__  */
