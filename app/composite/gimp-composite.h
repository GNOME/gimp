/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gimp Image Compositing
 * Copyright (C) 2003  Helvetix Victorinox, a pseudonym, <helvetix@gimp.org>
 * $Id$
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

#ifndef gimp_composite_h
#define gimp_composite_h

#include <sys/types.h>
#include <glib-object.h>
#include "base/base-enums.h"
#include "paint-funcs/paint-funcs-types.h"

#ifndef NULL
#define NULL ((void) 0)
#endif

typedef enum {
  GIMP_PIXELFORMAT_V8,
  GIMP_PIXELFORMAT_VA8,
  GIMP_PIXELFORMAT_RGB8,
  GIMP_PIXELFORMAT_RGBA8,
#if GIMP_16BITCOLOR
  GIMP_PIXELFORMAT_V16,
  GIMP_PIXELFORMAT_VA16,
  GIMP_PIXELFORMAT_RGB16,
  GIMP_PIXELFORMAT_RGBA16,
#endif
  GIMP_PIXELFORMAT_ANY,
  GIMP_PIXELFORMAT_N
} GimpPixelFormat;

typedef struct {
  u_int8_t v;
} gimp_v8_t;

typedef struct {
  u_int8_t v;
  u_int8_t a;
} gimp_va8_t;

typedef struct {
  u_int8_t r;
  u_int8_t g;
  u_int8_t b;
} gimp_rgb8_t;

typedef struct {
  u_int8_t r;
  u_int8_t g;
  u_int8_t b;
  u_int8_t a;
} gimp_rgba8_t;

#ifdef GIMP_16BITCOLOUR
typedef struct {
  u_int16_t v;
} gimp_v16_t;

typedef struct {
  u_int16_t v;
  u_int16_t a;
} gimp_va16_t;

typedef struct {
  u_int16_t r;
  u_int16_t g;
  u_int16_t b;
} gimp_rgb16_t;

typedef struct {
  u_int16_t r;
  u_int16_t g;
  u_int16_t b;
  u_int16_t a;
} gimp_rgba16_t;
#endif

extern unsigned char gimp_composite_pixel_bpp[]; /* bytes per-pixel for each of the pixel formats */
extern unsigned char gimp_composite_pixel_alphap[]; /* does pixel format have alpha? */
extern GimpPixelFormat gimp_composite_pixel_alpha[]; /* converter between alpha and non-alpha pixel formats */

#define GIMP_COMPOSITE_ALPHA_OPAQUE (-1)
#define GIMP_COMPOSITE_ALPHA_TRANSPARENT (0)
/*
 * This is the enumeration of all the supported compositing
 * operations.  Many of them are taken from the GimpLayerModeEffect
 * enumeration, but there are (possibly more) implemented.  Here is
 * where they are all enumerated.
 *
 * Nota Bene: Unfortunately, the order here is important!
 */
typedef enum {
  GIMP_COMPOSITE_NORMAL        = GIMP_NORMAL_MODE,
  GIMP_COMPOSITE_DISSOLVE      = GIMP_DISSOLVE_MODE,
  GIMP_COMPOSITE_BEHIND        = GIMP_BEHIND_MODE,
  GIMP_COMPOSITE_MULTIPLY      = GIMP_MULTIPLY_MODE,
  GIMP_COMPOSITE_SCREEN        = GIMP_SCREEN_MODE,
  GIMP_COMPOSITE_OVERLAY       = GIMP_OVERLAY_MODE,
  GIMP_COMPOSITE_DIFFERENCE    = GIMP_DIFFERENCE_MODE,
  GIMP_COMPOSITE_ADDITION      = GIMP_ADDITION_MODE,
  GIMP_COMPOSITE_SUBTRACT      = GIMP_SUBTRACT_MODE,
  GIMP_COMPOSITE_DARKEN        = GIMP_DARKEN_ONLY_MODE,
  GIMP_COMPOSITE_LIGHTEN       = GIMP_LIGHTEN_ONLY_MODE,
  GIMP_COMPOSITE_HUE           = GIMP_HUE_MODE,
  GIMP_COMPOSITE_SATURATION    = GIMP_SATURATION_MODE,
  GIMP_COMPOSITE_COLOR_ONLY    = GIMP_COLOR_MODE,
  GIMP_COMPOSITE_VALUE         = GIMP_VALUE_MODE,
  GIMP_COMPOSITE_DIVIDE        = GIMP_DIVIDE_MODE,
  GIMP_COMPOSITE_DODGE         = GIMP_DODGE_MODE,
  GIMP_COMPOSITE_BURN          = GIMP_BURN_MODE,
  GIMP_COMPOSITE_HARDLIGHT     = GIMP_HARDLIGHT_MODE,
  GIMP_COMPOSITE_SOFTLIGHT     = GIMP_SOFTLIGHT_MODE,
  GIMP_COMPOSITE_GRAIN_EXTRACT = GIMP_GRAIN_EXTRACT_MODE,
  GIMP_COMPOSITE_GRAIN_MERGE   = GIMP_GRAIN_MERGE_MODE,
  GIMP_COMPOSITE_COLOR_ERASE   = GIMP_COLOR_ERASE_MODE,
  GIMP_COMPOSITE_ERASE         = GIMP_ERASE_MODE,
  GIMP_COMPOSITE_REPLACE       = GIMP_REPLACE_MODE,
  GIMP_COMPOSITE_ANTI_ERASE    = GIMP_ANTI_ERASE_MODE,
  GIMP_COMPOSITE_BLEND,
  GIMP_COMPOSITE_SHADE,
  GIMP_COMPOSITE_SWAP,
  GIMP_COMPOSITE_SCALE,
  GIMP_COMPOSITE_CONVERT,
  GIMP_COMPOSITE_N
} GimpCompositeOperation;

struct GimpCompositeOperationEffects {
  unsigned char affect_opacity;
  unsigned char increase_opacity;
  unsigned char decrease_opacity;
};

extern struct GimpCompositeOperationEffects gimp_composite_operation_effects[];

/*
 * This is structure for communicating all that is necessary to a
 * compositing operation.
 */
typedef struct {
  unsigned char *A;             /* Source A */
  unsigned char *B;             /* Source B */
  unsigned char *D;             /* Destination */
  unsigned char *M;             /* Mask */
  unsigned long n_pixels;

  GimpPixelFormat pixelformat_A;
  GimpPixelFormat pixelformat_B;
  GimpPixelFormat pixelformat_D;
  GimpPixelFormat pixelformat_M;

  struct { int opacity; char affect;  } replace;
  struct { int scale;                 } scale;
  struct { int blend;                 } blend;
  struct { int x; int y; int opacity; } dissolve;

  CombinationMode combine;
  GimpCompositeOperation op;
} GimpCompositeContext;


extern void gimp_composite_dispatch(GimpCompositeContext *);
extern void gimp_composite_init();
extern void gimp_composite_context_print(GimpCompositeContext *);
#endif
