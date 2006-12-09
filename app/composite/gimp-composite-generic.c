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
 *
 */

/*
 * This file is supposed to contain the generic (read: C) implementation
 * of the pixel fiddling paint-functions.
 */

#include "config.h"

#include <string.h>
#include <stdio.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-generic.h"


#define OPAQUE_OPACITY       255
#define TRANSPARENT_OPACITY  0

#define RANDOM_TABLE_SIZE    4096


#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

/* A drawable has an alphachannel if contains either 4 or 2 bytes data
 * aka GRAYA and RGBA and thus the macro below works. This will have
 * to change if we support bigger formats. We'll do it so for now because
 * masking is always cheaper than passing parameters over the stack.      */
/* FIXME: Move to a global place */

#define HAS_ALPHA(bytes) (~bytes & 1)


static guchar  add_lut[511];
static gint32  random_table[RANDOM_TABLE_SIZE];
static guchar  burn_lut[256][256];

/*
 *
 * Pixel format type conversion
 *
 * XXX This implementation will not work for >8 bit colours.
 * XXX This implementation is totally wrong.
 */
/**
 * gimp_composite_convert_any_any_any_generic:
 * @ctx:
 *
 *
 **/
void
gimp_composite_convert_any_any_any_generic (GimpCompositeContext *ctx)
{
  int i;
  int j;
  unsigned char *D = ctx->D;
  unsigned char *A = ctx->A;
  int bpp_A = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  int bpp_D = gimp_composite_pixel_bpp[ctx->pixelformat_D];

  for (i = 0; i < ctx->n_pixels; i++) {
    for (j = 0; j < bpp_A; j++) {
      D[j] = A[j];
    }
    D[j] = GIMP_COMPOSITE_ALPHA_OPAQUE;
    A += bpp_A;
    D += bpp_D;
  }
}

/**
 * gimp_composite_color_any_any_any_generic:
 * @dest:
 * @color:
 * @w:
 * @bytes:
 *
 *
 **/
void
gimp_composite_color_any_any_any_generic (guchar       *dest,
                                          const guchar *color,
                                          guint         w,
                                          guint         bytes)
{
  /* dest % bytes and color % bytes must be 0 or we will crash
     when bytes = 2 or 4.
     Is this safe to assume?  Lets find out.
     This is 4-7X as fast as the simple version.
   */

#if defined(sparc) || defined(__sparc__)
  guchar c0, c1, c2, c3;
#else
  guchar c0, c1, c2;
  guint32 *longd, longc;
  guint16 *shortd, shortc;
#endif

  switch (bytes)
    {
    case 1:
      memset(dest, *color, w);
      break;

    case 2:
#if defined(sparc) || defined(__sparc__)
      c0 = color[0];
      c1 = color[1];
      while (w--)
        {
          dest[0] = c0;
          dest[1] = c1;
          dest += 2;
        }
#else
      shortc = ((guint16 *) color)[0];
      shortd = (guint16 *) dest;
      while (w--)
        {
          *shortd = shortc;
          shortd++;
        }
#endif /* sparc || __sparc__ */
      break;

    case 3:
      c0 = color[0];
      c1 = color[1];
      c2 = color[2];
      while (w--)
        {
          dest[0] = c0;
          dest[1] = c1;
          dest[2] = c2;
          dest += 3;
        }
      break;

    case 4:
#if defined(sparc) || defined(__sparc__)
      c0 = color[0];
      c1 = color[1];
      c2 = color[2];
      c3 = color[3];
      while (w--)
        {
          dest[0] = c0;
          dest[1] = c1;
          dest[2] = c2;
          dest[3] = c3;
          dest += 4;
        }
#else
      longc = ((guint32 *) color)[0];
      longd = (guint32 *) dest;
      while (w--)
        {
          *longd = longc;
          longd++;
        }
#endif /* sparc || __sparc__ */
      break;

    default:
      while (w--)
        {
          memcpy(dest, color, bytes);
          dest += bytes;
        }
    }
}

/**
 * gimp_composite_blend_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a blend operation between sources ctx->A and ctx->B, using
 * the generalised algorithm: D = A * (255 - &beta;) + B * &beta;
 *
 * The result is left in ctx->D
 **/
