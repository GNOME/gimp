/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmastrokeoptions.c
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligmacontext.h"
#include "ligmadashpattern.h"
#include "ligmapaintinfo.h"
#include "ligmaparamspecs.h"
#include "ligmastrokeoptions.h"

#include "paint/ligmapaintoptions.h"

#include "ligma-intl.h"


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


typedef struct _LigmaStrokeOptionsPrivate LigmaStrokeOptionsPrivate;

struct _LigmaStrokeOptionsPrivate
{
  LigmaStrokeMethod  method;

  /*  options for method == LIBART  */
  gdouble           width;
  LigmaUnit          unit;

  LigmaCapStyle      cap_style;
  LigmaJoinStyle     join_style;

  gdouble           miter_limit;

  gdouble           dash_offset;
  GArray           *dash_info;

  /*  options for method == PAINT_TOOL  */
  LigmaPaintOptions *paint_options;
  gboolean          emulate_dynamics;
};

#define GET_PRIVATE(options) \
        ((LigmaStrokeOptionsPrivate *) ligma_stroke_options_get_instance_private ((LigmaStrokeOptions *) (options)))


static void   ligma_stroke_options_config_iface_init (gpointer      iface,
                                                     gpointer      iface_data);

static void   ligma_stroke_options_finalize          (GObject      *object);
static void   ligma_stroke_options_set_property      (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   ligma_stroke_options_get_property      (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static LigmaConfig * ligma_stroke_options_duplicate   (LigmaConfig   *config);


G_DEFINE_TYPE_WITH_CODE (LigmaStrokeOptions, ligma_stroke_options,
                         LIGMA_TYPE_FILL_OPTIONS,
                         G_ADD_PRIVATE (LigmaStrokeOptions)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_stroke_options_config_iface_init))

#define parent_class ligma_stroke_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;

static guint stroke_options_signals[LAST_SIGNAL] = { 0 };


static void
ligma_stroke_options_class_init (LigmaStrokeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *array_spec;

  object_class->finalize     = ligma_stroke_options_finalize;
  object_class->set_property = ligma_stroke_options_set_property;
  object_class->get_property = ligma_stroke_options_get_property;

  klass->dash_info_changed = NULL;

  stroke_options_signals[DASH_INFO_CHANGED] =
    g_signal_new ("dash-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaStrokeOptionsClass, dash_info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DASH_PRESET);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_METHOD,
                         "method",
                         _("Method"),
                         NULL,
                         LIGMA_TYPE_STROKE_METHOD,
                         LIGMA_STROKE_LINE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_WIDTH,
                           "width",
                           _("Line width"),
                           NULL,
                           0.0, 2000.0, 6.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "unit",
                         _("Unit"),
                         NULL,
                         TRUE, FALSE, LIGMA_UNIT_PIXEL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_CAP_STYLE,
                         "cap-style",
                         _("Cap style"),
                         NULL,
                         LIGMA_TYPE_CAP_STYLE, LIGMA_CAP_BUTT,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_JOIN_STYLE,
                         "join-style",
                         _("Join style"),
                         NULL,
                         LIGMA_TYPE_JOIN_STYLE, LIGMA_JOIN_MITER,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_MITER_LIMIT,
                           "miter-limit",
                           _("Miter limit"),
                           _("Convert a mitered join to a bevelled "
                             "join if the miter would extend to a "
                             "distance of more than miter-limit * "
                             "line-width from the actual join point."),
                           0.0, 100.0, 10.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_DASH_OFFSET,
                           "dash-offset",
                           _("Dash offset"),
                           NULL,
                           0.0, 2000.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  array_spec = g_param_spec_double ("dash-length", NULL, NULL,
                                    0.0, 2000.0, 1.0, LIGMA_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DASH_INFO,
                                   ligma_param_spec_value_array ("dash-info",
                                                                NULL, NULL,
                                                                array_spec,
                                                                LIGMA_PARAM_STATIC_STRINGS |
                                                                LIGMA_CONFIG_PARAM_FLAGS));

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_PAINT_OPTIONS,
                           "paint-options",
                           NULL, NULL,
                           LIGMA_TYPE_PAINT_OPTIONS,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_EMULATE_DYNAMICS,
                            "emulate-brush-dynamics",
                            _("Emulate brush dynamics"),
                            NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_stroke_options_config_iface_init (gpointer  iface,
                                       gpointer  iface_data)
{
  LigmaConfigInterface *config_iface = (LigmaConfigInterface *) iface;

  parent_config_iface = g_type_interface_peek_parent (config_iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (LIGMA_TYPE_CONFIG);

  config_iface->duplicate = ligma_stroke_options_duplicate;
}

static void
ligma_stroke_options_init (LigmaStrokeOptions *options)
{
}

static void
ligma_stroke_options_finalize (GObject *object)
{
  LigmaStrokeOptionsPrivate *private = GET_PRIVATE (object);

  if (private->dash_info)
    {
      ligma_dash_pattern_free (private->dash_info);
      private->dash_info = NULL;
    }

  g_clear_object (&private->paint_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_stroke_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaStrokeOptions        *options = LIGMA_STROKE_OPTIONS (object);
  LigmaStrokeOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_METHOD:
      private->method = g_value_get_enum (value);
      break;

    case PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case PROP_UNIT:
      private->unit = g_value_get_int (value);
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
        LigmaValueArray *value_array = g_value_get_boxed (value);
        GArray         *pattern;

        pattern = ligma_dash_pattern_from_value_array (value_array);
        ligma_stroke_options_take_dash_pattern (options, LIGMA_DASH_CUSTOM,
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
ligma_stroke_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaStrokeOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_METHOD:
      g_value_set_enum (value, private->method);
      break;

    case PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case PROP_UNIT:
      g_value_set_int (value, private->unit);
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
        LigmaValueArray *value_array;

        value_array = ligma_dash_pattern_to_value_array (private->dash_info);
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

static LigmaConfig *
ligma_stroke_options_duplicate (LigmaConfig *config)
{
  LigmaStrokeOptions        *options = LIGMA_STROKE_OPTIONS (config);
  LigmaStrokeOptionsPrivate *private = GET_PRIVATE (options);
  LigmaStrokeOptions        *new_options;

  new_options = LIGMA_STROKE_OPTIONS (parent_config_iface->duplicate (config));

  if (private->paint_options)
    {
      GObject *paint_options;

      paint_options = ligma_config_duplicate (LIGMA_CONFIG (private->paint_options));
      g_object_set (new_options, "paint-options", paint_options, NULL);
      g_object_unref (paint_options);
    }

  return LIGMA_CONFIG (new_options);
}


/*  public functions  */

LigmaStrokeOptions *
ligma_stroke_options_new (Ligma        *ligma,
                         LigmaContext *context,
                         gboolean     use_context_color)
{
  LigmaPaintInfo     *paint_info = NULL;
  LigmaStrokeOptions *options;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (use_context_color == FALSE || context != NULL, NULL);

  if (context)
    paint_info = ligma_context_get_paint_info (context);

  if (! paint_info)
    paint_info = ligma_paint_info_get_standard (ligma);

  options = g_object_new (LIGMA_TYPE_STROKE_OPTIONS,
                          "ligma",       ligma,
                          "paint-info", paint_info,
                          NULL);

  if (use_context_color)
    {
      ligma_context_define_properties (LIGMA_CONTEXT (options),
                                      LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                                      LIGMA_CONTEXT_PROP_MASK_PATTERN,
                                      FALSE);

      ligma_context_set_parent (LIGMA_CONTEXT (options), context);
    }

  return options;
}

LigmaStrokeMethod
ligma_stroke_options_get_method (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options),
                        LIGMA_STROKE_LINE);

  return GET_PRIVATE (options)->method;
}

gdouble
ligma_stroke_options_get_width (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), 1.0);

  return GET_PRIVATE (options)->width;
}

LigmaUnit
ligma_stroke_options_get_unit (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), LIGMA_UNIT_PIXEL);

  return GET_PRIVATE (options)->unit;
}

LigmaCapStyle
ligma_stroke_options_get_cap_style (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), LIGMA_CAP_BUTT);

  return GET_PRIVATE (options)->cap_style;
}

