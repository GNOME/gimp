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
 * This file is supposed to contain the MMX implementation  of the 
 * pixelfiddeling paint-functions. 
 */

#ifndef __PAINT_FUNCS_MMX_H__
#define __PAINT_FUNCS_MMX_H__

/* FIXME: Needs a bigger overhaul. Maybe inline assembly would be better? */ 
#ifdef HAVE_ASM_MMX

#define MMX_PIXEL_OP(x) \
void \
x( \
  const guchar *src1, \
  const guchar *src2, \
  guint count, \
  guchar *dst) __attribute((regparm(3)));

#define MMX_PIXEL_OP_3A_1A(op) \
  MMX_PIXEL_OP(op##_pixels_3a_3a) \
  MMX_PIXEL_OP(op##_pixels_1a_1a)

#define USE_MMX_PIXEL_OP_3A_1A(op) \
  if (HAS_ALPHA (alms->bytes1) && HAS_ALPHA (alms->bytes2)) \
    { \
      if (alms->bytes1==2 && alms->bytes2==2) \
	return op##_pixels_1a_1a(alms->src1, alms->src2, alms->length, *(alms->dest)); \
      if (alms->bytes1==4 && alms->bytes2==4) \
	return op##_pixels_3a_3a(alms->src1, alms->src2, alms->length, *(alms->dest)); \
    } 

MMX_PIXEL_OP_3A_1A(multiply);
static void
layer_multiply_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(multiply);
}

MMX_PIXEL_OP_3A_1A(screen);
static void
layer_screen_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(screen);
}

MMX_PIXEL_OP_3A_1A(overlay);
static void
layer_overlay_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(overlay);
}

MMX_PIXEL_OP_3A_1A(difference);
static void
layer_difference_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(difference);
}

MMX_PIXEL_OP_3A_1A(add);
static void
layer_addition_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(add);
}

MMX_PIXEL_OP_3A_1A(substract);
static void
layer_subtract_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(substract);
}

MMX_PIXEL_OP_3A_1A(darken);
static void
layer_darken_only_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(darken);
}

MMX_PIXEL_OP_3A_1A(lighten);
static void
layer_lighten_only_mode_mmx (struct apply_layer_mode_struct *alms)
{
  USE_MMX_PIXEL_OP_3A_1A(lighten);
}

#endif /* HAVE_ASM_MMX */
#endif  /*  __PAINT_FUNCS_MMX_H__  */
