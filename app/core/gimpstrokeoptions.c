/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokeoptions.c
 * Copyright (C) 2003 Simon Budig
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig-params.h"

#include "gimpmarshal.h"
#include "gimpstrokeoptions.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_WIDTH,
  PROP_UNIT,
  PROP_CAP_STYLE,
  PROP_JOIN_STYLE,
  PROP_MITER,
  PROP_ANTIALIAS,
  PROP_DASH_UNIT,
  PROP_DASH_OFFSET,
  PROP_DASH_INFO
};

enum
{
  DASH_INFO_CHANGED,
  LAST_SIGNAL
};


static void   gimp_stroke_options_class_init   (GimpStrokeOptionsClass *klass);

static void   gimp_stroke_options_set_property (GObject         *object,
                                                guint            property_id,
                                                const GValue    *value,
                                                GParamSpec      *pspec);
static void   gimp_stroke_options_get_property (GObject         *object,
                                                guint            property_id,
                                                GValue          *value,
                                                GParamSpec      *pspec);

static guint  stroke_options_signals[LAST_SIGNAL] = { 0 };

static GimpContextClass *parent_class = NULL;


GType
gimp_stroke_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpStrokeOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_stroke_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpStrokeOptions),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      type = g_type_register_static (GIMP_TYPE_CONTEXT,
                                     "GimpStrokeOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_stroke_options_class_init (GimpStrokeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *array_spec;

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_stroke_options_set_property;
  object_class->get_property = gimp_stroke_options_get_property;

  klass->dash_info_changed = NULL;

  stroke_options_signals[DASH_INFO_CHANGED] =
    g_signal_new ("dash_info_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpStrokeOptionsClass, dash_info_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DASH_PRESET);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_STYLE,
                                 "style", NULL,
                                 GIMP_TYPE_STROKE_STYLE,
                                 GIMP_STROKE_STYLE_SOLID,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_WIDTH,
                                   "width", NULL,
                                   0.0, 2000.0, 6.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT,
				 "unit", NULL,
				 TRUE, FALSE, GIMP_UNIT_PIXEL,
				 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CAP_STYLE,
                                 "cap-style", NULL,
                                 GIMP_TYPE_CAP_STYLE, GIMP_CAP_BUTT,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_JOIN_STYLE,
                                 "join-style", NULL,
                                 GIMP_TYPE_JOIN_STYLE, GIMP_JOIN_MITER,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_MITER,
                                   "miter", NULL,
                                   0.0, 100.0, 10.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                                    "antialias", NULL,
                                    TRUE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_DASH_OFFSET,
                                   "dash-offset", NULL,
                                   0.0, 2000.0, 0.0,
                                   0);

  array_spec = g_param_spec_double ("dash-length", NULL, NULL,
                                    0.0, 2000.0, 1.0, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DASH_INFO,
                                   g_param_spec_value_array ("dash-info",
                                                             NULL, NULL,
                                                             array_spec,
                                                             GIMP_CONFIG_PARAM_FLAGS));

}