LigmaJoinStyle
ligma_stroke_options_get_join_style (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), LIGMA_JOIN_MITER);

  return GET_PRIVATE (options)->join_style;
}

gdouble
ligma_stroke_options_get_miter_limit (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), 1.0);

  return GET_PRIVATE (options)->miter_limit;
}

gdouble
ligma_stroke_options_get_dash_offset (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), 0.0);

  return GET_PRIVATE (options)->dash_offset;
}

GArray *
ligma_stroke_options_get_dash_info (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), NULL);

  return GET_PRIVATE (options)->dash_info;
}

LigmaPaintOptions *
ligma_stroke_options_get_paint_options (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), NULL);

  return GET_PRIVATE (options)->paint_options;
}

gboolean
ligma_stroke_options_get_emulate_dynamics (LigmaStrokeOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->emulate_dynamics;
}

/**
 * ligma_stroke_options_take_dash_pattern:
 * @options: a #LigmaStrokeOptions object
 * @preset: a value out of the #LigmaDashPreset enum
 * @pattern: a #GArray or %NULL if @preset is not %LIGMA_DASH_CUSTOM
 *
 * Sets the dash pattern. Either a @preset is passed and @pattern is
 * %NULL or @preset is %LIGMA_DASH_CUSTOM and @pattern is the #GArray
 * to use as the dash pattern. Note that this function takes ownership
 * of the passed pattern.
 */