void
gimp_composite_blend_any_any_any_generic (GimpCompositeContext *ctx)
{
  guchar *src1 = ctx->A;
  guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guchar blend = ctx->blend.blend;
  guint bytes = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint w = ctx->n_pixels;
  guint b;
  const guchar blend2 = (255 - blend);

  while (w--)
    {
      for (b = 0; b < bytes; b++)
        dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


#if 0
void
gimp_composite_shade_generic (const guchar *src, guchar *dest, const guchar *col, guchar blend, guint w, guint bytes, guint has_alpha)
{
  const guchar blend2 = (255 - blend);
  const guint alpha = (has_alpha) ? bytes - 1 : bytes;
  guint b;

  while (w--)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = (src[b] * blend2 + col[b] * blend) / 255;

      if (has_alpha)
        dest[alpha] = src[alpha];       /* alpha channel */

      src += bytes;
      dest += bytes;
    }
}
#endif

/**
 * gimp_composite_darken_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a darken operation between sources ctx->A and ctx->B, using
 * the generalised algorithm:
 * D_r = min(A_r, B_r);
 * D_g = min(A_g, B_g);
 * D_b = min(A_b, B_b);
 * D_a = min(A_a, B_a);
 *
 **/
void
gimp_composite_darken_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;
  guchar s1, s2;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          s1 = src1[b];
          s2 = src2[b];
          dest[b] = (s1 < s2) ? s1 : s2;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

/**
 * gimp_composite_lighten_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a lighten operation between sources ctx->A and ctx->B, using the
 * generalised algorithm:
 * D_r = max(A_r, B_r);
 * D_g = max(A_g, B_g);
 * D_b = max(A_b, B_b);
 * D_a = min(A_a, B_a);
 *
 **/
void
gimp_composite_lighten_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;
  guchar s1, s2;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          s1 = src1[b];
          s2 = src2[b];
          dest[b] = (s1 < s2) ? s2 : s1;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_hue_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a conversion to hue only of the source ctx->A using
 * the hue of ctx->B.
 **/
void
gimp_composite_hue_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  gint r1, g1, b1;
  gint r2, g2, b2;

  if (bytes1 > 2)
    {
      /*  assumes inputs are only 4 byte RGBA pixels  */
      while (length--)
        {
          r1 = src1[0];
          g1 = src1[1];
          b1 = src1[2];

          r2 = src2[0];
          g2 = src2[1];
          b2 = src2[2];

          gimp_rgb_to_hsv_int (&r1, &g1, &b1);
          gimp_rgb_to_hsv_int (&r2, &g2, &b2);

          /*  Composition should have no effect if saturation is zero.
           *  otherwise, black would be painted red (see bug #123296).
           */
          if (g2)
            r1 = r2;

          /*  set the destination  */
          gimp_hsv_to_rgb_int (&r1, &g1, &b1);

          dest[0] = r1;
          dest[1] = g1;
          dest[2] = b1;

          if (has_alpha1 && has_alpha2)
            dest[3] = MIN (src1[3], src2[3]);
          else if (has_alpha2)
            dest[3] = src2[3];

          src1 += bytes1;
          src2 += bytes2;
          dest += bytes2;
        }
    }
  else
    {
      ctx->D = ctx->B;
    }
}


/**
 * gimp_composite_saturation_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a conversion to saturation only of the source ctx->A using
 * the saturation level of ctx->B.
 *
 **/
void
gimp_composite_saturation_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  gint r1, g1, b1;
  gint r2, g2, b2;

  if (bytes1 > 2) {
    /*  assumes inputs are only 4 byte RGBA pixels  */
    while (length--)
      {
        r1 = src1[0];
        g1 = src1[1];
        b1 = src1[2];
        r2 = src2[0];
        g2 = src2[1];
        b2 = src2[2];
        gimp_rgb_to_hsv_int(&r1, &g1, &b1);
        gimp_rgb_to_hsv_int(&r2, &g2, &b2);

        g1 = g2;

        /*  set the destination  */
        gimp_hsv_to_rgb_int(&r1, &g1, &b1);

        dest[0] = r1;
        dest[1] = g1;
        dest[2] = b1;

        if (has_alpha1 && has_alpha2)
          dest[3] = MIN(src1[3], src2[3]);
        else if (has_alpha2)
          dest[3] = src2[3];

        src1 += bytes1;
        src2 += bytes2;
        dest += bytes2;
      }
  } else {
    ctx->D = ctx->B;
  }
}


/**
 * gimp_composite_value_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a conversion to value only of the source ctx->A using
 * the value of ctx->B.
 *
 **/
