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

/* FIXME: Needs a bigger overhaul. Maybe inline assembly would be better? 
 * 'bigger overhaul' indeed, but let's not talk about assembly language now (Nov-03-2002)
 * This code is being disabled to address Bug #86290; point your favorite browser to
 * http://bugzilla.gnome.org/show_bug.cgi?id=86290 for the reason this is being done.
 * Presuming you've read the "Additional Comments" there, and really, really, think that
 * Gimp ought to have this MMX code, RIGHT NOW (!) then this is what I believe you have to do:
 * 1.  Be aware that the MMX code is incomplete; the macro USE_MMX_PIXEL_OP_3A_1A has been
 *     written the way it has so that the MMX code is invoked only on region depths it knows
 *     how to handle. In that case, the MMX code executes and the macro performs a return to
 *     caller. Otherwise, it DOES NOT DO ANYTHING, and the macro ought to be followed by 
 *     code that had better do something. Check out an archival app/paint_funcs/paint_funcs.c
 *     say, CVS version 1.98 (Nov 2, 2001) and study how USE_MMX_PIXEL_OP_3A_1A was used there;
 *     you'll discover that it must have some kind of alternate code following in case program
 *     flow does not return to the caller inside the macro. 
 * 2a. For this reason, you have to pretty much undo the separation of 'generic C' and 'mmx' code at
 *     at the heart of Nov-18-2001, Nov-19-2001 check-ins (See Daniel Egger's comments in 
 *     ChangeLog). That, or replicate the 'generic C' in the various *_mmx functions implemented
 *     in this header file, below. Not only would that make the separation effort rather pointless, but
 *     the replication of functionality creates a maintenance problem.
 * 2b. Alternatively, you can finish the MMX code so that it handles all cases that the generic
 *     C does. This is ideal, and makes sensible the separation effort, for then  USE_MMX_PIXEL_OP_3A_1A
 *     need not have any additional implementation following. For that matter, you'd likely not 
 *     even need the macro. You'd also be a better man than I am, Gunga Din.
 * Whatever you do, proceed carefully and keep fresh batteries in your electric torch: there be monsters here.
 *                                                                            grosgood@rcn.com
 */

#ifdef HAVE_ASM_MMX
#if GIMP_ENABLE_MMX

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

#endif  /* GIMP_ENABLE_MMX        */ 
#endif  /* HAVE_ASM_MMX           */
#endif  /* __PAINT_FUNCS_MMX_H__  */
 
