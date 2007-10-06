/* GIMP - The GNU Image Manipulation Program
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


/*  Called initially to setup rendering features  */
void  paint_funcs_setup     (void);
void  paint_funcs_free      (void);


/*  Paint functions  */

void  color_pixels          (guchar       *dest,
                             const guchar *color,
                             guint         w,
                             guint         bytes);

void  color_pixels_mask     (guchar       *dest,
                             const guchar *mask,
                             const guchar *color,
                             guint         w,
                             guint         bytes);

void  pattern_pixels_mask   (guchar       *dest,
                             const guchar *mask,
                             TempBuf      *pattern,
                             guint         w,
                             guint         bytes,
                             gint          x,
                             gint          y);

void  blend_pixels          (const guchar *src1,
                             const guchar *src2,
                             guchar       *dest,
                             guchar        blend,
                             guint         w,
                             guint         bytes);

void  shade_pixels          (const guchar *src,
                             guchar       *dest,
                             const guchar *color,
                             guchar        rblend,
                             guint         w,
                             guint         bytes,
                             gboolean      has_alpha);

void  extract_alpha_pixels  (const guchar *src,
                             const guchar *mask,
                             guchar       *dest,
                             guint         w,
                             guint         bytes);

void  swap_pixels           (guchar       *src,
                             guchar       *dest,
                             guint         length);

void  scale_pixels          (const guchar *src,
                             guchar       *dest,
                             guint         length,
                             gint          scale);

void  add_alpha_pixels      (const guchar *src,
                             guchar       *dest,
                             guint         length,
                             guint         bytes);

void  flatten_pixels        (const guchar *src,
                             guchar       *dest,
                             const guchar *bg,
                             guint         length,
                             guint         bytes);

void  gray_to_rgb_pixels    (const guchar *src,
                             guchar       *dest,
                             guint         length,
                             guint         bytes);

void  copy_component_pixels (const guchar *src,
                             guchar       *dest,
                             guint         length,
                             guint         bytes,
                             guint         pixel);

void  copy_color_pixels     (const guchar *src,
                             guchar       *dest,
                             guint         length,
                             guint         bytes);

/*  apply the mask data to the alpha channel of the pixel data  */
void  apply_mask_to_alpha_channel         (guchar       *src,
                                           const guchar *mask,
                                           guint         opacity,
                                           guint         length,
                                           guint         bytes);

/*  combine the mask data with the alpha channel of the pixel data  */
void  combine_mask_and_alpha_channel_stipple (guchar       *src,
                                              const guchar *mask,
                                              guint         opacity,
                                              guint         length,
                                              guint         bytes);

void  combine_mask_and_alpha_channel_stroke  (guchar       *src,
                                              const guchar *mask,
                                              guint         opacity,
                                              guint         length,
                                              guint         bytes);

/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void  copy_gray_to_inten_a_pixels         (const guchar *src,
                                           guchar       *dest,
                                           guint         length,
                                           guint         bytes);

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void  initial_channel_pixels              (const guchar *src,
                                           guchar       *dest,
                                           guint         length,
                                           guint         bytes);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_pixels              (const guchar *src,
                                           guchar       *dest,
                                           const guchar *cmap,
                                           guint         length);

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void  initial_indexed_a_pixels            (const guchar *src,
                                           guchar       *dest,
                                           const guchar *mask,
                                           const guchar *no_mask,
                                           const guchar *cmap,
                                           guint         opacity,
                                           guint         length);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_pixels                (const guchar   *src,
                                           guchar         *dest,
                                           const guchar   *mask,
                                           const guchar   *no_mask,
                                           guint           opacity,
                                           const gboolean *affect,
                                           guint           length,
                                           guint           bytes);

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void  initial_inten_a_pixels              (const guchar   *src,
                                           guchar         *dest,
                                           const guchar   *mask,
                                           guint           opacity,
                                           const gboolean *affect,
                                           guint           length,
                                           guint           bytes);