void
gimp_composite_value_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  gint r1, g1, b1;
  gint r2, g2, b2;

  if (bytes1 > 2) {
    /*  assumes inputs are only 4 byte RGBA pixels  */
    /*  assumes inputs are only 4 byte RGBA pixels  */
    while (length--)
      {
        r1 = src1[0];
        g1 = src1[1];
        b1 = src1[2];
        r2 = src2[0];
        g2 = src2[1];
        b2 = src2[2];
        gimp_rgb_to_hsv_int(&r1, &g1, &b1);
        gimp_rgb_to_hsv_int(&r2, &g2, &b2);

        b1 = b2;

        /*  set the destination  */
        gimp_hsv_to_rgb_int(&r1, &g1, &b1);

        dest[0] = r1;
        dest[1] = g1;
        dest[2] = b1;

        if (has_alpha1 && has_alpha2)
          dest[3] = MIN(src1[3], src2[3]);
        else if (has_alpha2)
          dest[3] = src2[3];

        src1 += bytes1;
        src2 += bytes2;
        dest += bytes2;
      }
  } else {
    ctx->D = ctx->B;
  }
}


/**
 * gimp_composite_color_only_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a conversion to of the source ctx->A using
 * the hue and saturation values of ctx->B.
 *
 **/
void
gimp_composite_color_only_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  gint r1, g1, b1;
  gint r2, g2, b2;

  if (bytes1 > 2) {
    /*  assumes inputs are only 4 byte RGBA pixels  */
    while (length--)
      {
        r1 = src1[0];
        g1 = src1[1];
        b1 = src1[2];
        r2 = src2[0];
        g2 = src2[1];
        b2 = src2[2];
        gimp_rgb_to_hsl_int(&r1, &g1, &b1);
        gimp_rgb_to_hsl_int(&r2, &g2, &b2);

        /*  transfer hue and saturation to the source pixel  */
        r1 = r2;
        g1 = g2;

        /*  set the destination  */
        gimp_hsl_to_rgb_int(&r1, &g1, &b1);

        dest[0] = r1;
        dest[1] = g1;
        dest[2] = b1;

        if (has_alpha1 && has_alpha2)
          dest[3] = MIN(src1[3], src2[3]);
        else if (has_alpha2)
          dest[3] = src2[3];

        src1 += bytes1;
        src2 += bytes2;
        dest += bytes2;
      }
  } else {
    ctx->D = ctx->B;
  }
}

/**
 * gimp_composite_behind_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform a behind operation to between the pixel sources ctx->A and ctx->B.
 *
 **/
void
gimp_composite_behind_any_any_any_generic (GimpCompositeContext * ctx)
{
  ctx->D = ctx->B;
  ctx->combine = gimp_composite_pixel_alphap[ctx->pixelformat_A]
    ? BEHIND_INTEN
    : NO_COMBINATION;
}

/**
 * gimp_composite_multiply_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] multiply operation between the pixel sources
 * ctx->A and ctx->B.
 *
 **/
void
gimp_composite_multiply_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, tmp;

  if (has_alpha1 && has_alpha2) {
    while (length--)
      {
        for (b = 0; b < alpha; b++)
          dest[b] = INT_MULT(src1[b], src2[b], tmp);

        dest[alpha] = MIN(src1[alpha], src2[alpha]);

        src1 += bytes1;
        src2 += bytes2;
        dest += bytes2;
      }
  } else if (has_alpha2) {
    while (length--)
      {
        for (b = 0; b < alpha; b++)
          dest[b] = INT_MULT(src1[b], src2[b], tmp);

        dest[alpha] = src2[alpha];

        src1 += bytes1;
        src2 += bytes2;
        dest += bytes2;
      }
  } else {
    while (length--)
      {
        for (b = 0; b < alpha; b++)
          dest[b] = INT_MULT(src1[b], src2[b], tmp);

        src1 += bytes1;
        src2 += bytes2;
        dest += bytes2;
      }
  }
}


/**
 * gimp_composite_divide_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] divide operation between the pixel sources ctx->A
 * and ctx->B.  ctx->A is the numerator, ctx->B the denominator.
 *
 **/
void
gimp_composite_divide_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, result;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          result = ((src1[b] * 256) / (1 + src2[b]));
          dest[b] = MIN(result, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_screen_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] screen operation between the pixel sources
 * ctx->A and ctx->B, using the generalised algorithm:
 *
 * D = 255 - (255 - A) * (255 - B)
 *
 **/
