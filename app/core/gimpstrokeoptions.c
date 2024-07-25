/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokeoptions.c
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdashpattern.h"
#include "gimppaintinfo.h"
#include "gimpparamspecs.h"
#include "gimpstrokeoptions.h"

#include "paint/gimppaintoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,

  PROP_METHOD,

  PROP_STYLE,
  PROP_WIDTH,
  PROP_UNIT,
  PROP_CAP_STYLE,
  PROP_JOIN_STYLE,
  PROP_MITER_LIMIT,
  PROP_ANTIALIAS,
  PROP_DASH_UNIT,
  PROP_DASH_OFFSET,
  PROP_DASH_INFO,

  PROP_PAINT_OPTIONS,
  PROP_EMULATE_DYNAMICS
};

enum
{
  DASH_INFO_CHANGED,
  LAST_SIGNAL
};


typedef struct _GimpStrokeOptionsPrivate GimpStrokeOptionsPrivate;

struct _GimpStrokeOptionsPrivate
{
  GimpStrokeMethod  method;

  /*  options for method == LIBART  */
  gdouble           width;
  GimpUnit         *unit;

  GimpCapStyle      cap_style;
  GimpJoinStyle     join_style;

  gdouble           miter_limit;

  gdouble           dash_offset;
  GArray           *dash_info;

  /*  options for method == PAINT_TOOL  */
  GimpPaintOptions *paint_options;
  gboolean          emulate_dynamics;
};

#define GET_PRIVATE(options) \
        ((GimpStrokeOptionsPrivate *) gimp_stroke_options_get_instance_private ((GimpStrokeOptions *) (options)))


static void   gimp_stroke_options_config_iface_init (gpointer      iface,
                                                     gpointer      iface_data);

