/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpimage.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpgradient.h"
#include "core/gimppaintinfo.h"

#include "gimppaintoptions.h"

#include "gimp-intl.h"


#define DEFAULT_BRUSH_SIZE             20.0
#define DEFAULT_BRUSH_ASPECT_RATIO     0.0
#define DEFAULT_BRUSH_ANGLE            0.0

#define DEFAULT_APPLICATION_MODE       GIMP_PAINT_CONSTANT
#define DEFAULT_HARD                   FALSE

#define DEFAULT_USE_JITTER             FALSE
#define DEFAULT_JITTER_AMOUNT          0.2

#define DEFAULT_DYNAMICS_EXPANDED      FALSE

#define DEFAULT_FADE_LENGTH            100.0
#define DEFAULT_FADE_REVERSE           FALSE
#define DEFAULT_FADE_REPEAT            GIMP_REPEAT_NONE
#define DEFAULT_FADE_UNIT              GIMP_UNIT_PIXEL

#define DEFAULT_GRADIENT_REVERSE       FALSE
#define DEFAULT_GRADIENT_REPEAT        GIMP_REPEAT_TRIANGULAR
#define DEFAULT_GRADIENT_LENGTH        100.0
#define DEFAULT_GRADIENT_UNIT          GIMP_UNIT_PIXEL

#define DYNAMIC_MAX_VALUE              1.0
#define DYNAMIC_MIN_VALUE              0.0

#define DEFAULT_SMOOTHING_QUALITY      20
#define DEFAULT_SMOOTHING_FACTOR       50


enum
{
  PROP_0,

  PROP_PAINT_INFO,

  PROP_BRUSH_SIZE,
  PROP_BRUSH_ASPECT_RATIO,
  PROP_BRUSH_ANGLE,

  PROP_APPLICATION_MODE,
  PROP_HARD,

  PROP_USE_JITTER,
  PROP_JITTER_AMOUNT,

  PROP_DYNAMICS_EXPANDED,

  PROP_FADE_LENGTH,
  PROP_FADE_REVERSE,
  PROP_FADE_REPEAT,
  PROP_FADE_UNIT,

  PROP_GRADIENT_REVERSE,

  PROP_BRUSH_VIEW_TYPE,
  PROP_BRUSH_VIEW_SIZE,
  PROP_DYNAMICS_VIEW_TYPE,
  PROP_DYNAMICS_VIEW_SIZE,
  PROP_PATTERN_VIEW_TYPE,
  PROP_PATTERN_VIEW_SIZE,
  PROP_GRADIENT_VIEW_TYPE,
  PROP_GRADIENT_VIEW_SIZE,

  PROP_USE_SMOOTHING,
  PROP_SMOOTHING_QUALITY,
  PROP_SMOOTHING_FACTOR
};