/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void  combine_indexed_and_indexed_pixels  (const guchar   *src1,
                                           const guchar   *src2,
                                           guchar         *dest,
                                           const guchar   *mask,
                                           guint           opacity,
                                           const gboolean *affect,
                                           guint           length,
                                           guint           bytes);

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void  combine_indexed_and_indexed_a_pixels (const guchar *src1,
                                            const guchar *src2,
                                            guchar       *dest,
                                            const guchar *mask,
                                            guint         opacity,
                                            const gint   *affect,
                                            guint         length,
                                            guint         bytes);

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void  combine_indexed_a_and_indexed_a_pixels(const guchar   *src1,
                                             const guchar   *src2,
                                             guchar         *dest,
                                             const guchar   *mask,
                                             guint           opacity,
                                             const gboolean *affect,
                                             guint           length,
                                             guint           bytes);

void combine_inten_a_and_indexed_pixels     (const guchar   *src1,
                                             const guchar   *src2,
                                             guchar         *dest,
                                             const guchar   *mask,
                                             const guchar   *cmap,
                                             guint           opacity,
                                             guint           length,
                                             guint           bytes);

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void  combine_inten_a_and_indexed_a_pixels (const guchar *src1,
                                            const guchar *src2,
                                            guchar       *dest,
                                            const guchar *mask,
                                            const guchar *cmap,
                                            guint         opacity,
                                            guint         length,
                                            guint         bytes);

/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_pixels       (const guchar   *src1,
                                            const guchar   *src2,
                                            guchar         *dest,
                                            const guchar   *mask,
                                            guint           opacity,
                                            const gboolean *affect,
                                            guint           length,
                                            guint           bytes);

/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void  combine_inten_and_inten_a_pixels    (const guchar   *src1,
                                           const guchar   *src2,
                                           guchar         *dest,
                                           const guchar   *mask,
                                           guint           opacity,
                                           const gboolean *affect,
                                           guint           length,
                                           guint           bytes);

/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void  combine_inten_a_and_inten_pixels    (const guchar   *src1,
                                           const guchar   *src2,
                                           guchar         *dest,
                                           const guchar   *mask,
                                           guint           opacity,
                                           const gboolean *affect,
                                           gboolean        mode_affect,
                                           guint           length,
                                           guint           bytes);

/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void  combine_inten_a_and_inten_a_pixels   (const guchar   *src1,
                                            const guchar   *src2,
                                            guchar         *dest,
                                            const guchar   *mask,
                                            guint           opacity,
                                            const gboolean *affect,
                                            gboolean        mode_affect,
                                            guint           length,
                                            guint           bytes);

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void  combine_inten_a_and_channel_mask_pixels(const guchar *src,
                                              const guchar *channel,
                                              guchar       *dest,
                                              const guchar *col,
                                              guint         opacity,
                                              guint         length,
                                              guint         bytes);

void  combine_inten_a_and_channel_selection_pixels(const guchar *src,
                                                   const guchar *channel,
                                                   guchar       *dest,
                                                   const guchar *col,
                                                   guint         opacity,
                                                   guint         length,
                                                   guint         bytes);

/*  extract information from intensity pixels based on
 *  a mask.
 */
void  extract_from_inten_pixels           (guchar       *src,
                                           guchar       *dest,
                                           const guchar *mask,
                                           const guchar *bg,
                                           gboolean      cut,
                                           guint         length,
                                           guint         src_bytes,
                                           guint         dest_bytes);

/*  extract information from indexed pixels based on
 *  a mask.
 */
void  extract_from_indexed_pixels         (guchar       *src,
                                           guchar       *dest,
                                           const guchar *mask,
                                           const guchar *cmap,
                                           const guchar *bg,
                                           gboolean      cut,
                                           guint         length,
                                           guint         src_bytes,
                                           guint         dest_bytes);


/*  Region functions  */
void  color_region                        (PixelRegion  *dest,
                                           const guchar *color);
void  color_region_mask                   (PixelRegion  *dest,
                                           PixelRegion  *mask,
                                           const guchar *color);

void  pattern_region                      (PixelRegion  *dest,
                                           PixelRegion  *mask,
                                           TempBuf      *pattern,
                                           gint          off_x,
                                           gint          off_y);

void  blend_region                        (PixelRegion *src1,
                                           PixelRegion *src2,
                                           PixelRegion *dest,
                                           guchar       blend);

