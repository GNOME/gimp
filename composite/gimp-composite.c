/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gimp image compositing
 * Copyright (C) 2003  Helvetix Victorinox, a pseudonym, <helvetix@gimp.org>
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
 * $Id$
 */
#include <stdio.h>

#include "gimp-composite.h"

/*
 * Details about pixel formats, bits-per-pixel alpha and non alpha
 * versions of pixel formats.
 */
/*
 * Report on the number of bytes a particular pixel format consumes per pixel.
 */
unsigned char gimp_composite_pixel_bpp[] = {
  1, /* GIMP_PIXELFORMAT_V8      */
  2, /* GIMP_PIXELFORMAT_VA8     */
  3, /* GIMP_PIXELFORMAT_RGB8    */
  4, /* GIMP_PIXELFORMAT_RGBA8   */
#if GIMP_16BITCOLOR
  2, /* GIMP_PIXELFORMAT_V16     */
  4, /* GIMP_PIXELFORMAT_VA16    */
  6, /* GIMP_PIXELFORMAT_RGB16   */
  8, /* GIMP_PIXELFORMAT_RGBA16  */
#endif
  0, /* GIMP_PIXELFORMAT_ANY */
};

char *gimp_composite_pixel_name[] = {
  "GIMP_PIXELFORMAT_V8",
  "GIMP_PIXELFORMAT_VA8",
  "GIMP_PIXELFORMAT_RGB8",
  "GIMP_PIXELFORMAT_RGBA8",
#if GIMP_16BITCOLOR
  "GIMP_PIXELFORMAT_V16",
  "GIMP_PIXELFORMAT_VA16",
  "GIMP_PIXELFORMAT_RGB16 ",
  "GIMP_PIXELFORMAT_RGBA16 ",
#endif
  "GIMP_PIXELFORMAT_ANY",
};
/*
 * Report true (non-zero) if a pixel format has alpha.
 */
unsigned char gimp_composite_pixel_alphap[] = {
  0, /* GIMP_PIXELFORMAT_V8      */
  1, /* GIMP_PIXELFORMAT_VA8     */
  0, /* GIMP_PIXELFORMAT_RGB8    */
  1, /* GIMP_PIXELFORMAT_RGBA8   */
#if GIMP_16BITCOLOR
  0, /* GIMP_PIXELFORMAT_V16     */
  1, /* GIMP_PIXELFORMAT_VA16    */
  0, /* GIMP_PIXELFORMAT_RGB16   */
  1, /* GIMP_PIXELFORMAT_RGBA16  */
#endif
  0, /* GIMP_PIXELFORMAT_UNKNOWN */
};

/*
 * Convert to/from pixel formats with/without alpha.
 */
GimpPixelFormat gimp_composite_pixel_alpha[] = {
  GIMP_PIXELFORMAT_VA8,         /* GIMP_PIXELFORMAT_V8      */
  GIMP_PIXELFORMAT_V8,          /* GIMP_PIXELFORMAT_VA8     */
  GIMP_PIXELFORMAT_RGBA8,       /* GIMP_PIXELFORMAT_RGB8    */
  GIMP_PIXELFORMAT_RGB8,        /* GIMP_PIXELFORMAT_RGBA8   */
#if GIMP_16BITCOLOR
  GIMP_PIXELFORMAT_VA16,
  GIMP_PIXELFORMAT_V16,
  GIMP_PIXELFORMAT_RGBA16,
  GIMP_PIXELFORMAT_RGB16
#endif
  GIMP_PIXELFORMAT_ANY,         /* GIMP_PIXELFORMAT_ANY */
};


/*
 * XXX I don't like to put this here.  I think this information,
 * specific to the functions, ought to be with the function.
 */