static void    gimp_paint_options_dispose          (GObject      *object);
static void    gimp_paint_options_finalize         (GObject      *object);
static void    gimp_paint_options_set_property     (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void    gimp_paint_options_get_property     (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);



G_DEFINE_TYPE (GimpPaintOptions, gimp_paint_options, GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_paint_options_parent_class


static void
gimp_paint_options_class_init (GimpPaintOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_paint_options_dispose;
  object_class->finalize     = gimp_paint_options_finalize;
  object_class->set_property = gimp_paint_options_set_property;
  object_class->get_property = gimp_paint_options_get_property;

  g_object_class_install_property (object_class, PROP_PAINT_INFO,
                                   g_param_spec_object ("paint-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PAINT_INFO,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BRUSH_SIZE,
                                   "brush-size", _("Brush Size"),
                                   1.0, 10000.0, DEFAULT_BRUSH_SIZE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BRUSH_ASPECT_RATIO,
                                   "brush-aspect-ratio", _("Brush Aspect Ratio"),
                                   -20.0, 20.0, DEFAULT_BRUSH_ASPECT_RATIO,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BRUSH_ANGLE,
                                   "brush-angle", _("Brush Angle"),
                                   -180.0, 180.0, DEFAULT_BRUSH_ANGLE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_APPLICATION_MODE,
                                 "application-mode", _("Every stamp has its own opacity"),
                                 GIMP_TYPE_PAINT_APPLICATION_MODE,
                                 DEFAULT_APPLICATION_MODE,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HARD,
                                    "hard", _("Ignore fuzziness of the current brush"),
                                    DEFAULT_HARD,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_JITTER,
                                    "use-jitter", _("Scatter brush as you paint"),
                                    DEFAULT_USE_JITTER,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_JITTER_AMOUNT,
                                   "jitter-amount", _("Distance of scattering"),
                                   0.0, 50.0, DEFAULT_JITTER_AMOUNT,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DYNAMICS_EXPANDED,
                                     "dynamics-expanded", NULL,
                                    DEFAULT_DYNAMICS_EXPANDED,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FADE_LENGTH,
                                   "fade-length", _("Distance over which strokes fade out"),
                                   0.0, 32767.0, DEFAULT_FADE_LENGTH,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_FADE_UNIT,
                                 "fade-unit", NULL,
                                 TRUE, TRUE, DEFAULT_FADE_UNIT,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADE_REVERSE,
                                    "fade-reverse", _("Reverse direction of fading"),
                                    DEFAULT_FADE_REVERSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FADE_REPEAT,
                                 "fade-repeat", _("How fade is repeated as you paint"),
                                 GIMP_TYPE_REPEAT_MODE,
                                 DEFAULT_FADE_REPEAT,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_GRADIENT_REVERSE,
                                    "gradient-reverse", NULL,
                                    DEFAULT_GRADIENT_REVERSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BRUSH_VIEW_TYPE,
                                 "brush-view-type", NULL,
                                 GIMP_TYPE_VIEW_TYPE,
                                 GIMP_VIEW_TYPE_GRID,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_BRUSH_VIEW_SIZE,
                                "brush-view-size", NULL,
                                GIMP_VIEW_SIZE_TINY,
                                GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                GIMP_VIEW_SIZE_SMALL,
                                GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_DYNAMICS_VIEW_TYPE,
                                  "dynamics-view-type", NULL,
                                 GIMP_TYPE_VIEW_TYPE,
                                 GIMP_VIEW_TYPE_LIST,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_DYNAMICS_VIEW_SIZE,
                                "dynamics-view-size", NULL,
                                GIMP_VIEW_SIZE_TINY,
                                GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                GIMP_VIEW_SIZE_SMALL,
                                GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_PATTERN_VIEW_TYPE,
                                 "pattern-view-type", NULL,
                                 GIMP_TYPE_VIEW_TYPE,
                                 GIMP_VIEW_TYPE_GRID,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_PATTERN_VIEW_SIZE,
                                "pattern-view-size", NULL,
                                GIMP_VIEW_SIZE_TINY,
                                GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                GIMP_VIEW_SIZE_SMALL,
                                GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRADIENT_VIEW_TYPE,
                                 "gradient-view-type", NULL,
                                 GIMP_TYPE_VIEW_TYPE,
                                 GIMP_VIEW_TYPE_LIST,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_GRADIENT_VIEW_SIZE,
                                "gradient-view-size", NULL,
                                GIMP_VIEW_SIZE_TINY,
                                GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                GIMP_VIEW_SIZE_LARGE,
                                GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_SMOOTHING,
                                    "use-smoothing", _("Paint smoother strokes"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_SMOOTHING_QUALITY,
                                "smoothing-quality", _("Depth of smoothing"),
                                1, 100, DEFAULT_SMOOTHING_QUALITY,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_SMOOTHING_FACTOR,
                                   "smoothing-factor", _("Gravity of the pen"),
                                   3.0, 1000.0, DEFAULT_SMOOTHING_FACTOR,
                                   /* Max velocity is set at 3.
                                    * Allowing for smoothing factor to be
                                    * less than velcoty results in numeric
                                    * instablility */
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_paint_options_init (GimpPaintOptions *options)
{
  options->application_mode_save = DEFAULT_APPLICATION_MODE;

  options->jitter_options    = g_slice_new0 (GimpJitterOptions);
  options->fade_options      = g_slice_new0 (GimpFadeOptions);
  options->gradient_options  = g_slice_new0 (GimpGradientOptions);
  options->smoothing_options = g_slice_new0 (GimpSmoothingOptions);
}

static void
gimp_paint_options_dispose (GObject *object)
{
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (object);

  if (options->paint_info)
    {
      g_object_unref (options->paint_info);
      options->paint_info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_paint_options_finalize (GObject *object)
{
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (object);

  g_slice_free (GimpJitterOptions,    options->jitter_options);
  g_slice_free (GimpFadeOptions,      options->fade_options);
  g_slice_free (GimpGradientOptions,  options->gradient_options);
  g_slice_free (GimpSmoothingOptions, options->smoothing_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_paint_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpPaintOptions    *options            = GIMP_PAINT_OPTIONS (object);
  GimpFadeOptions     *fade_options       = options->fade_options;
  GimpJitterOptions   *jitter_options     = options->jitter_options;
  GimpGradientOptions *gradient_options   = options->gradient_options;
  GimpSmoothingOptions *smoothing_options = options->smoothing_options;

  switch (property_id)
    {
    case PROP_PAINT_INFO:
      options->paint_info = g_value_dup_object (value);
      break;

    case PROP_BRUSH_SIZE:
      options->brush_size = g_value_get_double (value);
      break;

    case PROP_BRUSH_ASPECT_RATIO:
      options->brush_aspect_ratio = g_value_get_double (value);
      break;

    case PROP_BRUSH_ANGLE:
      options->brush_angle = - 1.0 * g_value_get_double (value) / 360.0; /* let's make the angle mathematically correct */
      break;

    case PROP_APPLICATION_MODE:
      options->application_mode = g_value_get_enum (value);
      break;

    case PROP_HARD:
      options->hard = g_value_get_boolean (value);
      break;

    case PROP_USE_JITTER:
      jitter_options->use_jitter = g_value_get_boolean (value);
      break;

    case PROP_JITTER_AMOUNT:
      jitter_options->jitter_amount = g_value_get_double (value);
      break;

    case PROP_DYNAMICS_EXPANDED:
      options->dynamics_expanded = g_value_get_boolean (value);
      break;

    case PROP_FADE_LENGTH:
      fade_options->fade_length = g_value_get_double (value);
      break;

    case PROP_FADE_REVERSE:
      fade_options->fade_reverse = g_value_get_boolean (value);
      break;

    case PROP_FADE_REPEAT:
      fade_options->fade_repeat = g_value_get_enum (value);
      break;

    case PROP_FADE_UNIT:
      fade_options->fade_unit = g_value_get_int (value);
      break;

    case PROP_GRADIENT_REVERSE:
      gradient_options->gradient_reverse = g_value_get_boolean (value);
      break;

    case PROP_BRUSH_VIEW_TYPE:
      options->brush_view_type = g_value_get_enum (value);
      break;

    case PROP_BRUSH_VIEW_SIZE:
      options->brush_view_size = g_value_get_int (value);
      break;

    case PROP_DYNAMICS_VIEW_TYPE:
      options->dynamics_view_type = g_value_get_enum (value);
      break;

    case PROP_DYNAMICS_VIEW_SIZE:
      options->dynamics_view_size = g_value_get_int (value);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      options->pattern_view_type = g_value_get_enum (value);
      break;

    case PROP_PATTERN_VIEW_SIZE:
      options->pattern_view_size = g_value_get_int (value);
      break;

    case PROP_GRADIENT_VIEW_TYPE:
      options->gradient_view_type = g_value_get_enum (value);
      break;

    case PROP_GRADIENT_VIEW_SIZE:
      options->gradient_view_size = g_value_get_int (value);
      break;

    case PROP_USE_SMOOTHING:
      smoothing_options->use_smoothing = g_value_get_boolean (value);
      break;

    case PROP_SMOOTHING_QUALITY:
      smoothing_options->smoothing_quality = g_value_get_int (value);
      break;

    case PROP_SMOOTHING_FACTOR:
      smoothing_options->smoothing_factor = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpPaintOptions     *options           = GIMP_PAINT_OPTIONS (object);
  GimpFadeOptions      *fade_options      = options->fade_options;
  GimpJitterOptions    *jitter_options    = options->jitter_options;
  GimpGradientOptions  *gradient_options  = options->gradient_options;
  GimpSmoothingOptions *smoothing_options = options->smoothing_options;

  switch (property_id)
    {
    case PROP_PAINT_INFO:
      g_value_set_object (value, options->paint_info);
      break;

    case PROP_BRUSH_SIZE:
      g_value_set_double (value, options->brush_size);
      break;

    case PROP_BRUSH_ASPECT_RATIO:
      g_value_set_double (value, options->brush_aspect_ratio);
      break;

    case PROP_BRUSH_ANGLE:
      g_value_set_double (value, - 1.0 * options->brush_angle * 360.0); /* mathematically correct -> intuitively correct */
      break;

    case PROP_APPLICATION_MODE:
      g_value_set_enum (value, options->application_mode);
      break;

    case PROP_HARD:
      g_value_set_boolean (value, options->hard);
      break;

    case PROP_USE_JITTER:
      g_value_set_boolean (value, jitter_options->use_jitter);
      break;

    case PROP_JITTER_AMOUNT:
      g_value_set_double (value, jitter_options->jitter_amount);
      break;

    case PROP_DYNAMICS_EXPANDED:
      g_value_set_boolean (value, options->dynamics_expanded);
      break;

    case PROP_FADE_LENGTH:
      g_value_set_double (value, fade_options->fade_length);
      break;

    case PROP_FADE_REVERSE:
      g_value_set_boolean (value, fade_options->fade_reverse);
      break;

    case PROP_FADE_REPEAT:
      g_value_set_enum (value, fade_options->fade_repeat);
      break;

    case PROP_FADE_UNIT:
      g_value_set_int (value, fade_options->fade_unit);
      break;

    case PROP_GRADIENT_REVERSE:
      g_value_set_boolean (value, gradient_options->gradient_reverse);
      break;

    case PROP_BRUSH_VIEW_TYPE:
      g_value_set_enum (value, options->brush_view_type);
      break;

    case PROP_BRUSH_VIEW_SIZE:
      g_value_set_int (value, options->brush_view_size);
      break;

    case PROP_DYNAMICS_VIEW_TYPE:
      g_value_set_enum (value, options->dynamics_view_type);
      break;

    case PROP_DYNAMICS_VIEW_SIZE:
      g_value_set_int (value, options->dynamics_view_size);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      g_value_set_enum (value, options->pattern_view_type);
      break;

    case PROP_PATTERN_VIEW_SIZE:
      g_value_set_int (value, options->pattern_view_size);
      break;

    case PROP_GRADIENT_VIEW_TYPE:
      g_value_set_enum (value, options->gradient_view_type);
      break;

    case PROP_GRADIENT_VIEW_SIZE:
      g_value_set_int (value, options->gradient_view_size);
      break;

    case PROP_USE_SMOOTHING:
      g_value_set_boolean (value, smoothing_options->use_smoothing);
      break;

    case PROP_SMOOTHING_QUALITY:
      g_value_set_int (value, smoothing_options->smoothing_quality);
      break;

    case PROP_SMOOTHING_FACTOR:
      g_value_set_double (value, smoothing_options->smoothing_factor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


GimpPaintOptions *
gimp_paint_options_new (GimpPaintInfo *paint_info)
{
  GimpPaintOptions *options;

  g_return_val_if_fail (GIMP_IS_PAINT_INFO (paint_info), NULL);

  options = g_object_new (paint_info->paint_options_type,
                          "gimp",       paint_info->gimp,
                          "name",       gimp_object_get_name (paint_info),
                          "paint-info", paint_info,
                          NULL);

  return options;
}

gdouble
gimp_paint_options_get_fade (GimpPaintOptions *paint_options,
                             GimpImage        *image,
                             gdouble           pixel_dist)
{
  GimpFadeOptions *fade_options;
  gdouble          z        = -1.0;
  gdouble          fade_out =  0.0;
  gdouble          unit_factor;
  gdouble          pos;

  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options),
                        DYNAMIC_MAX_VALUE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), DYNAMIC_MAX_VALUE);

  fade_options = paint_options->fade_options;

  switch (fade_options->fade_unit)
    {
    case GIMP_UNIT_PIXEL:
      fade_out = fade_options->fade_length;
      break;

    case GIMP_UNIT_PERCENT:
      fade_out = (MAX (gimp_image_get_width  (image),
                       gimp_image_get_height (image)) *
                  fade_options->fade_length / 100);
      break;

    default:
      {
        gdouble xres;
        gdouble yres;

        gimp_image_get_resolution (image, &xres, &yres);

        unit_factor = gimp_unit_get_factor (fade_options->fade_unit);
        fade_out    = (fade_options->fade_length *
                       MAX (xres, yres) / unit_factor);
      }
      break;
    }

  /*  factor in the fade out value  */
  if (fade_out > 0.0)
    {
      pos = pixel_dist / fade_out;
    }
  else
    pos = DYNAMIC_MAX_VALUE;

  /*  for no repeat, set pos close to 1.0 after the first chunk  */
  if (fade_options->fade_repeat == GIMP_REPEAT_NONE && pos >= DYNAMIC_MAX_VALUE)
    pos = DYNAMIC_MAX_VALUE - 0.0000001;

  if (((gint) pos & 1) &&
      fade_options->fade_repeat != GIMP_REPEAT_SAWTOOTH)
    pos = DYNAMIC_MAX_VALUE - (pos - (gint) pos);
  else
    pos = pos - (gint) pos;

  z = pos;

  if (fade_options->fade_reverse)
    z = 1.0 - z;

  return z;    /*  ln (1/255)  */
}

gdouble
gimp_paint_options_get_jitter (GimpPaintOptions *paint_options,
                               GimpImage        *image)
{
  GimpJitterOptions *jitter_options;

  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), 0.0);

  jitter_options = paint_options->jitter_options;

  if (jitter_options->use_jitter)
    {
      return jitter_options->jitter_amount;
    }

  return 0.0;
}

gboolean
gimp_paint_options_get_gradient_color (GimpPaintOptions *paint_options,
                                       GimpImage        *image,
                                       gdouble           grad_point,
                                       gdouble           pixel_dist,
                                       GimpRGB          *color)
{
  GimpGradientOptions *gradient_options;
  GimpGradient        *gradient;
  GimpDynamics        *dynamics;
  GimpDynamicsOutput  *color_output;

  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  gradient_options = paint_options->gradient_options;

  gradient = gimp_context_get_gradient (GIMP_CONTEXT (paint_options));

  dynamics = gimp_context_get_dynamics (GIMP_CONTEXT (paint_options));

  color_output = gimp_dynamics_get_output (dynamics,
                                           GIMP_DYNAMICS_OUTPUT_COLOR);

  if (gimp_dynamics_output_is_enabled (color_output))
    {
      gimp_gradient_get_color_at (gradient, GIMP_CONTEXT (paint_options),
                                  NULL, grad_point,
                                  gradient_options->gradient_reverse,
                                  color);

      return TRUE;
    }

  return FALSE;
}

GimpBrushApplicationMode
gimp_paint_options_get_brush_mode (GimpPaintOptions *paint_options)
{
  GimpDynamics       *dynamics;
  GimpDynamicsOutput *force_output;

  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), GIMP_BRUSH_SOFT);

  if (paint_options->hard)
    return GIMP_BRUSH_HARD;

  dynamics = gimp_context_get_dynamics (GIMP_CONTEXT (paint_options));

  force_output = gimp_dynamics_get_output (dynamics,
                                           GIMP_DYNAMICS_OUTPUT_FORCE);

  if (!force_output)
    return GIMP_BRUSH_SOFT;

  if (gimp_dynamics_output_is_enabled (force_output))
    return GIMP_BRUSH_PRESSURE;

  return GIMP_BRUSH_SOFT;
}

void
gimp_paint_options_set_default_brush_size (GimpPaintOptions *paint_options,
                                           GimpBrush        *brush)
{
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (brush == NULL || GIMP_IS_BRUSH (brush));

  if (! brush)
    brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

  if (brush)
    {
      gint height;
      gint width;

      gimp_brush_transform_size (brush, 1.0, 0.0, 0.0, &height, &width);

      g_object_set (paint_options,
                    "brush-size", (gdouble) MAX (height, width),
                    NULL);
    }
}

void
gimp_paint_options_copy_brush_props (GimpPaintOptions *src,
                                     GimpPaintOptions *dest)
{
  gdouble  brush_size;
  gdouble  brush_angle;
  gdouble  brush_aspect_ratio;

  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (src));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (dest));

  g_object_get (src,
                "brush-size", &brush_size,
                "brush-angle", &brush_angle,
                "brush-aspect-ratio", &brush_aspect_ratio,
                NULL);

  g_object_set (dest,
                "brush-size", brush_size,
                "brush-angle", brush_angle,
                "brush-aspect-ratio", brush_aspect_ratio,
                NULL);
}

void
gimp_paint_options_copy_dynamics_props (GimpPaintOptions *src,
                                        GimpPaintOptions *dest)
{
  gboolean        dynamics_expanded;
  gboolean        fade_reverse;
  gdouble         fade_length;
  GimpUnit        fade_unit;
  GimpRepeatMode  fade_repeat;

  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (src));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (dest));

  g_object_get (src,
                "dynamics-expanded", &dynamics_expanded,
                "fade-reverse", &fade_reverse,
                "fade-length", &fade_length,
                "fade-unit", &fade_unit,
                "fade-repeat", &fade_repeat,
                NULL);

  g_object_set (dest,
                "dynamics-expanded", dynamics_expanded,
                "fade-reverse", fade_reverse,
                "fade-length", fade_length,
                "fade-unit", fade_unit,
                "fade-repeat", fade_repeat,
                NULL);
}

void
gimp_paint_options_copy_gradient_props (GimpPaintOptions *src,
                                        GimpPaintOptions *dest)
{
  gboolean  gradient_reverse;

  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (src));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (dest));

  g_object_get (src,
                "gradient-reverse", &gradient_reverse,
                NULL);

  g_object_set (dest,
                "gradient-reverse", gradient_reverse,
                NULL);
}