static void
gimp_stroke_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpStrokeOptions *options = GIMP_STROKE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_STYLE:
      options->style = g_value_get_enum (value);
      break;
    case PROP_WIDTH:
      options->width = g_value_get_double (value);
      break;
    case PROP_UNIT:
      options->unit = g_value_get_int (value);
      break;
    case PROP_CAP_STYLE:
      options->cap_style = g_value_get_enum (value);
      break;
    case PROP_JOIN_STYLE:
      options->join_style = g_value_get_enum (value);
      break;
    case PROP_MITER:
      options->miter = g_value_get_double (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_DASH_OFFSET:
      options->dash_offset = g_value_get_double (value);
      break;
    case PROP_DASH_INFO:
      {
        GValueArray *val_array;
        GValue      *item;
        gint         i;
        gdouble      val;

        if (options->dash_info)
          g_array_free (options->dash_info, TRUE);

        val_array = g_value_get_boxed (value);
        if (val_array == NULL || val_array->n_values == 0)
          {
            options->dash_info = NULL;
          }
        else
          {
            options->dash_info = g_array_sized_new (FALSE, FALSE,
                                                    sizeof (gdouble),
                                                    val_array->n_values);

            for (i=0; i < val_array->n_values; i++)
              {
                item = g_value_array_get_nth (val_array, i);

                g_return_if_fail (G_VALUE_HOLDS_DOUBLE (item));

                val = g_value_get_double (item);

                options->dash_info = g_array_append_val (options->dash_info,
                                                         val);
              }
          }
        g_signal_emit (options,
                       stroke_options_signals [DASH_INFO_CHANGED], 0,
                       GIMP_DASH_CUSTOM);
      }
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
  GimpStrokeOptions *options = GIMP_STROKE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, options->style);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, options->width);
      break;
    case PROP_UNIT:
      g_value_set_int (value, options->unit);
      break;
    case PROP_CAP_STYLE:
      g_value_set_enum (value, options->cap_style);
      break;
    case PROP_JOIN_STYLE:
      g_value_set_enum (value, options->join_style);
      break;
    case PROP_MITER:
      g_value_set_double (value, options->miter);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_DASH_OFFSET:
      g_value_set_double (value, options->dash_offset);
      break;
    case PROP_DASH_INFO:
      {
        GValueArray *val_array;
        GValue       item = { 0, };
        gint         i;

        g_value_init (&item, G_TYPE_DOUBLE);

        if (options->dash_info == NULL || options->dash_info->len == 0)
          {
            g_value_set_boxed (value, NULL);
          }
        else
          {
            val_array = g_value_array_new (options->dash_info->len);

            for (i=0; i < options->dash_info->len; i++)
              {
                g_value_set_double (&item, g_array_index (options->dash_info,
                                                          gdouble,
                                                          i));

                g_value_array_append (val_array, &item);
              }

            g_value_set_boxed (value, val_array);
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_stroke_options_set_dash_preset (GimpStrokeOptions *options,
                                     GimpDashPreset     preset)
{
  GArray *new_pattern;
  gdouble dash;
  gint    i;

  if (preset == GIMP_DASH_CUSTOM)
    return;

  new_pattern = g_array_new (FALSE, FALSE, sizeof (gdouble));

  switch (preset)
    {
      case GIMP_DASH_LINE:
        break;
      case GIMP_DASH_LONG_DASH:
        dash = 9.0; g_array_append_val (new_pattern, dash);
        dash = 3.0; g_array_append_val (new_pattern, dash);
        break;
      case GIMP_DASH_MEDIUM_DASH:
        dash = 6.0; g_array_append_val (new_pattern, dash);
        dash = 6.0; g_array_append_val (new_pattern, dash);
        break;
      case GIMP_DASH_SHORT_DASH:
        dash = 3.0; g_array_append_val (new_pattern, dash);
        dash = 9.0; g_array_append_val (new_pattern, dash);
        break;
      case GIMP_DASH_SPARSE_DOTS:
        for (i = 0; i < 2; i++)
          {
            dash = 1.0; g_array_append_val (new_pattern, dash);
            dash = 5.0; g_array_append_val (new_pattern, dash);
          }
        break;
      case GIMP_DASH_NORMAL_DOTS:
        for (i = 0; i < 3; i++)
          {
            dash = 1.0; g_array_append_val (new_pattern, dash);
            dash = 3.0; g_array_append_val (new_pattern, dash);
          }
        break;
      case GIMP_DASH_DENSE_DOTS:
        for (i = 0; i < 12; i++)
          {
            dash = 1.0; g_array_append_val (new_pattern, dash);
          }
        break;
      case GIMP_DASH_STIPPLES:
        for (i = 0; i < 24; i++)
          {
            dash = 0.5; g_array_append_val (new_pattern, dash);
          }
        break;
      case GIMP_DASH_DASH_DOT:
        dash = 7.0; g_array_append_val (new_pattern, dash);
        dash = 2.0; g_array_append_val (new_pattern, dash);
        dash = 1.0; g_array_append_val (new_pattern, dash);
        dash = 2.0; g_array_append_val (new_pattern, dash);
        break;
      case GIMP_DASH_DASH_DOT_DOT:
        dash = 7.0; g_array_append_val (new_pattern, dash);
        for (i=0; i < 5; i++)
          {
            dash = 1.0; g_array_append_val (new_pattern, dash);
          }
        break;
      default:
        g_printerr ("Unknown Dash pattern: %d\n", preset);
    }

  if (options->dash_info != NULL)
    g_array_free (options->dash_info, TRUE);
  options->dash_info = NULL;

  if (new_pattern->len >= 2)
    options->dash_info = new_pattern;
  else
    g_array_free (new_pattern, TRUE);

  g_signal_emit (options,
                 stroke_options_signals [DASH_INFO_CHANGED], 0,
                 preset);
  g_object_notify (G_OBJECT (options), "dash-info");
}
