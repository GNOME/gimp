/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimppaintoptions.h"


#define DEFAULT_INCREMENTAL     FALSE

#define DEFAULT_OPACITY         TRUE
#define DEFAULT_PRESSURE        TRUE
#define DEFAULT_RATE            FALSE
#define DEFAULT_SIZE            FALSE
#define DEFAULT_COLOR           FALSE

#define DEFAULT_USE_FADE        FALSE
#define DEFAULT_FADE_OUT        100.0
#define DEFAULT_FADE_UNIT       GIMP_UNIT_PIXEL
#define DEFAULT_USE_GRADIENT    FALSE
#define DEFAULT_GRADIENT_LENGTH 100.0
#define DEFAULT_GRADIENT_UNIT   GIMP_UNIT_PIXEL
#define DEFAULT_GRADIENT_TYPE   GIMP_GRADIENT_LOOP_TRIANGLE


static void   gimp_paint_options_init       (GimpPaintOptions      *options);
static void   gimp_paint_options_class_init (GimpPaintOptionsClass *options_class);

static GimpPressureOptions * gimp_pressure_options_new (void);
static GimpGradientOptions * gimp_gradient_options_new (void);


GType
gimp_paint_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPaintOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paint_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paint_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpPaintOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_paint_options_class_init (GimpPaintOptionsClass *klass)
{
}

static void
gimp_paint_options_init (GimpPaintOptions *options)
{
  options->incremental      = options->incremental_d = DEFAULT_INCREMENTAL;
  options->incremental_save = DEFAULT_INCREMENTAL;

  options->pressure_options = gimp_pressure_options_new ();
  options->gradient_options = gimp_gradient_options_new ();
}

GimpPaintOptions *
gimp_paint_options_new (Gimp  *gimp,
                        GType  options_type)
{
  GimpPaintOptions *options;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (options_type, GIMP_TYPE_PAINT_OPTIONS),
                        NULL);

  options = g_object_new (options_type,
                          "gimp", gimp,
                          NULL);

  return options;
}


/*  private functions  */

static GimpPressureOptions *
gimp_pressure_options_new (void)
{
  GimpPressureOptions *pressure;

  pressure = g_new0 (GimpPressureOptions, 1);

  pressure->opacity  = pressure->opacity_d  = DEFAULT_OPACITY;
  pressure->pressure = pressure->pressure_d = DEFAULT_PRESSURE;
  pressure->rate     = pressure->rate_d     = DEFAULT_RATE;
  pressure->size     = pressure->size_d     = DEFAULT_SIZE;
  pressure->color    = pressure->color_d    = DEFAULT_COLOR;

  pressure->opacity_w  = NULL;
  pressure->pressure_w = NULL;
  pressure->rate_w     = NULL;
  pressure->size_w     = NULL;
  pressure->color_w    = NULL;

  return pressure;
}

static GimpGradientOptions *
gimp_gradient_options_new (void)
{
  GimpGradientOptions *gradient;

  gradient = g_new0 (GimpGradientOptions, 1);

  gradient->use_fade        = gradient->use_fade_d        = DEFAULT_USE_FADE;
  gradient->fade_out        = gradient->fade_out_d        = DEFAULT_FADE_OUT;
  gradient->fade_unit       = gradient->fade_unit_d       = DEFAULT_FADE_UNIT;
  gradient->use_gradient    = gradient->use_gradient_d    = DEFAULT_USE_GRADIENT;
  gradient->gradient_length = gradient->gradient_length_d = DEFAULT_GRADIENT_LENGTH;
  gradient->gradient_unit   = gradient->gradient_unit_d   = DEFAULT_GRADIENT_UNIT;
  gradient->gradient_type   = gradient->gradient_type_d   = DEFAULT_GRADIENT_TYPE;

  gradient->use_fade_w        = NULL;
  gradient->fade_out_w        = NULL;
  gradient->fade_unit_w       = NULL;
  gradient->use_gradient_w    = NULL;
  gradient->gradient_length_w = NULL;
  gradient->gradient_unit_w   = NULL;
  gradient->gradient_type_w   = NULL;

  return gradient;
}
