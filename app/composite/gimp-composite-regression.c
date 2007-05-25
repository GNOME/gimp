/* GIMP - The GNU Image Manipulation Program
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#ifndef timersub
/*
 * Linux <sys/time.h> has a handy macro for finding the difference between
 * two timers.  This is lifted directly from <sys/time.h> on a GLIBC 2.2.x
 * system.
 */
#define timersub(a, b, result) \
  do {                                               \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
    if ((result)->tv_usec < 0)                       \
      {                                              \
        --(result)->tv_sec;                          \
        (result)->tv_usec += 1000000;                \
      }                                              \
  } while (0)
#endif


#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-regression.h"
#include "gimp-composite-util.h"
#include "gimp-composite-generic.h"

/**
 * gimp_composite_regression_print_vector:
 * @vector:
 * @format:
 * @n_pixels:
 *
 *
 **/
void
gimp_composite_regression_print_vector (guchar vector[], GimpPixelFormat format, gulong n_pixels)
{
                switch (format)
                                {
                                case GIMP_PIXELFORMAT_V8:
                                                gimp_composite_regression_print_vector_v8 ((gimp_v8_t *) vector, n_pixels);
                                                break;

                                case GIMP_PIXELFORMAT_VA8:
                                                gimp_composite_regression_print_vector_va8 ((gimp_va8_t *) vector, n_pixels);
                                                break;

                                case GIMP_PIXELFORMAT_RGB8:
                                                gimp_composite_regression_print_vector_rgb8 ((gimp_rgb8_t *) vector, n_pixels);
                                                break;

                                case GIMP_PIXELFORMAT_RGBA8:
                                                gimp_composite_regression_print_vector_rgba8 ((gimp_rgba8_t *) vector, n_pixels);
                                                break;

                                case GIMP_PIXELFORMAT_ANY:
                                case GIMP_PIXELFORMAT_N:
                                default:
                                                break;
                                }
}


/**
 * gimp_composite_regression_print_vector_v8:
 * @v:
 * @n_pixels:
 *
 *
 **/
void
gimp_composite_regression_print_vector_v8 (gimp_v8_t v[], unsigned int n_pixels)
{
                unsigned int i;

                for (i = 0; i < n_pixels; i++)
                                {
                                                g_print ("#%02x\n", v[i].v);
                                }
}

/**
 * gimp_composite_regression_print_vector_va8:
 * @v:
 * @n_pixels:
 *
 *
 **/
void
gimp_composite_regression_print_vector_va8 (gimp_va8_t v[], unsigned int n_pixels)
{
                unsigned int i;

                for (i = 0; i < n_pixels; i++)
                                {
                                                g_print ("#%02x,%02X\n", v[i].v, v[i].a);
                                }
}

/**
 * gimp_composite_regression_print_vector_rgb8:
 * @rgb8:
 * @n_pixels:
 *
 *
 **/
void
gimp_composite_regression_print_vector_rgb8 (gimp_rgb8_t v[], unsigned int n_pixels)
{
                unsigned int i;

                for (i = 0; i < n_pixels; i++)
                                {
                                                g_print ("#%02x%02x%02x\n", v[i].r, v[i].g, v[i].b);
                                }
}

/**
 * gimp_composite_regression_print_vector_rgba8:
 * @v:
 * @n_pixels:
 *
 *
 **/
void
gimp_composite_regression_print_vector_rgba8 (gimp_rgba8_t v[], unsigned int n_pixels)
{
                unsigned int i;

                for (i = 0; i < n_pixels; i++)
                                {
                                                g_print ("#%02x%02x%02x,%02X\n", v[i].r, v[i].g, v[i].b, v[i].a);
                                }
}

/**
 * gimp_composite_regression_print_rgba8:
 * @rgba8:
 *
 *
 **/
void
gimp_composite_regression_print_rgba8 (gimp_rgba8_t *rgba8)
{
  g_print ("#%02x%02x%02x,%02X", rgba8->r, rgba8->g, rgba8->b, rgba8->a);
}

/**
 * gimp_composite_regression_print_va8:
 * @va8:
 *
 *
 **/
void
gimp_composite_regression_print_va8 (gimp_va8_t *va8)
{
  g_print ("#%02x,%02X", va8->v, va8->a);
}

/**
 * gimp_composite_regression_compare_contexts:
 * @operation:
 * @ctx1:
 * @ctx2:
 *
 *
 *
 * Return value:
 **/