static void   gimp_stroke_options_finalize          (GObject      *object);
static void   gimp_stroke_options_set_property      (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   gimp_stroke_options_get_property      (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static GimpConfig * gimp_stroke_options_duplicate   (GimpConfig   *config);


G_DEFINE_TYPE_WITH_CODE (GimpStrokeOptions, gimp_stroke_options,
                         GIMP_TYPE_FILL_OPTIONS,
                         G_ADD_PRIVATE (GimpStrokeOptions)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_stroke_options_config_iface_init))

#define parent_class gimp_stroke_options_parent_class

static GimpConfigInterface *parent_config_iface = NULL;

static guint stroke_options_signals[LAST_SIGNAL] = { 0 };


static void
gimp_stroke_options_class_init (GimpStrokeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *array_spec;

  object_class->finalize     = gimp_stroke_options_finalize;
  object_class->set_property = gimp_stroke_options_set_property;
  object_class->get_property = gimp_stroke_options_get_property;

  klass->dash_info_changed = NULL;

  stroke_options_signals[DASH_INFO_CHANGED] =
    g_signal_new ("dash-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpStrokeOptionsClass, dash_info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DASH_PRESET);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_METHOD,
                         "method",
                         _("Method"),
                         NULL,
                         GIMP_TYPE_STROKE_METHOD,
                         GIMP_STROKE_LINE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_WIDTH,
                           "width",
                           _("Line width"),
                           NULL,
                           0.0, 2000.0, 6.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "unit",
                         _("Unit"),
                         NULL,
                         TRUE, FALSE, gimp_unit_pixel (),
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CAP_STYLE,
                         "cap-style",
                         _("Cap style"),
                         NULL,
                         GIMP_TYPE_CAP_STYLE, GIMP_CAP_BUTT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_JOIN_STYLE,
                         "join-style",
                         _("Join style"),
                         NULL,
                         GIMP_TYPE_JOIN_STYLE, GIMP_JOIN_MITER,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_MITER_LIMIT,
                           "miter-limit",
                           _("Miter limit"),
                           _("Convert a mitered join to a bevelled "
                             "join if the miter would extend to a "
                             "distance of more than miter-limit * "
                             "line-width from the actual join point."),
                           0.0, 100.0, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_DASH_OFFSET,
                           "dash-offset",
                           _("Dash offset"),
                           NULL,
                           0.0, 2000.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  array_spec = g_param_spec_double ("dash-length", NULL, NULL,
                                    0.0, 2000.0, 1.0, GIMP_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DASH_INFO,
                                   gimp_param_spec_value_array ("dash-info",
                                                                NULL, NULL,
                                                                array_spec,
                                                                GIMP_PARAM_STATIC_STRINGS |
                                                                GIMP_CONFIG_PARAM_FLAGS));

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_PAINT_OPTIONS,
                           "paint-options",
                           NULL, NULL,
                           GIMP_TYPE_PAINT_OPTIONS,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EMULATE_DYNAMICS,
                            "emulate-brush-dynamics",
                            _("Emulate brush dynamics"),
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_stroke_options_config_iface_init (gpointer  iface,
                                       gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  parent_config_iface = g_type_interface_peek_parent (config_iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (GIMP_TYPE_CONFIG);

  config_iface->duplicate = gimp_stroke_options_duplicate;
}

static void
gimp_stroke_options_init (GimpStrokeOptions *options)
{
}

static void
gimp_stroke_options_finalize (GObject *object)
{
  GimpStrokeOptionsPrivate *private = GET_PRIVATE (object);

  if (private->dash_info)
    {
      gimp_dash_pattern_free (private->dash_info);
      private->dash_info = NULL;
    }

  g_clear_object (&private->paint_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_stroke_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpStrokeOptions        *options = GIMP_STROKE_OPTIONS (object);
  GimpStrokeOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_METHOD:
      private->method = g_value_get_enum (value);
      break;

    case PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case PROP_UNIT:
      private->unit = g_value_get_object (value);
      break;
    case PROP_CAP_STYLE:
      private->cap_style = g_value_get_enum (value);
      break;
    case PROP_JOIN_STYLE:
      private->join_style = g_value_get_enum (value);
      break;
    case PROP_MITER_LIMIT:
      private->miter_limit = g_value_get_double (value);
      break;
    case PROP_DASH_OFFSET:
      private->dash_offset = g_value_get_double (value);
      break;
    case PROP_DASH_INFO:
      {
        GimpValueArray *value_array = g_value_get_boxed (value);
        GArray         *pattern;

        pattern = gimp_dash_pattern_from_value_array (value_array);
        gimp_stroke_options_take_dash_pattern (options, GIMP_DASH_CUSTOM,
                                               pattern);
      }
      break;

    case PROP_PAINT_OPTIONS:
      if (private->paint_options)
        g_object_unref (private->paint_options);
      private->paint_options = g_value_dup_object (value);
      break;
    case PROP_EMULATE_DYNAMICS:
      private->emulate_dynamics = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_stroke_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpStrokeOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_METHOD:
      g_value_set_enum (value, private->method);
      break;

    case PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case PROP_UNIT:
      g_value_set_object (value, private->unit);
      break;
    case PROP_CAP_STYLE:
      g_value_set_enum (value, private->cap_style);
      break;
    case PROP_JOIN_STYLE:
      g_value_set_enum (value, private->join_style);
      break;
    case PROP_MITER_LIMIT:
      g_value_set_double (value, private->miter_limit);
      break;
    case PROP_DASH_OFFSET:
      g_value_set_double (value, private->dash_offset);
      break;
    case PROP_DASH_INFO:
      {
        GimpValueArray *value_array;

        value_array = gimp_dash_pattern_to_value_array (private->dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;

    case PROP_PAINT_OPTIONS:
      g_value_set_object (value, private->paint_options);
      break;
    case PROP_EMULATE_DYNAMICS:
      g_value_set_boolean (value, private->emulate_dynamics);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpConfig *
gimp_stroke_options_duplicate (GimpConfig *config)
{
  GimpStrokeOptions        *options = GIMP_STROKE_OPTIONS (config);
  GimpStrokeOptionsPrivate *private = GET_PRIVATE (options);
  GimpStrokeOptions        *new_options;

  new_options = GIMP_STROKE_OPTIONS (parent_config_iface->duplicate (config));

  if (private->paint_options)
    {
      GObject *paint_options;

      paint_options = gimp_config_duplicate (GIMP_CONFIG (private->paint_options));
      g_object_set (new_options, "paint-options", paint_options, NULL);
      g_object_unref (paint_options);
    }

  return GIMP_CONFIG (new_options);
}


/*  public functions  */

GimpStrokeOptions *
gimp_stroke_options_new (Gimp        *gimp,
                         GimpContext *context,
                         gboolean     use_context_color)
{
  GimpPaintInfo     *paint_info = NULL;
  GimpStrokeOptions *options;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (use_context_color == FALSE || context != NULL, NULL);

  if (context)
    paint_info = gimp_context_get_paint_info (context);

  if (! paint_info)
    paint_info = gimp_paint_info_get_standard (gimp);

  options = g_object_new (GIMP_TYPE_STROKE_OPTIONS,
                          "gimp",       gimp,
                          "paint-info", paint_info,
                          NULL);

  if (use_context_color)
    {
      gimp_context_define_properties (GIMP_CONTEXT (options),
                                      GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                                      GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                                      GIMP_CONTEXT_PROP_MASK_PATTERN,
                                      FALSE);

      gimp_context_set_parent (GIMP_CONTEXT (options), context);
    }

  return options;
}

GimpStrokeMethod
gimp_stroke_options_get_method (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options),
                        GIMP_STROKE_LINE);

  return GET_PRIVATE (options)->method;
}

gdouble
gimp_stroke_options_get_width (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), 1.0);

  return GET_PRIVATE (options)->width;
}

GimpUnit *
gimp_stroke_options_get_unit (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), gimp_unit_pixel ());

  return GET_PRIVATE (options)->unit;
}

GimpCapStyle
gimp_stroke_options_get_cap_style (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), GIMP_CAP_BUTT);

  return GET_PRIVATE (options)->cap_style;
}