void
gimp_composite_screen_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, tmp;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_overlay_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] overlay operation between the pixel sources
 * ctx->A and ctx->B, using the generalised algorithm:
 *
 * D =  A * (A + (2 * B) * (255 - A))
 *
 **/
void
gimp_composite_overlay_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, tmp, tmpM;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          dest[b] = INT_MULT(src1[b], src1[b] + INT_MULT(2 * src2[b], 255 - src1[b], tmpM), tmp);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_dodge_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] dodge operation between the pixel sources
 * ctx->A and ctx->B, using the generalised algorithm:
 *
 * D = saturation of 255 or (A * 256) / (256 - B)
 *
 **/
void
gimp_composite_dodge_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, tmp;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          tmp = src1[b] << 8;
          tmp /= 256 - src2[b];
          dest[b] = (guchar) MIN (tmp, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_burn_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] dodge operation between the pixel sources
 * ctx->A and ctx->B, using the generalised algorithm:
 *
 * D = saturation of 255 or depletion of 0, of ((255 - A) * 256) / (B + 1)
 *
 **/
void
gimp_composite_burn_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = burn_lut[src1[b]][src2[b]];
      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_hardlight_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] hardlight operation between the pixel sources
 * ctx->A and ctx->B.
 *
 **/
void
gimp_composite_hardlight_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, tmp;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          if (src2[b] > 128)
            {
              tmp = ((gint) 255 - src1[b]) * ((gint) 255 - ((src2[b] - 128) << 1));
              dest[b] = (guchar) MIN (255 - (tmp >> 8), 255);
            }
          else
            {
              tmp = (gint) src1[b] * ((gint) src2[b] << 1);
              dest[b] = (guchar) MIN (tmp >> 8, 255);
            }
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_softlight_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] softlight operation between the pixel sources
 * ctx->A and ctx->B.
 *
 **/
void
gimp_composite_softlight_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = gimp_composite_pixel_alphap[ctx->pixelformat_A];
  const guint has_alpha2 = gimp_composite_pixel_alphap[ctx->pixelformat_B];
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b, tmpS, tmpM, tmp1, tmp2, tmp3;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          /* Mix multiply and screen */
          tmpM = INT_MULT(src1[b], src2[b], tmpM);
          tmpS = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp1);
          dest[b] = INT_MULT((255 - src1[b]), tmpM, tmp2) + INT_MULT(src1[b], tmpS, tmp3);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_grain_extract_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] grain-extract operation between the pixel sources
 * ctx->A and ctx->B.
 *
 **/
void
gimp_composite_grain_extract_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = gimp_composite_pixel_alphap[ctx->pixelformat_A];
  const guint has_alpha2 = gimp_composite_pixel_alphap[ctx->pixelformat_B];
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;
  gint diff;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          diff = src1[b] - src2[b] + 128;
          dest[b] = (guchar) CLAMP(diff, 0, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_grain_merge_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] grain-merge operation between the pixel sources
 * ctx->A and ctx->B.
 *
 **/
void
gimp_composite_grain_merge_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = HAS_ALPHA(bytes1);
  const guint has_alpha2 = HAS_ALPHA(bytes2);
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;
  gint sum;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          /* Add, re-center and clip. */
          sum = src1[b] + src2[b] - 128;
          dest[b] = (guchar) CLAMP(sum, 0, 255);
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}

/**
 * gimp_composite_addition_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] addition operation of the pixel sources ctx->A
 * and ctx->B.
 *
 **/
void
gimp_composite_addition_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = gimp_composite_pixel_alphap[ctx->pixelformat_A];
  const guint has_alpha2 = gimp_composite_pixel_alphap[ctx->pixelformat_B];
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;

  if (has_alpha1 && has_alpha2) {
    while (length--)
      {
        for (b = 0; b < alpha; b++)
          D[b] = add_lut[A[b] + B[b]];
        D[alpha] = MIN(A[alpha], B[alpha]);
        A += bytes1;
        B += bytes2;
        D += bytes2;
      }
  }  else if (has_alpha2) {
    while (length--)
      {
        for (b = 0; b < alpha; b++)
          D[b] = add_lut[A[b] + B[b]];
        D[alpha] = B[alpha];
        A += bytes1;
        B += bytes2;
        D += bytes2;
      }
  } else {
    while (length--)
      {
        for (b = 0; b < alpha; b++)
          D[b] = add_lut[A[b] + B[b]];
        A += bytes1;
        B += bytes2;
        D += bytes2;
      }
  }
}