void  shade_region                        (PixelRegion *src,
                                           PixelRegion *dest,
                                           guchar      *color,
                                           guchar       blend);

void  copy_region                         (PixelRegion *src,
                                           PixelRegion *dest);
void  copy_region_nocow                   (PixelRegion *src,
                                           PixelRegion *dest);

void  add_alpha_region                    (PixelRegion *src,
                                           PixelRegion *dest);

void  flatten_region                      (PixelRegion *src,
                                           PixelRegion *dest,
                                           guchar      *bg);

void  extract_alpha_region                (PixelRegion *src,
                                           PixelRegion *mask,
                                           PixelRegion *dest);

void  extract_from_region                 (PixelRegion       *src,
                                           PixelRegion       *dest,
                                           PixelRegion       *mask,
                                           const guchar      *cmap,
                                           const guchar      *bg,
                                           GimpImageBaseType  type,
                                           gboolean           cut);


void  convolve_region                     (PixelRegion         *srcR,
                                           PixelRegion         *destR,
                                           const gfloat        *matrix,
                                           gint                 size,
                                           gdouble              divisor,
                                           GimpConvolutionType  mode,
                                           gboolean             alpha_weighting);

void  multiply_alpha_region               (PixelRegion *srcR);

void  separate_alpha_region               (PixelRegion *srcR);

void  gaussian_blur_region                (PixelRegion *srcR,
                                           gdouble      radius_x,
                                           gdouble      radius_y);

void  border_region                       (PixelRegion *src,
                                           gint16       xradius,
                                           gint16       yradius,
                                           gboolean     feather,
                                           gboolean     edge_lock);

gfloat shapeburst_region                  (PixelRegion      *srcPR,
                                           PixelRegion      *distPR,
                                           GimpProgressFunc  progress_callback,
                                           gpointer          progress_data);

void  thin_region                         (PixelRegion *region,
                                           gint16       xradius,
                                           gint16       yradius,
                                           gboolean     edge_lock);

void  fatten_region                       (PixelRegion *region,
                                           gint16       xradius,
                                           gint16       yradius);

void  smooth_region                       (PixelRegion *region);
void  erode_region                        (PixelRegion *region);
void  dilate_region                       (PixelRegion *region);

void  swap_region                         (PixelRegion *src,
                                           PixelRegion *dest);


/*  Apply a mask to an image's alpha channel  */
void  apply_mask_to_region                (PixelRegion *src,
                                           PixelRegion *mask,
                                           guint        opacity);

/*  Combine a mask with an image's alpha channel  */
void  combine_mask_and_region             (PixelRegion *src,
                                           PixelRegion *mask,
                                           guint        opacity,
                                           gboolean     stipple);

/*  Copy a gray image to an intensity-alpha region  */
void  copy_gray_to_region                 (PixelRegion *src,
                                           PixelRegion *dest);

/*  Copy a component (indexed by pixel) to a 1-byte region  */
void  copy_component                      (PixelRegion *src,
                                           PixelRegion *dest,
                                           guint        pixel);

/*  Copy the color bytes (without alpha channel) to a src_bytes-1 - byte region  */
void  copy_color                          (PixelRegion *src,
                                           PixelRegion *dest);

void  initial_region                      (PixelRegion    *src,
                                           PixelRegion    *dest,
                                           PixelRegion    *mask,
                                           const guchar   *data,
                                           guint           opacity,
                                           GimpLayerModeEffects  mode,
                                           const gboolean *affect,
                                           InitialMode     type);

void  combine_regions                     (PixelRegion    *src1,
                                           PixelRegion    *src2,
                                           PixelRegion    *dest,
                                           PixelRegion    *mask,
                                           const guchar   *data,
                                           guint           opacity,
                                           GimpLayerModeEffects  mode,
                                           const gboolean *affect,
                                           CombinationMode type);

void  combine_regions_replace             (PixelRegion    *src1,
                                           PixelRegion    *src2,
                                           PixelRegion    *dest,
                                           PixelRegion    *mask,
                                           const guchar   *data,
                                           guint           opacity,
                                           const gboolean *affect,
                                           CombinationMode type);


#endif  /*  __PAINT_FUNCS_H__  */
