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

#ifndef __PAINT_FUNCS_AREA_H__
#define __PAINT_FUNCS_AREA_H__

#include "tag.h"

/* Forward declarations */
struct _PixelArea;
struct _Paint;


/*  The types of convolutions  */
#define NORMAL     0   /*  Negative numbers truncated  */
#define ABSOLUTE   1   /*  Absolute value              */
#define NEGATIVE   2   /*  add 127 to values           */

/*  The types of thinning  */
#define SHRINK_REGION 0
#define GROW_REGION   1

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
#define ERASE_MODE         20
#define REPLACE_MODE       21


void              paint_funcs_area_setup       (void);
void              paint_funcs_area_free        (void);
unsigned char *   paint_funcs_area_get_buffer  (int);
void              paint_funcs_area_randomize   (int);


void  color_area              (struct _PixelArea *, struct _Paint *);

void  blend_area              (struct _PixelArea *, struct _PixelArea *,
                               struct _PixelArea *, gfloat);

void  shade_area              (struct _PixelArea *, struct _PixelArea *,
                               struct _Paint *, gfloat);

void  copy_area               (struct _PixelArea *, struct _PixelArea *);

void  add_alpha_area          (struct _PixelArea *, struct _PixelArea *);

void  flatten_area            (struct _PixelArea *, struct _PixelArea *,
                               struct _Paint *);

void  extract_alpha_area      (struct _PixelArea *, struct _PixelArea *,
                               struct _PixelArea *);

void  extract_from_area       (struct _PixelArea *, struct _PixelArea *,
                               struct _PixelArea *, struct _Paint *, int);

void  convolve_area           (struct _PixelArea *, struct _PixelArea *,
                               int *, int, int, int);

void  multiply_alpha_area     (struct _PixelArea *);

void  separate_alpha_area     (struct _PixelArea *);

void  gaussian_blur_area      (struct _PixelArea *, double);

void  border_area             (struct _PixelArea *, void *, int, int);

void  scale_area              (struct _PixelArea *, struct _PixelArea *);

void  scale_area_no_resample  (struct _PixelArea *, struct _PixelArea *);

void  subsample_area          (struct _PixelArea *, struct _PixelArea *, int);

float shapeburst_area         (struct _PixelArea *, struct _PixelArea *);

int   thin_area               (struct _PixelArea *, int);

void  swap_area               (struct _PixelArea *, struct _PixelArea *);


void  apply_mask_to_area      (struct _PixelArea *, struct _PixelArea *, struct _Paint *);

void  combine_mask_and_area   (struct _PixelArea *, struct _PixelArea *, struct _Paint *);

void  copy_gray_to_area       (struct _PixelArea *, struct _PixelArea *);

/* FIXME args */
void  initial_area            (struct _PixelArea *, struct _PixelArea *,
                               struct _PixelArea *, unsigned char *,
                               int, int, int *, int);

/* FIXME args */
void  combine_areas           (struct _PixelArea *, struct _PixelArea *,
                               struct _PixelArea *, struct _PixelArea *,
                               unsigned char *, int,
                               int, int *, int);

/* FIXME args */
void  combine_areas_replace   (struct _PixelArea *, struct _PixelArea *,
                               struct _PixelArea *, struct _PixelArea *,
                               unsigned char *,
                               int, int *, int);


#endif  /*  __PAINT_FUNCS_AREA_H__  */