struct GimpCompositeOperationEffects gimp_composite_operation_effects[] = {
  { TRUE,  TRUE,  FALSE, },     /*  GIMP_NORMAL_MODE        */
  { TRUE,  TRUE,  FALSE, },     /*  GIMP_DISSOLVE_MODE      */
  { TRUE,  TRUE,  FALSE, },     /*  GIMP_BEHIND_MODE        */
  { FALSE, FALSE, FALSE, },     /*  GIMP_MULTIPLY_MODE      */
  { FALSE, FALSE, FALSE, },     /*  GIMP_SCREEN_MODE        */
  { FALSE, FALSE, FALSE, },     /*  GIMP_OVERLAY_MODE       */
  { FALSE, FALSE, FALSE, },     /*  GIMP_DIFFERENCE_MODE    */
  { FALSE, FALSE, FALSE, },     /*  GIMP_ADDITION_MODE      */
  { FALSE, FALSE, FALSE, },     /*  GIMP_SUBTRACT_MODE      */
  { FALSE, FALSE, FALSE, },     /*  GIMP_DARKEN_ONLY_MODE   */
  { FALSE, FALSE, FALSE, },     /*  GIMP_LIGHTEN_ONLY_MODE  */
  { FALSE, FALSE, FALSE, },     /*  GIMP_HUE_MODE           */
  { FALSE, FALSE, FALSE, },     /*  GIMP_SATURATION_MODE    */
  { FALSE, FALSE, FALSE, },     /*  GIMP_COLOR_MODE         */
  { FALSE, FALSE, FALSE, },     /*  GIMP_VALUE_MODE         */
  { FALSE, FALSE, FALSE, },     /*  GIMP_DIVIDE_MODE        */
  { FALSE, FALSE, FALSE, },     /*  GIMP_DODGE_MODE         */
  { FALSE, FALSE, FALSE, },     /*  GIMP_BURN_MODE          */
  { FALSE, FALSE, FALSE, },     /*  GIMP_HARDLIGHT_MODE     */
  { FALSE, FALSE, FALSE, },     /*  GIMP_SOFTLIGHT_MODE     */
  { FALSE, FALSE, FALSE, },     /*  GIMP_GRAIN_EXTRACT_MODE */
  { FALSE, FALSE, FALSE, },     /*  GIMP_GRAIN_MERGE_MODE   */
  { TRUE,  FALSE, TRUE,  },     /*  GIMP_COLOR_ERASE_MODE   */
  { TRUE,  FALSE, TRUE,  },     /*  GIMP_ERASE_MODE         */
  { TRUE,  TRUE,  TRUE,  },     /*  GIMP_REPLACE_MODE       */
  { TRUE,  TRUE,  FALSE, },     /*  GIMP_ANTI_ERASE_MODE    */

  { FALSE, FALSE, FALSE },      /*  GIMP_SWAP */
  { FALSE, FALSE, FALSE },      /*  GIMP_SCALE */
  { FALSE, FALSE, FALSE },      /*  GIMP_CONVERT */
};

void
gimp_composite_unsupported(GimpCompositeContext *ctx)
{
  printf("compositing function %d unsupported\n", ctx->op);
}

struct {
  char announce_function;
} gimp_composite_debug;

#include "gimp-composite-dispatch.c"

void
gimp_composite_dispatch(GimpCompositeContext *ctx)
{
  void (*function)();

  function = gimp_composite_function[ctx->op][ctx->pixelformat_A][ctx->pixelformat_B][ctx->pixelformat_D];

  if (function)
    (*function)(ctx);
  else {
    printf("unsupported composite operation %d %d %d (see gimp-composite.h)\n", ctx->op, ctx->pixelformat_A, ctx->pixelformat_B);
  }
}

void
gimp_composite_context_print(GimpCompositeContext *ctx)
{
  printf("%p: %s op=%d A=%s(%d):%p B=%s(%d):%p D=%s(%d):%p M=%s(%d):%p n_pixels=%lu\n",
         ctx,
         gimp_composite_function_name[ctx->op][ctx->pixelformat_A][ctx->pixelformat_B][ctx->pixelformat_D],
         ctx->op,
         gimp_composite_pixel_name[ctx->pixelformat_A], ctx->pixelformat_A, ctx->A, 
         gimp_composite_pixel_name[ctx->pixelformat_B], ctx->pixelformat_B, ctx->A, 
         gimp_composite_pixel_name[ctx->pixelformat_D], ctx->pixelformat_D, ctx->A, 
         gimp_composite_pixel_name[ctx->pixelformat_M], ctx->pixelformat_M, ctx->A, 
         ctx->n_pixels);
}