/**
 * gimp_composite_subtract_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] subtract operation of the pixel source
 * ctx-B from ctx->A.
 *
 **/
void
gimp_composite_subtract_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = gimp_composite_pixel_alphap[ctx->pixelformat_A];
  const guint has_alpha2 = gimp_composite_pixel_alphap[ctx->pixelformat_B];
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;
  gint diff;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          diff = src1[b] - src2[b];
          dest[b] = (diff < 0) ? 0 : diff;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_difference_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] difference operation between the pixel sources
 * ctx->A and ctx->B.
 *
 **/
void
gimp_composite_difference_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  const guchar *src2 = ctx->B;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint bytes2 = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  const guint has_alpha1 = gimp_composite_pixel_alphap[ctx->pixelformat_A];
  const guint has_alpha2 = gimp_composite_pixel_alphap[ctx->pixelformat_B];
  const guint alpha = (has_alpha1 || has_alpha2) ? MAX(bytes1, bytes2) - 1 : bytes1;
  guint b;
  gint diff;

  while (length--)
    {
      for (b = 0; b < alpha; b++)
        {
          diff = src1[b] - src2[b];
          dest[b] = (diff < 0) ? -diff : diff;
        }

      if (has_alpha1 && has_alpha2)
        dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (has_alpha2)
        dest[alpha] = src2[alpha];

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes2;
    }
}


/**
 * gimp_composite_dissolve_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] alpha dissolve operation between the pixel
 * sources ctx->A and ctx->B.
 *
 **/
void
gimp_composite_dissolve_any_any_any_generic (GimpCompositeContext * ctx)
{
  GRand *gr;
  gint alpha;
  gint b;
  gint combined_opacity;
  gint db;
  gint length = ctx->n_pixels;
  gint opacity = ctx->dissolve.opacity;
  gint sb = gimp_composite_pixel_bpp[ctx->pixelformat_B];
  gint x = ctx->dissolve.x;
  gint y = ctx->dissolve.y;
  const guchar *mask = ctx->M;
  gint32 rand_val;
  guchar *dest = ctx->D;
  guchar *src = ctx->B;
  guint has_alpha = gimp_composite_pixel_alphap[ctx->pixelformat_B];

  /*
   * if destination does not have an alpha channel, add one to it.
   */

  if (!gimp_composite_pixel_alphap[ctx->pixelformat_D]) {
    ctx->pixelformat_D = gimp_composite_pixel_alpha[ctx->pixelformat_D];
  }
  db = gimp_composite_pixel_bpp[ctx->pixelformat_D];

  gr = g_rand_new_with_seed(random_table[y % RANDOM_TABLE_SIZE]);

  for (b = 0; b < x; b ++)
    g_rand_int (gr);

  alpha = db - 1;

  /*
   * XXX NB: The mask is assumed to be a linear array of bytes, no
   * accounting for the mask being of a particular pixel format.
   */
  while (length--)
    {
      /*  preserve the intensity values  */
      for (b = 0; b < alpha; b++)
        dest[b] = src[b];

      /*  dissolve if random value is >= opacity  */
      rand_val = g_rand_int_range(gr, 0, 255);

      if (mask) {
        if (has_alpha)
          combined_opacity = opacity * src[alpha] * (*mask) / (255 * 255);
        else
          combined_opacity = opacity * (*mask) / 255;

        mask++;
      } else {
        if (has_alpha)
          combined_opacity = opacity * src[alpha] / 255;
        else
          combined_opacity = opacity;
      }

      dest[alpha] =
        (rand_val >= combined_opacity) ? TRANSPARENT_OPACITY : OPAQUE_OPACITY;

      dest += db;
      src += sb;
    }

  g_rand_free(gr);

  ctx->combine = gimp_composite_pixel_alphap[ctx->pixelformat_A] ? COMBINE_INTEN_A_INTEN_A : COMBINE_INTEN_INTEN_A;
}

/**
 * gimp_composite_replace_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] replace operation of the pixel
 * source ctx->A with the ctx->B.
 *
 **/
void
gimp_composite_replace_any_any_any_generic (GimpCompositeContext *ctx)
{
  ctx->D = ctx->B;
  ctx->combine = REPLACE_INTEN;
}


/**
 * gimp_composite_swap_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Swap the contents of ctx->A and ctx->B.
 *
 **/
