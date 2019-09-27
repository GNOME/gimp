/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Largely based on gimpdrawable-gradient.c
 *
 * gimpoperationgradient.c
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "core/gimpgradient.h"

#include "gimpoperationgradient.h"


#define GRADIENT_CACHE_N_SUPERSAMPLES 4
#define GRADIENT_CACHE_MAX_SIZE       ((1 << 20) / sizeof (GimpRGB))


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_GRADIENT,
  PROP_START_X,
  PROP_START_Y,
  PROP_END_X,
  PROP_END_Y,
  PROP_GRADIENT_TYPE,
  PROP_GRADIENT_REPEAT,
  PROP_OFFSET,
  PROP_GRADIENT_REVERSE,
  PROP_GRADIENT_BLEND_COLOR_SPACE,
  PROP_SUPERSAMPLE,
  PROP_SUPERSAMPLE_DEPTH,
  PROP_SUPERSAMPLE_THRESHOLD,
  PROP_DITHER
};

typedef struct
{
  GimpGradient                *gradient;
  gboolean                     reverse;
  GimpGradientBlendColorSpace  blend_color_space;
  GimpRGB                     *gradient_cache;
  gint                         gradient_cache_size;
  GimpGradientSegment         *last_seg;
  gdouble                      offset;
  gdouble                      sx, sy;
  GimpGradientType             gradient_type;
  gdouble                      dist;
  gdouble                      vec[2];
  GimpRepeatMode               repeat;
  GeglSampler                 *dist_sampler;
} RenderBlendData;


typedef struct
{
  gfloat        *data;
  GeglRectangle  roi;
  GRand         *dither_rand;
} PutPixelData;


/*  local function prototypes  */

static void            gimp_operation_gradient_dispose           (GObject               *gobject);
static void            gimp_operation_gradient_finalize          (GObject               *gobject);
static void            gimp_operation_gradient_get_property      (GObject               *object,
                                                                  guint                  property_id,
                                                                  GValue                *value,
                                                                  GParamSpec            *pspec);
static void            gimp_operation_gradient_set_property      (GObject               *object,
                                                                  guint                  property_id,
                                                                  const GValue          *value,
                                                                  GParamSpec            *pspec);

static void            gimp_operation_gradient_prepare           (GeglOperation         *operation);

static GeglRectangle   gimp_operation_gradient_get_bounding_box  (GeglOperation         *operation);

