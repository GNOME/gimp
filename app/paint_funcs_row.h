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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __PAINT_FUNCS_ROW_H__
#define __PAINT_FUNCS_ROW_H__


/* forward declarations */
struct _Paint;
struct _PixelRow;


/* pixel level functions */
void
color_row           (
                     struct _PixelRow * dest,
                     struct _Paint    * col
                     );

void
blend_row           (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest,
                     struct _Paint    * blend
                     );

void 
shade_row           (
                     struct _PixelRow * src,
                     struct _PixelRow * dest,
                     struct _Paint    * col
                     );

void 
extract_alpha_row   (
                     struct _PixelRow * src,
                     struct _PixelRow * mask,
                     struct _PixelRow * dest
                     );

void 
darken_row          (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
lighten_row         (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
hsv_only_row        (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest,
                     int mode
                     );

void 
color_only_row      (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest,
                     int mode
                     );

void 
multiply_row        (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
screen_row          (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
overlay_row         (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
add_row             (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
subtract_row        (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
difference_row      (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest
                     );

void 
dissolve_row        (
                     struct _PixelRow * src,
                     struct _PixelRow * dest,
                     struct _Paint    * opacity,
                     int x,
                     int y
                     );

void 
replace_row         (
                     struct _PixelRow * src1,
                     struct _PixelRow * src2,
                     struct _PixelRow * dest,
                     struct _PixelRow * mask,
                     struct _Paint    * opacity
                     );

void 
swap_row            (
                     struct _PixelRow * src,
                     struct _PixelRow * dest
                     );
void 
scale_row           (
                     struct _PixelRow * src,
                     struct _PixelRow * dest,
                     gfloat scale
                     );

void 
add_alpha_row       (
                     struct _PixelRow * src,
                     struct _PixelRow * dest
                     );

void 
flatten_row         (
                     struct _PixelRow * src,
                     struct _PixelRow * dest,
                     struct _Paint    * bgcol
                     );

void 
apply_mask_to_alpha_row     (
                             struct _PixelRow * src,
                             struct _PixelRow * mask,
                             struct _Paint    * opacity
                             );

void 
combine_mask_and_alpha_row  (
                             struct _PixelRow * src,
                             struct _PixelRow * mask,
                             struct _Paint    * opacity
                             );

void 
copy_gray_to_inten_a_row    (
                             struct _PixelRow * src,
                             struct _PixelRow * dest
                             );

void 
initial_channel_row         (
                             struct _PixelRow * src,
                             struct _PixelRow * dest
                             );

/* FIXME cmap */
void 
initial_indexed_row         (
                             struct _PixelRow * src,
                             struct _PixelRow * dest,
                             unsigned char * cmap
                             );

/* FIXME cmap */
void 
initial_indexed_a_row       (
                             struct _PixelRow * src,
                             struct _PixelRow * dest,
                             struct _PixelRow * mask,
                             struct _Paint    * opacity,
                             unsigned char * cmap
                             );

void 
initial_inten_row           (
                             struct _PixelRow * src,
                             struct _PixelRow * dest,
                             struct _PixelRow * mask,
                             struct _Paint    * opacity
                             );

void 
initial_inten_a_row         (
                             struct _PixelRow * src,
                             struct _PixelRow * dest,
                             struct _PixelRow * mask,
                             struct _Paint    * opacity
                             );




void 
combine_indexed_and_indexed_row        (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity
                                        );

void 
combine_indexed_and_indexed_a_row      (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity
                                        );

void 
combine_indexed_a_and_indexed_a_row    (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity
                                        );

void 
combine_inten_a_and_indexed_a_row      (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity
                                        );

void 
combine_inten_and_inten_row            (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity
                                        );

void 
combine_inten_and_inten_a_row          (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity
                                        );

void 
combine_inten_a_and_inten_row          (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity,
                                        int mode_affect
                                        );

void 
combine_inten_a_and_inten_a_row        (
                                        struct _PixelRow * src1,
                                        struct _PixelRow * src2,
                                        struct _PixelRow * dest,
                                        struct _PixelRow * mask,
                                        struct _Paint    * opacity,
                                        int mode_affect
                                        );

void 
combine_inten_a_and_channel_mask_row   (
                                        struct _PixelRow * src,
                                        struct _PixelRow * channel,
                                        struct _PixelRow * dest,
                                        struct _Paint    * col,
                                        struct _Paint    * opacity
                                        );

void 
combine_inten_a_and_channel_selection_row  (
                                            struct _PixelRow * src,
                                            struct _PixelRow * channel,
                                            struct _PixelRow * dest,
                                            struct _Paint    * col,
                                            struct _Paint    * opacity
                                            );

void 
behind_inten_row          (
                           struct _PixelRow * src1,
                           struct _PixelRow * src2,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * opacity
                           );

void 
behind_indexed_row        (
                           struct _PixelRow * src1,
                           struct _PixelRow * src2,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * opacity
                           );

void 
replace_inten_row         (
                           struct _PixelRow * src1,
                           struct _PixelRow * src2,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * opacity
                           );

void 
replace_indexed_row       (
                           struct _PixelRow * src1,
                           struct _PixelRow * src2,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * opacity
                           );

void 
erase_inten_row           (
                           struct _PixelRow * src1,
                           struct _PixelRow * src2,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * opacity
                           );

void 
erase_indexed_row         (
                           struct _PixelRow * src1,
                           struct _PixelRow * src2,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * opacity
                           );

void 
extract_from_inten_row    (
                           struct _PixelRow * src,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * bg,
                           int cut
                           );

void 
extract_from_indexed_row  (
                           struct _PixelRow * src,
                           struct _PixelRow * dest,
                           struct _PixelRow * mask,
                           struct _Paint    * bg,
                           int cut
                           );

#endif  /*  __PAINT_FUNCS_ROW_H__  */
