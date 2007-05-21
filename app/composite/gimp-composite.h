/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gimp Image Compositing
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

#ifndef __GIMP_COMPOSITE_H__
#define __GIMP_COMPOSITE_H__

typedef enum
{
  GIMP_PIXELFORMAT_V8,
  GIMP_PIXELFORMAT_VA8,
  GIMP_PIXELFORMAT_RGB8,
  GIMP_PIXELFORMAT_RGBA8,
  GIMP_PIXELFORMAT_ANY,
  GIMP_PIXELFORMAT_N
} GimpPixelFormat;

/*
 * gtk-doc is unhappy with these #ifdef's inside the enumeration.
 *
#ifdef GIMP_COMPOSITE_16BIT
  GIMP_PIXELFORMAT_V16,
  GIMP_PIXELFORMAT_VA16,
  GIMP_PIXELFORMAT_RGB16,
  GIMP_PIXELFORMAT_RGBA16,
#endif
#ifdef GIMP_COMPOSITE_32BIT
  GIMP_PIXELFORMAT_V32,
  GIMP_PIXELFORMAT_VA32,
  GIMP_PIXELFORMAT_RGB32,
  GIMP_PIXELFORMAT_RGBA32,
#endif
  *
  */

/* bytes per-pixel for each of the pixel formats */
extern const guchar gimp_composite_pixel_bpp[];

/* does pixel format have alpha? */
extern const guchar gimp_composite_pixel_alphap[];

/* converter between alpha and non-alpha pixel formats */
extern const GimpPixelFormat gimp_composite_pixel_alpha[];


#define GIMP_COMPOSITE_ALPHA_OPAQUE      (-1)
#define GIMP_COMPOSITE_ALPHA_TRANSPARENT (0)

/*
 * This is the enumeration of all the supported compositing
 * operations.  Many of them are taken from the GimpLayerModeEffect
 * enumeration, but there are (possibly more) implemented.  Here is
 * where they are all enumerated.
 *
 * Nota Bene: Unfortunately, the order here is important!
 */
typedef enum
{
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
  GIMP_COMPOSITE_XOR,
  GIMP_COMPOSITE_N
} GimpCompositeOperation;

struct GimpCompositeOperationEffects
{
  guchar affect_opacity;
  guchar increase_opacity;
  guchar decrease_opacity;
};

extern struct GimpCompositeOperationEffects gimp_composite_operation_effects[];

/*
 * This is structure for communicating all that is necessary to a
 * compositing operation.
 */
typedef struct
{
  guchar          *A;             /* Source A    */
  guchar          *B;             /* Source B    */
  guchar          *D;             /* Destination */
  const guchar    *M;             /* Mask        */
  gulong           n_pixels;

  GimpPixelFormat  pixelformat_A;
  GimpPixelFormat  pixelformat_B;
  GimpPixelFormat  pixelformat_D;
  GimpPixelFormat  pixelformat_M;

  struct { gint opacity; gchar affect;   } replace;
  struct { gint scale;                   } scale;
  struct { gint blend;                   } blend;
  struct { gint x; gint y; gint opacity; } dissolve;

  CombinationMode        combine;
  GimpCompositeOperation op;
} GimpCompositeContext;


struct GimpCompositeOptions
{
  gulong  bits;
};

#define GIMP_COMPOSITE_OPTION_USE           0x1
#define GIMP_COMPOSITE_OPTION_NOEXTENSIONS  0x2
#define GIMP_COMPOSITE_OPTION_VERBOSE       0x4


extern struct GimpCompositeOptions gimp_composite_options;

void          gimp_composite_init               (gboolean  be_verbose,
                                                 gboolean  use_cpu_accel);
gboolean      gimp_composite_use_cpu_accel      (void);

void          gimp_composite_dispatch           (GimpCompositeContext   *ctx);

void          gimp_composite_context_print      (GimpCompositeContext   *ctx);
const gchar * gimp_composite_mode_astext        (GimpCompositeOperation  op);
const gchar * gimp_composite_pixelformat_astext (GimpPixelFormat         format);

extern const gchar *gimp_composite_function_name[GIMP_COMPOSITE_N][GIMP_PIXELFORMAT_N][GIMP_PIXELFORMAT_N][GIMP_PIXELFORMAT_N];
extern void (*gimp_composite_function[GIMP_COMPOSITE_N][GIMP_PIXELFORMAT_N][GIMP_PIXELFORMAT_N][GIMP_PIXELFORMAT_N])(GimpCompositeContext *);

#endif  /* __GIMP_COMPOSITE_H__  */