int
gimp_composite_regression_compare_contexts (char *operation, GimpCompositeContext *ctx1, GimpCompositeContext *ctx2)
{
  switch (ctx1->pixelformat_D) {
  case GIMP_PIXELFORMAT_ANY:
  case GIMP_PIXELFORMAT_N:
  default:
    g_print("Bad pixelformat! %d\n", ctx1->pixelformat_A);
    exit(1);
    break;

  case GIMP_PIXELFORMAT_V8:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_VA8:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_RGB8:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_RGBA8:
    gimp_composite_regression_comp_rgba8(operation, (gimp_rgba8_t *) ctx1->A, (gimp_rgba8_t *) ctx1->B, (gimp_rgba8_t *) ctx1->D, (gimp_rgba8_t *) ctx2->D, ctx1->n_pixels);
    break;

#if GIMP_COMPOSITE_16BIT
  case GIMP_PIXELFORMAT_V16:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_VA16:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_RGB16:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_RGBA16:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

#endif
#if GIMP_COMPOSITE_32BIT
  case GIMP_PIXELFORMAT_V32:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_VA32:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_RGB32:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

  case GIMP_PIXELFORMAT_RGBA32:
    if (memcmp(ctx1->D, ctx2->D, ctx1->n_pixels * gimp_composite_pixel_bpp[ctx1->pixelformat_D])) {
      g_print("%s: failed to agree\n", operation);
      return (1);
    }
    break;

#endif
  }

  return (0);
}


/**
 * gimp_composite_regression_comp_rgba8:
 * @str:
 * @rgba8A:
 * @rgba8B:
 * @expected:
 * @actual:
 * @length:
 *
 *
 *
 * Return value:
 **/
int
gimp_composite_regression_comp_rgba8 (char *str, gimp_rgba8_t *rgba8A, gimp_rgba8_t *rgba8B, gimp_rgba8_t *expected, gimp_rgba8_t *actual, gulong length)
{
  gulong i;
  int failed;
  int fail_count;

  fail_count = 0;

  for (i = 0; i < length; i++) {
    failed = 0;

    if (expected[i].r != actual[i].r) { failed = 1; }
    if (expected[i].g != actual[i].g) { failed = 1; }
    if (expected[i].b != actual[i].b) { failed = 1; }
    if (expected[i].a != actual[i].a) { failed = 1; }
    if (failed) {
      fail_count++;
      g_print("%s %8lu A=", str, i); gimp_composite_regression_print_rgba8(&rgba8A[i]);
      if (rgba8B != (gimp_rgba8_t *) 0) {
        g_print(" B="); gimp_composite_regression_print_rgba8(&rgba8B[i]);
      }
      g_print("   expected=");
      gimp_composite_regression_print_rgba8(&expected[i]);
      g_print(" actual=");
      gimp_composite_regression_print_rgba8(&actual[i]);
      g_print("\n");
    }
    if (fail_count > 5)
      break;
  }

  return (fail_count);
}

/**
 * gimp_composite_regression_comp_va8:
 * @str:
 * @va8A:
 * @va8B:
 * @expected:
 * @actual:
 * @length:
 *
 *
 *
 * Return value:
 **/
int
gimp_composite_regression_comp_va8 (char *str, gimp_va8_t *va8A, gimp_va8_t *va8B, gimp_va8_t *expected, gimp_va8_t *actual, gulong length)
{
  int i;
  int failed;
  int fail_count;

  fail_count = 0;

  for (i = 0; i < length; i++) {
    failed = 0;

    if (expected[i].v != actual[i].v) { failed = 1; }
    if (expected[i].a != actual[i].a) { failed = 1; }
    if (failed) {
      fail_count++;
      g_print("%s %8d A=", str, i); gimp_composite_regression_print_va8(&va8A[i]);
      if (va8B != (gimp_va8_t *) 0) { g_print(" B="); gimp_composite_regression_print_va8(&va8B[i]); }
      g_print("   ");
      g_print("expected=");
      gimp_composite_regression_print_va8(&expected[i]);
      g_print(" actual=");
      gimp_composite_regression_print_va8(&actual[i]);
      g_print("\n");
    }
    if (fail_count > 5)
      break;
  }

  return (fail_count);
}

/**
 * gimp_composite_regression_dump_rgba8:
 * @str:
 * @rgba:
 * @n_pixels:
 *
 *
 **/
void
gimp_composite_regression_dump_rgba8 (char *str, gimp_rgba8_t *rgba, gulong n_pixels)
{
  int i;

  g_print("%s\n", str);

  for (i = 0; i < n_pixels; i++) {
    g_print("%5d: ", i);
    gimp_composite_regression_print_rgba8(&rgba[i]);
    g_print("\n");
  }
}