void
gimp_composite_swap_any_any_any_generic (GimpCompositeContext * ctx)
{
  guchar *src = ctx->A;
  guchar *dest = ctx->B;
  guint bytes1 = gimp_composite_pixel_bpp[ctx->pixelformat_A];
  guint length = ctx->n_pixels * bytes1;

  while (length--)
    {
      guchar tmp = *dest;

      *dest = *src;
      *src = tmp;

      src++;
      dest++;
    }
}

/**
 * gimp_composite_normal_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] over operation of the pixel
 * source ctx->B on ctx->A.
 *
 **/
void
gimp_composite_normal_any_any_any_generic (GimpCompositeContext * ctx)
{
  ctx->D = ctx->B;
}


/**
 * gimp_composite_erase_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] "erase" operation of the pixel
 * source ctx->A using the alpha information in ctx->B.
 *
 **/
void
gimp_composite_erase_any_any_any_generic (GimpCompositeContext *ctx)
{
  ctx->D = ctx->B;
  ctx->combine = (gimp_composite_pixel_alphap[ctx->pixelformat_A] && gimp_composite_pixel_alphap[ctx->pixelformat_B]) ? ERASE_INTEN : 0;
}

/**
 * gimp_composite_anti_erase_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] "anti-erase" operation of the pixel
 * source ctx->A using the alpha information in ctx->B.
 *
 **/
void
gimp_composite_anti_erase_any_any_any_generic (GimpCompositeContext *ctx)
{
  ctx->D = ctx->B;
  ctx->combine = (gimp_composite_pixel_alphap[ctx->pixelformat_A] && gimp_composite_pixel_alphap[ctx->pixelformat_B]) ? ANTI_ERASE_INTEN : 0;
}

/**
 * gimp_composite_color_erase_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] "color-erase" operation of the pixel
 * source ctx->A using the alpha information in ctx->B.
 *
 **/
void
gimp_composite_color_erase_any_any_any_generic (GimpCompositeContext *ctx)
{
  ctx->D = ctx->B;
  ctx->combine = (gimp_composite_pixel_alphap[ctx->pixelformat_A] && gimp_composite_pixel_alphap[ctx->pixelformat_B])
    ? COLOR_ERASE_INTEN
    : 0;
}


/**
 * gimp_composite_scale_any_any_any_generic:
 * @ctx: The compositing context.
 *
 * Perform an RGB[A] scale operation of the pixel source ctx->A using
 * the scale coefficient on the compositing context, @ctx.
 *
 **/
void
gimp_composite_scale_any_any_any_generic (GimpCompositeContext * ctx)
{
  const guchar *src1 = ctx->A;
  guchar *dest = ctx->D;
  guint length = ctx->n_pixels;
  guint bytes1 = (ctx->pixelformat_A == GIMP_PIXELFORMAT_V8) ? 1
    : (ctx->pixelformat_A == GIMP_PIXELFORMAT_VA8) ? 2
    : (ctx->pixelformat_A == GIMP_PIXELFORMAT_RGB8) ? 3 : (ctx->pixelformat_A == GIMP_PIXELFORMAT_RGBA8) ? 4 : 0;
  gint tmp;

  length = ctx->n_pixels * bytes1;

  while (length--)
    {
      *dest++ = (guchar) INT_MULT(*src1, ctx->scale.scale, tmp);
      src1++;
    }
}

/**
 * gimp_composite_generic_init:
 *
 * Initialise the generic set of compositing functions.
 *
 * Returns: boolean indicating that the initialisation was successful.
 **/
gboolean
gimp_composite_generic_init (void)
{
  GRand *gr;
  guint  i;
  gint   a, b;

#define RANDOM_SEED 314159265

  /*  generate a table of random seeds  */
  gr = g_rand_new_with_seed (RANDOM_SEED);

  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = g_rand_int (gr);

  g_rand_free (gr);

  /*  generate a table for burn compositing */
  for (a = 0; a < 256; a++)
    for (b = 0; b < 256; b++)
      {
        /* FIXME: Is the burn effect supposed to be dependant on the sign
         * of this temporary variable?
         */
        gint tmp;

        tmp = (255 - a) << 8;
        tmp /= b + 1;
        tmp = (255 - tmp);

        burn_lut[a][b] = CLAMP (tmp, 0, 255);
      }

  /*  generate a table for saturate add */
  for (i = 0; i < 256; i++)
    add_lut[i] = i;

  for (i = 256; i <= 510; i++)
    add_lut[i] = 255;

  return TRUE;
}