static gdouble         gradient_calc_conical_sym_factor          (gdouble                dist,
                                                                  gdouble               *axis,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_conical_asym_factor         (gdouble                dist,
                                                                  gdouble               *axis,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_square_factor               (gdouble                dist,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_radial_factor               (gdouble                dist,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_linear_factor               (gdouble                dist,
                                                                  gdouble               *vec,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_bilinear_factor             (gdouble                dist,
                                                                  gdouble               *vec,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_spiral_factor               (gdouble                dist,
                                                                  gdouble               *axis,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y,
                                                                  gboolean               clockwise);

static gdouble         gradient_calc_shapeburst_angular_factor   (GeglSampler           *dist_sampler,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_shapeburst_spherical_factor (GeglSampler           *dist_sampler,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);
static gdouble         gradient_calc_shapeburst_dimpled_factor   (GeglSampler           *dist_sampler,
                                                                  gdouble                offset,
                                                                  gdouble                x,
                                                                  gdouble                y);

static void            gradient_render_pixel                     (gdouble                x,
                                                                  gdouble                y,
                                                                  GimpRGB               *color,
                                                                  gpointer               render_data);

static void            gradient_put_pixel                        (gint                   x,
                                                                  gint                   y,
                                                                  GimpRGB               *color,
                                                                  gpointer               put_pixel_data);

static void            gradient_dither_pixel                     (GimpRGB               *color,
                                                                  GRand                 *dither_rand,
                                                                  gfloat                *dest);

static gboolean        gimp_operation_gradient_process           (GeglOperation         *operation,
                                                                  GeglBuffer            *input,
                                                                  GeglBuffer            *output,
                                                                  const GeglRectangle   *result,
                                                                  gint                   level);

static void            gimp_operation_gradient_invalidate_cache  (GimpOperationGradient *self);
static void            gimp_operation_gradient_validate_cache    (GimpOperationGradient *self);


G_DEFINE_TYPE (GimpOperationGradient, gimp_operation_gradient,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_gradient_parent_class


static void
gimp_operation_gradient_class_init (GimpOperationGradientClass *klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->dispose             = gimp_operation_gradient_dispose;
  object_class->finalize            = gimp_operation_gradient_finalize;
  object_class->set_property        = gimp_operation_gradient_set_property;
  object_class->get_property        = gimp_operation_gradient_get_property;

  operation_class->prepare          = gimp_operation_gradient_prepare;
  operation_class->get_bounding_box = gimp_operation_gradient_get_bounding_box;

  filter_class->process             = gimp_operation_gradient_process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:gradient",
                                 "categories",  "gimp",
                                 "description", "GIMP Gradient operation",
                                 NULL);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        "Context",
                                                        "A GimpContext",
                                                        GIMP_TYPE_OBJECT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GRADIENT,
                                   g_param_spec_object ("gradient",
                                                        "Gradient",
                                                        "A GimpGradient to render",
                                                        GIMP_TYPE_OBJECT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_START_X,
                                   g_param_spec_double ("start-x",
                                                        "Start X",
                                                        "X coordinate of the first point",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_START_Y,
                                   g_param_spec_double ("start-y",
                                                        "Start Y",
                                                        "Y coordinate of the first point",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_END_X,
                                   g_param_spec_double ("end-x",
                                                        "End X",
                                                        "X coordinate of the second point",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 200,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_END_Y,
                                   g_param_spec_double ("end-y",
                                                        "End Y",
                                                        "Y coordinate of the second point",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 200,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GRADIENT_TYPE,
                                   g_param_spec_enum ("gradient-type",
                                                      "Gradient Type",
                                                      "The type of gradient to render",
                                                      GIMP_TYPE_GRADIENT_TYPE,
                                                      GIMP_GRADIENT_LINEAR,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GRADIENT_REPEAT,
                                   g_param_spec_enum ("gradient-repeat",
                                                      "Repeat mode",
                                                      "Repeat mode",
                                                      GIMP_TYPE_REPEAT_MODE,
                                                      GIMP_REPEAT_NONE,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET,
                                   g_param_spec_double ("offset",
                                                        "Offset",
                                                        "Offset relates to the starting and ending coordinates "
                                                        "specified for the blend. This parameter is mode dependent.",
                                                        0, G_MAXDOUBLE, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GRADIENT_REVERSE,
                                   g_param_spec_boolean ("gradient-reverse",
                                                         "Reverse",
                                                         "Reverse the gradient",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GRADIENT_BLEND_COLOR_SPACE,
                                   g_param_spec_enum ("gradient-blend-color-space",
                                                      "Blend Color Space",
                                                      "Which color space to use when blending RGB gradient segments",
                                                      GIMP_TYPE_GRADIENT_BLEND_COLOR_SPACE,
                                                      GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SUPERSAMPLE,
                                   g_param_spec_boolean ("supersample",
                                                         "Supersample",
                                                         "Do adaptive supersampling",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SUPERSAMPLE_DEPTH,
                                   g_param_spec_int ("supersample-depth",
                                                     "Max depth",
                                                     "Maximum recursion levels for supersampling",
                                                     1, 9, 3,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SUPERSAMPLE_THRESHOLD,
                                   g_param_spec_double ("supersample-threshold",
                                                        "Threshold",
                                                        "Supersampling threshold",
                                                        0, 4, 0.20,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DITHER,
                                   g_param_spec_boolean ("dither",
                                                         "Dither",
                                                         "Use dithering to reduce banding",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_operation_gradient_init (GimpOperationGradient *self)
{
  g_mutex_init (&self->gradient_cache_mutex);
}

static void
gimp_operation_gradient_dispose (GObject *object)
{
  GimpOperationGradient *self = GIMP_OPERATION_GRADIENT (object);

  gimp_operation_gradient_invalidate_cache (self);

  g_clear_object (&self->gradient);
  g_clear_object (&self->context);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_operation_gradient_finalize (GObject *object)
{
  GimpOperationGradient *self = GIMP_OPERATION_GRADIENT (object);

  g_mutex_clear (&self->gradient_cache_mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_gradient_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
 GimpOperationGradient *self = GIMP_OPERATION_GRADIENT (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, self->context);
      break;

    case PROP_GRADIENT:
      g_value_set_object (value, self->gradient);
      break;

    case PROP_START_X:
      g_value_set_double (value, self->start_x);
      break;

    case PROP_START_Y:
      g_value_set_double (value, self->start_y);
      break;

    case PROP_END_X:
      g_value_set_double (value, self->end_x);
      break;

    case PROP_END_Y:
      g_value_set_double (value, self->end_y);
      break;

    case PROP_GRADIENT_TYPE:
      g_value_set_enum (value, self->gradient_type);
      break;

    case PROP_GRADIENT_REPEAT:
      g_value_set_enum (value, self->gradient_repeat);
      break;

    case PROP_OFFSET:
      g_value_set_double (value, self->offset);
      break;

    case PROP_GRADIENT_REVERSE:
      g_value_set_boolean (value, self->gradient_reverse);
      break;

    case PROP_GRADIENT_BLEND_COLOR_SPACE:
      g_value_set_enum (value, self->gradient_blend_color_space);
      break;

    case PROP_SUPERSAMPLE:
      g_value_set_boolean (value, self->supersample);
      break;

    case PROP_SUPERSAMPLE_DEPTH:
      g_value_set_int (value, self->supersample_depth);
      break;

    case PROP_SUPERSAMPLE_THRESHOLD:
      g_value_set_double (value, self->supersample_threshold);
      break;

    case PROP_DITHER:
      g_value_set_boolean (value, self->dither);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_gradient_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpOperationGradient *self = GIMP_OPERATION_GRADIENT (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      if (self->context)
        g_object_unref (self->context);

      self->context = g_value_dup_object (value);
      break;

    case PROP_GRADIENT:
      {
        GimpGradient *gradient = g_value_get_object (value);

        g_clear_object (&self->gradient);

        if (gradient)
          {
            if (gimp_gradient_has_fg_bg_segments (gradient))
              self->gradient = gimp_gradient_flatten (gradient, self->context);
            else
              self->gradient = g_object_ref (gradient);
          }

        gimp_operation_gradient_invalidate_cache (self);
      }
      break;

    case PROP_START_X:
      self->start_x = g_value_get_double (value);

      gimp_operation_gradient_invalidate_cache (self);
      break;

    case PROP_START_Y:
      self->start_y = g_value_get_double (value);

      gimp_operation_gradient_invalidate_cache (self);
      break;

    case PROP_END_X:
      self->end_x = g_value_get_double (value);

      gimp_operation_gradient_invalidate_cache (self);
      break;

    case PROP_END_Y:
      self->end_y = g_value_get_double (value);

      gimp_operation_gradient_invalidate_cache (self);
      break;

    case PROP_GRADIENT_TYPE:
      self->gradient_type = g_value_get_enum (value);
      break;

    case PROP_GRADIENT_REPEAT:
      self->gradient_repeat = g_value_get_enum (value);
      break;

    case PROP_OFFSET:
      self->offset = g_value_get_double (value);
      break;

    case PROP_GRADIENT_REVERSE:
      self->gradient_reverse = g_value_get_boolean (value);

      gimp_operation_gradient_invalidate_cache (self);
      break;

    case PROP_GRADIENT_BLEND_COLOR_SPACE:
      self->gradient_blend_color_space = g_value_get_enum (value);

      gimp_operation_gradient_invalidate_cache (self);
      break;

    case PROP_SUPERSAMPLE:
      self->supersample = g_value_get_boolean (value);
      break;

    case PROP_SUPERSAMPLE_DEPTH:
      self->supersample_depth = g_value_get_int (value);
      break;

    case PROP_SUPERSAMPLE_THRESHOLD:
      self->supersample_threshold = g_value_get_double (value);
      break;

    case PROP_DITHER:
      self->dither = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_gradient_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static GeglRectangle
gimp_operation_gradient_get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gdouble
gradient_calc_conical_sym_factor (gdouble  dist,
                                  gdouble *axis,
                                  gdouble  offset,
                                  gdouble  x,
                                  gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else if ((x != 0) || (y != 0))
    {
      gdouble vec[2];
      gdouble r;
      gdouble rat;

      /* Calculate offset from the start in pixels */

      r = sqrt (SQR (x) + SQR (y));

      vec[0] = x / r;
      vec[1] = y / r;

      rat = axis[0] * vec[0] + axis[1] * vec[1]; /* Dot product */

      if (rat > 1.0)
        rat = 1.0;
      else if (rat < -1.0)
        rat = -1.0;

      /* This cool idea is courtesy Josh MacDonald,
       * Ali Rahimi --- two more XCF losers.  */

      rat = acos (rat) / G_PI;
      rat = pow (rat, (offset / 10.0) + 1.0);

      return CLAMP (rat, 0.0, 1.0);
    }
  else
    {
      return 0.5;
    }
}

static gdouble
gradient_calc_conical_asym_factor (gdouble  dist,
                                   gdouble *axis,
                                   gdouble  offset,
                                   gdouble  x,
                                   gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else if (x != 0 || y != 0)
    {
      gdouble ang0, ang1;
      gdouble ang;
      gdouble rat;

      ang0 = atan2 (axis[0], axis[1]) + G_PI;

      ang1 = atan2 (x, y) + G_PI;

      ang = ang1 - ang0;

      if (ang < 0.0)
        ang += (2.0 * G_PI);

      rat = ang / (2.0 * G_PI);
      rat = pow (rat, (offset / 10.0) + 1.0);

      return CLAMP (rat, 0.0, 1.0);
    }
  else
    {
      return 0.5; /* We are on middle point */
    }
}

static gdouble
gradient_calc_square_factor (gdouble dist,
                             gdouble offset,
                             gdouble x,
                             gdouble y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      /* Calculate offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = MAX (fabs (x), fabs (y));
      rat = r / dist;

      if (rat < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat >= 1.0) ? 1.0 : 0.0;
      else
        return (rat - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_radial_factor (gdouble dist,
                             gdouble offset,
                             gdouble x,
                             gdouble y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      /* Calculate radial offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = sqrt (SQR (x) + SQR (y));
      rat = r / dist;

      if (rat < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat >= 1.0) ? 1.0 : 0.0;
      else
        return (rat - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_linear_factor (gdouble  dist,
                             gdouble *vec,
                             gdouble  offset,
                             gdouble  x,
                             gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (rat >= 0.0 && rat < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat >= 1.0) ? 1.0 : 0.0;
      else if (rat < 0.0)
        return rat / (1.0 - offset);
      else
        return (rat - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_bilinear_factor (gdouble  dist,
                               gdouble *vec,
                               gdouble  offset,
                               gdouble  x,
                               gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      /* Calculate linear offset from the start line outward */

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (fabs (rat) < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat == 1.0) ? 1.0 : 0.0;
      else
        return (fabs (rat) - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_spiral_factor (gdouble   dist,
                             gdouble  *axis,
                             gdouble   offset,
                             gdouble   x,
                             gdouble   y,
                             gboolean  clockwise)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else if (x != 0.0 || y != 0.0)
    {
      gdouble ang0, ang1;
      gdouble ang;
      double  r;

      offset = offset / 100.0;

      ang0 = atan2 (axis[0], axis[1]) + G_PI;
      ang1 = atan2 (x, y) + G_PI;

      if (clockwise)
        ang = ang1 - ang0;
      else
        ang = ang0 - ang1;

      if (ang < 0.0)
        ang += (2.0 * G_PI);

      r = sqrt (SQR (x) + SQR (y)) / dist;

      return fmod (ang / (2.0 * G_PI) + r + offset, 1.0);
    }
  else
    {
      return 0.5 ; /* We are on the middle point */
    }
}

static gdouble
gradient_calc_shapeburst_angular_factor (GeglSampler *dist_sampler,
                                         gdouble      offset,
                                         gdouble      x,
                                         gdouble      y)
{
  gfloat value;

  offset = offset / 100.0;

  gegl_sampler_get (dist_sampler, x, y, NULL, &value, GEGL_ABYSS_NONE);

  value = 1.0 - value;

  if (value < offset)
    value = 0.0;
  else if (offset == 1.0)
    value = (value >= 1.0) ? 1.0 : 0.0;
  else
    value = (value - offset) / (1.0 - offset);

  return value;
}


static gdouble
gradient_calc_shapeburst_spherical_factor (GeglSampler *dist_sampler,
                                           gdouble      offset,
                                           gdouble      x,
                                           gdouble      y)
{
  gfloat value;

  offset = 1.0 - offset / 100.0;

  gegl_sampler_get (dist_sampler, x, y, NULL, &value, GEGL_ABYSS_NONE);

  if (value > offset)
    value = 1.0;
  else if (offset == 0.0)
    value = (value <= 0.0) ? 0.0 : 1.0;
  else
    value = value / offset;

  value = 1.0 - sin (0.5 * G_PI * value);

  return value;
}


static gdouble
gradient_calc_shapeburst_dimpled_factor (GeglSampler *dist_sampler,
                                         gdouble      offset,
                                         gdouble      x,
                                         gdouble      y)
{
  gfloat value;

  offset = 1.0 - offset / 100.0;

  gegl_sampler_get (dist_sampler, x, y, NULL, &value, GEGL_ABYSS_NONE);

  if (value > offset)
    value = 1.0;
  else if (offset == 0.0)
    value = (value <= 0.0) ? 0.0 : 1.0;
  else
    value = value / offset;

  value = cos (0.5 * G_PI * value);

  return value;
}

static void
gradient_render_pixel (gdouble   x,
                       gdouble   y,
                       GimpRGB  *color,
                       gpointer  render_data)
{
  RenderBlendData *rbd = render_data;
  gdouble          factor;

  /*  we want to calculate the color at the pixel's center  */
  x += 0.5;
  y += 0.5;

  /* Calculate blending factor */

  switch (rbd->gradient_type)
    {
    case GIMP_GRADIENT_LINEAR:
      factor = gradient_calc_linear_factor (rbd->dist,
                                            rbd->vec, rbd->offset,
                                            x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_BILINEAR:
      factor = gradient_calc_bilinear_factor (rbd->dist,
                                              rbd->vec, rbd->offset,
                                              x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_RADIAL:
      factor = gradient_calc_radial_factor (rbd->dist,
                                            rbd->offset,
                                            x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_SQUARE:
      factor = gradient_calc_square_factor (rbd->dist, rbd->offset,
                                            x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_CONICAL_SYMMETRIC:
      factor = gradient_calc_conical_sym_factor (rbd->dist,
                                                 rbd->vec, rbd->offset,
                                                 x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_CONICAL_ASYMMETRIC:
      factor = gradient_calc_conical_asym_factor (rbd->dist,
                                                  rbd->vec, rbd->offset,
                                                  x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_SHAPEBURST_ANGULAR:
      factor = gradient_calc_shapeburst_angular_factor (rbd->dist_sampler,
                                                        rbd->offset,
                                                        x, y);
      break;

    case GIMP_GRADIENT_SHAPEBURST_SPHERICAL:
      factor = gradient_calc_shapeburst_spherical_factor (rbd->dist_sampler,
                                                          rbd->offset,
                                                          x, y);
      break;

    case GIMP_GRADIENT_SHAPEBURST_DIMPLED:
      factor = gradient_calc_shapeburst_dimpled_factor (rbd->dist_sampler,
                                                        rbd->offset,
                                                        x, y);
      break;

    case GIMP_GRADIENT_SPIRAL_CLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist,
                                            rbd->vec, rbd->offset,
                                            x - rbd->sx, y - rbd->sy, TRUE);
      break;

    case GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist,
                                            rbd->vec, rbd->offset,
                                            x - rbd->sx, y - rbd->sy, FALSE);
      break;

    default:
      g_return_if_reached ();
      break;
    }

  /* Adjust for repeat */

  switch (rbd->repeat)
    {
    case GIMP_REPEAT_NONE:
      break;

    case GIMP_REPEAT_SAWTOOTH:
      factor = factor - floor (factor);
      break;

    case GIMP_REPEAT_TRIANGULAR:
      {
        guint ifactor;

        if (factor < 0.0)
          factor = -factor;

        ifactor = (guint) factor;
        factor = factor - floor (factor);

        if (ifactor & 1)
          factor = 1.0 - factor;
      }
      break;

    case GIMP_REPEAT_TRUNCATE:
      if (factor < 0.0 || factor > 1.0)
        {
          gimp_rgba_set (color, 0.0, 0.0, 0.0, 0.0);
          return;
        }
      break;
    }

  /* Blend the colors */

  if (rbd->gradient_cache)
    {
      factor = CLAMP (factor, 0.0, 1.0);

      *color =
        rbd->gradient_cache[ROUND (factor * (rbd->gradient_cache_size - 1))];
    }
  else
    {
      rbd->last_seg = gimp_gradient_get_color_at (rbd->gradient, NULL,
                                                  rbd->last_seg, factor,
                                                  rbd->reverse,
                                                  rbd->blend_color_space,
                                                  color);
    }
}

static void
gradient_put_pixel (gint      x,
                    gint      y,
                    GimpRGB  *color,
                    gpointer  put_pixel_data)
{
  PutPixelData *ppd   = put_pixel_data;
  const gint    index = (y - ppd->roi.y) * ppd->roi.width + (x - ppd->roi.x);
  gfloat       *dest  = ppd->data + 4 * index;

  if (ppd->dither_rand)
    {
      gradient_dither_pixel (color, ppd->dither_rand, dest);

      dest += 4;
    }
  else
    {
      *dest++ = color->r;
      *dest++ = color->g;
      *dest++ = color->b;
      *dest++ = color->a;
    }
}

static void
gradient_dither_pixel (GimpRGB *color,
                       GRand   *dither_rand,
                       gfloat  *dest)
{
  gfloat r, g, b, a;
  guint  i;

  i = g_rand_int (dither_rand);

  r = color->r + (gdouble) (i & 0xff) / 256.0 / 256.0 - 0.5 / 256.0; i >>= 8;
  g = color->g + (gdouble) (i & 0xff) / 256.0 / 256.0 - 0.5 / 256.0; i >>= 8;
  b = color->b + (gdouble) (i & 0xff) / 256.0 / 256.0 - 0.5 / 256.0; i >>= 8;

  if (color->a > 0.0 && color->a < 1.0)
    a = color->a + (gdouble) (i & 0xff) / 256.0 / 256.0 - 0.5 / 256.0;
  else
    a = color->a;

  *dest++ = CLAMP (r, 0.0, 1.0);
  *dest++ = CLAMP (g, 0.0, 1.0);
  *dest++ = CLAMP (b, 0.0, 1.0);
  *dest++ = CLAMP (a, 0.0, 1.0);
}

static gboolean
gimp_operation_gradient_process (GeglOperation       *operation,
                                 GeglBuffer          *input,
                                 GeglBuffer          *output,
                                 const GeglRectangle *result,
                                 gint                 level)
{
  GimpOperationGradient *self = GIMP_OPERATION_GRADIENT (operation);

  const gdouble sx = self->start_x;
  const gdouble sy = self->start_y;
  const gdouble ex = self->end_x;
  const gdouble ey = self->end_y;

  RenderBlendData rbd = { 0, };

  GeglBufferIterator *iter;
  GeglRectangle      *roi;
  GRand              *dither_rand = NULL;

  if (! self->gradient)
    return TRUE;

  gimp_operation_gradient_validate_cache (self);

  rbd.gradient            = self->gradient;
  rbd.reverse             = self->gradient_reverse;
  rbd.blend_color_space   = self->gradient_blend_color_space;
  rbd.gradient_cache      = self->gradient_cache;
  rbd.gradient_cache_size = self->gradient_cache_size;

  /* Calculate type-specific parameters */

  switch (self->gradient_type)
    {
    case GIMP_GRADIENT_RADIAL:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
      break;

    case GIMP_GRADIENT_SQUARE:
      rbd.dist = MAX (fabs (ex - sx), fabs (ey - sy));
      break;

    case GIMP_GRADIENT_CONICAL_SYMMETRIC:
    case GIMP_GRADIENT_CONICAL_ASYMMETRIC:
    case GIMP_GRADIENT_SPIRAL_CLOCKWISE:
    case GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE:
    case GIMP_GRADIENT_LINEAR:
    case GIMP_GRADIENT_BILINEAR:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));

      if (rbd.dist > 0.0)
        {
          rbd.vec[0] = (ex - sx) / rbd.dist;
          rbd.vec[1] = (ey - sy) / rbd.dist;
        }

      break;

    case GIMP_GRADIENT_SHAPEBURST_ANGULAR:
    case GIMP_GRADIENT_SHAPEBURST_SPHERICAL:
    case GIMP_GRADIENT_SHAPEBURST_DIMPLED:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
      rbd.dist_sampler = gegl_buffer_sampler_new_at_level (
        input, babl_format ("Y float"), GEGL_SAMPLER_NEAREST, level);
      break;

    default:
      g_return_val_if_reached (FALSE);
      break;
    }

  /* Initialize render data */

  rbd.offset        = self->offset;
  rbd.sx            = self->start_x;
  rbd.sy            = self->start_y;
  rbd.gradient_type = self->gradient_type;
  rbd.repeat        = self->gradient_repeat;

  /* Render the gradient! */

  iter = gegl_buffer_iterator_new (output, result, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);
  roi = &iter->items[0].roi;

  if (self->dither)
    dither_rand = g_rand_new ();

  if (self->supersample)
    {
      PutPixelData ppd;

      ppd.dither_rand = dither_rand;

      while (gegl_buffer_iterator_next (iter))
        {
          ppd.data = iter->items[0].data;
          ppd.roi  = *roi;

          gimp_adaptive_supersample_area (roi->x, roi->y,
                                          roi->x + roi->width  - 1,
                                          roi->y + roi->height - 1,
                                          self->supersample_depth,
                                          self->supersample_threshold,
                                          gradient_render_pixel, &rbd,
                                          gradient_put_pixel, &ppd,
                                          NULL,
                                          NULL);
        }
    }
  else
    {
      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *dest = iter->items[0].data;
          gint    endx = roi->x + roi->width;
          gint    endy = roi->y + roi->height;
          gint    x, y;

          if (dither_rand)
            {
              for (y = roi->y; y < endy; y++)
                for (x = roi->x; x < endx; x++)
                  {
                    GimpRGB  color = { 0.0, 0.0, 0.0, 1.0 };

                    gradient_render_pixel (x, y, &color, &rbd);
                    gradient_dither_pixel (&color, dither_rand, dest);

                    dest += 4;
                  }
            }
          else
            {
              for (y = roi->y; y < endy; y++)
                for (x = roi->x; x < endx; x++)
                  {
                    GimpRGB  color = { 0.0, 0.0, 0.0, 1.0 };

                    gradient_render_pixel (x, y, &color, &rbd);

                    *dest++ = color.r;
                    *dest++ = color.g;
                    *dest++ = color.b;
                    *dest++ = color.a;
                  }
            }
        }
    }

  if (self->dither)
    g_rand_free (dither_rand);

  g_clear_object (&rbd.dist_sampler);

  return TRUE;
}

static void
gimp_operation_gradient_invalidate_cache (GimpOperationGradient *self)
{
  g_clear_pointer (&self->gradient_cache, g_free);
}

static void
gimp_operation_gradient_validate_cache (GimpOperationGradient *self)
{
  GimpGradientSegment *last_seg = NULL;
  gint                 cache_size;
  gint                 i;

  if (! self->gradient)
    return;

  g_mutex_lock (&self->gradient_cache_mutex);

  if (self->gradient_cache)
    {
      g_mutex_unlock (&self->gradient_cache_mutex);

      return;
    }

  cache_size = ceil (hypot (self->start_x - self->end_x,
                            self->start_y - self->end_y)) *
               GRADIENT_CACHE_N_SUPERSAMPLES;

  /*  have at least two values in the cache  */
  cache_size = MAX (cache_size, 2);

  /*  don't use a cache if its necessary size is too big  */
  if (cache_size > GRADIENT_CACHE_MAX_SIZE)
    {
      g_mutex_unlock (&self->gradient_cache_mutex);

      return;
    }

  self->gradient_cache      = g_new0 (GimpRGB, cache_size);
  self->gradient_cache_size = cache_size;

  for (i = 0; i < self->gradient_cache_size; i++)
    {
      gdouble factor = (gdouble) i / (gdouble) (self->gradient_cache_size - 1);

      last_seg = gimp_gradient_get_color_at (self->gradient, NULL, last_seg,
                                             factor,
                                             self->gradient_reverse,
                                             self->gradient_blend_color_space,
                                             self->gradient_cache + i);
    }

  g_mutex_unlock (&self->gradient_cache_mutex);
}