GimpJoinStyle
gimp_stroke_options_get_join_style (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), GIMP_JOIN_MITER);

  return GET_PRIVATE (options)->join_style;
}

gdouble
gimp_stroke_options_get_miter_limit (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), 1.0);

  return GET_PRIVATE (options)->miter_limit;
}

gdouble
gimp_stroke_options_get_dash_offset (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), 0.0);

  return GET_PRIVATE (options)->dash_offset;
}

GArray *
gimp_stroke_options_get_dash_info (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), NULL);

  return GET_PRIVATE (options)->dash_info;
}

GimpPaintOptions *
gimp_stroke_options_get_paint_options (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), NULL);

  return GET_PRIVATE (options)->paint_options;
}

gboolean
gimp_stroke_options_get_emulate_dynamics (GimpStrokeOptions *options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->emulate_dynamics;
}

/**
 * gimp_stroke_options_take_dash_pattern:
 * @options: a #GimpStrokeOptions object
 * @preset: a value out of the #GimpDashPreset enum
 * @pattern: a #GArray or %NULL if @preset is not %GIMP_DASH_CUSTOM
 *
 * Sets the dash pattern. Either a @preset is passed and @pattern is
 * %NULL or @preset is %GIMP_DASH_CUSTOM and @pattern is the #GArray
 * to use as the dash pattern. Note that this function takes ownership
 * of the passed pattern.
 */
void
gimp_stroke_options_take_dash_pattern (GimpStrokeOptions *options,
                                       GimpDashPreset     preset,
                                       GArray            *pattern)
{
  GimpStrokeOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (preset == GIMP_DASH_CUSTOM || pattern == NULL);

  private = GET_PRIVATE (options);

  if (preset != GIMP_DASH_CUSTOM)
    pattern = gimp_dash_pattern_new_from_preset (preset);

  if (private->dash_info)
    gimp_dash_pattern_free (private->dash_info);

  private->dash_info = pattern;

  g_object_notify (G_OBJECT (options), "dash-info");

  g_signal_emit (options, stroke_options_signals [DASH_INFO_CHANGED], 0,
                 preset);
}

void
gimp_stroke_options_prepare (GimpStrokeOptions *options,
                             GimpContext       *context,
                             GimpPaintOptions  *paint_options)
{
  GimpStrokeOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (paint_options == NULL ||
                    GIMP_IS_PAINT_OPTIONS (paint_options));

  private = GET_PRIVATE (options);

  switch (private->method)
    {
    case GIMP_STROKE_LINE:
      break;

    case GIMP_STROKE_PAINT_METHOD:
      {
        GimpPaintInfo *paint_info = GIMP_CONTEXT (options)->paint_info;

        if (paint_options)
          {
            g_return_if_fail (paint_info == paint_options->paint_info);

            /*  undefine the paint-relevant context properties and get them
             *  from the passed context
             */
            gimp_context_define_properties (GIMP_CONTEXT (paint_options),
                                            GIMP_CONTEXT_PROP_MASK_PAINT,
                                            FALSE);
            gimp_context_set_parent (GIMP_CONTEXT (paint_options), context);

            g_object_ref (paint_options);
          }
        else
          {
            GimpCoreConfig      *config       = context->gimp->config;
            GimpContextPropMask  global_props = 0;

            paint_options =
              gimp_config_duplicate (GIMP_CONFIG (paint_info->paint_options));

            /*  FG and BG are always shared between all tools  */
            global_props |= GIMP_CONTEXT_PROP_MASK_FOREGROUND;
            global_props |= GIMP_CONTEXT_PROP_MASK_BACKGROUND;

            if (config->global_brush)
              global_props |= GIMP_CONTEXT_PROP_MASK_BRUSH;
            if (config->global_dynamics)
              global_props |= GIMP_CONTEXT_PROP_MASK_DYNAMICS;
            if (config->global_pattern)
              global_props |= GIMP_CONTEXT_PROP_MASK_PATTERN;
            if (config->global_palette)
              global_props |= GIMP_CONTEXT_PROP_MASK_PALETTE;
            if (config->global_gradient)
              global_props |= GIMP_CONTEXT_PROP_MASK_GRADIENT;
            if (config->global_font)
              global_props |= GIMP_CONTEXT_PROP_MASK_FONT;

            gimp_context_copy_properties (context,
                                          GIMP_CONTEXT (paint_options),
                                          global_props);
          }

        g_object_set (options, "paint-options", paint_options, NULL);
        g_object_unref (paint_options);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

void
gimp_stroke_options_finish (GimpStrokeOptions *options)
{
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));

  g_object_set (options, "paint-options", NULL, NULL);
}