void
ligma_stroke_options_take_dash_pattern (LigmaStrokeOptions *options,
                                       LigmaDashPreset     preset,
                                       GArray            *pattern)
{
  LigmaStrokeOptionsPrivate *private;

  g_return_if_fail (LIGMA_IS_STROKE_OPTIONS (options));
  g_return_if_fail (preset == LIGMA_DASH_CUSTOM || pattern == NULL);

  private = GET_PRIVATE (options);

  if (preset != LIGMA_DASH_CUSTOM)
    pattern = ligma_dash_pattern_new_from_preset (preset);

  if (private->dash_info)
    ligma_dash_pattern_free (private->dash_info);

  private->dash_info = pattern;

  g_object_notify (G_OBJECT (options), "dash-info");

  g_signal_emit (options, stroke_options_signals [DASH_INFO_CHANGED], 0,
                 preset);
}

void
ligma_stroke_options_prepare (LigmaStrokeOptions *options,
                             LigmaContext       *context,
                             LigmaPaintOptions  *paint_options)
{
  LigmaStrokeOptionsPrivate *private;

  g_return_if_fail (LIGMA_IS_STROKE_OPTIONS (options));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (paint_options == NULL ||
                    LIGMA_IS_PAINT_OPTIONS (paint_options));

  private = GET_PRIVATE (options);

  switch (private->method)
    {
    case LIGMA_STROKE_LINE:
      break;

    case LIGMA_STROKE_PAINT_METHOD:
      {
        LigmaPaintInfo *paint_info = LIGMA_CONTEXT (options)->paint_info;

        if (paint_options)
          {
            g_return_if_fail (paint_info == paint_options->paint_info);

            /*  undefine the paint-relevant context properties and get them
             *  from the passed context
             */
            ligma_context_define_properties (LIGMA_CONTEXT (paint_options),
                                            LIGMA_CONTEXT_PROP_MASK_PAINT,
                                            FALSE);
            ligma_context_set_parent (LIGMA_CONTEXT (paint_options), context);

            g_object_ref (paint_options);
          }
        else
          {
            LigmaCoreConfig      *config       = context->ligma->config;
            LigmaContextPropMask  global_props = 0;

            paint_options =
              ligma_config_duplicate (LIGMA_CONFIG (paint_info->paint_options));

            /*  FG and BG are always shared between all tools  */
            global_props |= LIGMA_CONTEXT_PROP_MASK_FOREGROUND;
            global_props |= LIGMA_CONTEXT_PROP_MASK_BACKGROUND;

            if (config->global_brush)
              global_props |= LIGMA_CONTEXT_PROP_MASK_BRUSH;
            if (config->global_dynamics)
              global_props |= LIGMA_CONTEXT_PROP_MASK_DYNAMICS;
            if (config->global_pattern)
              global_props |= LIGMA_CONTEXT_PROP_MASK_PATTERN;
            if (config->global_palette)
              global_props |= LIGMA_CONTEXT_PROP_MASK_PALETTE;
            if (config->global_gradient)
              global_props |= LIGMA_CONTEXT_PROP_MASK_GRADIENT;
            if (config->global_font)
              global_props |= LIGMA_CONTEXT_PROP_MASK_FONT;

            ligma_context_copy_properties (context,
                                          LIGMA_CONTEXT (paint_options),
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
ligma_stroke_options_finish (LigmaStrokeOptions *options)
{
  g_return_if_fail (LIGMA_IS_STROKE_OPTIONS (options));

  g_object_set (options, "paint-options", NULL, NULL);
}