#define tv_to_secs(tv) ((double) ((tv).tv_sec) + (double) ((tv).tv_usec / 1000000.0))

#define timer_report(name,t1,t2) g_print("%-32s %10.4f %10.4f %10.4f%c\n", name, tv_to_secs(t1), tv_to_secs(t2), tv_to_secs(t1)/tv_to_secs(t2), tv_to_secs(t1)/tv_to_secs(t2) > 1.0 ? ' ' : '*');

/**
 * gimp_composite_regression_timer_report:
 * @name:
 * @t1:
 * @t2:
 *
 *
 **/
void
gimp_composite_regression_timer_report (char *name, double t1, double t2)
{
  g_print ("%-32s %10.4f %10.4f %10.4f%c\n", name, t1, t2, t1/t2, t1/t2 > 1.0 ? ' ' : '*');
}

/**
 * gimp_composite_regression_time_function:
 * @iterations:
 * @func:
 *
 *
 *
 * Return value:
 **/
double
gimp_composite_regression_time_function (gulong iterations, void (*func)(), GimpCompositeContext *ctx)
{
  struct timeval t0;
  struct timeval t1;
  struct timeval tv_elapsed;
  gulong i;

  gettimeofday (&t0, NULL);
  for (i = 0; i < iterations; i++) { (*func)(ctx); }
  gettimeofday (&t1, NULL);
  timersub (&t1, &t0, &tv_elapsed);

  return (tv_to_secs (tv_elapsed));
}

/**
 * gimp_composite_regression_random_rgba8:
 * @n_pixels:
 *
 *
 *
 * Return value:
 **/
gimp_rgba8_t *
gimp_composite_regression_random_rgba8 (gulong n_pixels)
{
  gimp_rgba8_t *rgba8;
  gulong i;

  if ((rgba8 = (gimp_rgba8_t *) calloc (sizeof(gimp_rgba8_t), n_pixels))) {
    for (i = 0; i < n_pixels; i++) {
      rgba8[i].r = rand() % 256;
      rgba8[i].g = rand() % 256;
      rgba8[i].b = rand() % 256;
      rgba8[i].a = rand() % 256;
    }
  }

  return (rgba8);
}

/**
 * gimp_composite_regression_fixed_rgba8:
 * @n_pixels:
 *
 *
 *
 * Return value:
 **/
gimp_rgba8_t *
gimp_composite_regression_fixed_rgba8 (gulong n_pixels)
{
  gimp_rgba8_t *rgba8;
  gulong i;
  gulong v;

  if ((rgba8 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels))) {
    for (i = 0; i < n_pixels; i++) {
      v = i % 256;
      rgba8[i].r = v;
      rgba8[i].g = v;
      rgba8[i].b = v;
      rgba8[i].a = v;
    }
  }

  return (rgba8);
}

/**
 * gimp_composite_context_init:
 * @ctx:
 * @op:
 * @a_format:
 * @b_format:
 * @d_format:
 * @m_format:
 * @n_pixels:
 * @A:
 * @B:
 * @M:
 * @D:
 *
 *
 *
 * Return value:
 **/
GimpCompositeContext *
gimp_composite_context_init (GimpCompositeContext *ctx,
                                                                                                                                                                                                                                        GimpCompositeOperation op,
                                                                                                                                                                                                                                        GimpPixelFormat a_format,
                                                                                                                                                                                                                                        GimpPixelFormat b_format,
                                                                                                                                                                                                                                        GimpPixelFormat d_format,
                                                                                                                                                                                                                                        GimpPixelFormat m_format,
                                                                                                                                                                                                                                        unsigned long n_pixels,
                                                                                                                                                                                                                                        unsigned char *A,
                                                                                                                                                                                                                                        unsigned char *B,
                                                                                                                                                                                                                                        unsigned char *M,
                                                                                                                                                                                                                                        unsigned char *D)
{
                memset ((void *) ctx, 0, sizeof(*ctx));
                ctx->op = op;
                ctx->n_pixels = n_pixels;
                ctx->scale.scale = 2;
                ctx->pixelformat_A = a_format;
                ctx->pixelformat_B = b_format;
                ctx->pixelformat_D = d_format;
                ctx->pixelformat_M = m_format;
                ctx->A = (unsigned char *) A;
                ctx->B = (unsigned char *) B;
                ctx->M = (unsigned char *) B;
                ctx->D = (unsigned char *) D;
                memset (ctx->D, 0, ctx->n_pixels * gimp_composite_pixel_bpp[ctx->pixelformat_D]);

                return (ctx);
}
