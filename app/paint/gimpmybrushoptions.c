/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifdef HAVE_LIBMYPAINT

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimppaintinfo.h"

#include "gimpmybrushoptions.h"

#include "gimp-intl.h"

#include <mypaint-brush.h>

enum
{
  PROP_0,
  PROP_RADIUS,
  PROP_OPAQUE,
  PROP_HARDNESS,
  PROP_MYBRUSH
};

typedef struct
{
  gdouble           radius;
  gdouble           opaque;
  gdouble           hardness;
  gchar            *brush_json;
} OptionsState;

static GHashTable *loaded_myb;

static void   gimp_mybrush_options_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_mybrush_options_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);
static void   options_state_free (gpointer options)
{
  OptionsState *state = options;
  g_free (state->brush_json);
  g_free (state);
}


G_DEFINE_TYPE (GimpMybrushOptions, gimp_mybrush_options, GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_mybrush_options_class_init (GimpMybrushOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_mybrush_options_set_property;
  object_class->get_property = gimp_mybrush_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_RADIUS,
                                   "radius", _("Radius"),
                                   -2.0, 6.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_OPAQUE,
                                   "opaque", _("Base Opacity"),
                                   0.0, 2.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HARDNESS,
                                   "hardness", NULL,
                                   0.0, 1.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_MYBRUSH,
                                   "mybrush", NULL,
                                   NULL,
                                   GIMP_PARAM_STATIC_STRINGS);

  loaded_myb = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, options_state_free);
}

static void
gimp_mybrush_options_load_path (GObject    *object,
                                gchar      *path)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (object);

  OptionsState *state = g_hash_table_lookup (loaded_myb, path);
  if (!state)
    {
      gchar        *brush_json = NULL;
      MyPaintBrush *brush = mypaint_brush_new ();

      state = g_new0 (OptionsState, 1);
      mypaint_brush_from_defaults (brush);

      if (g_file_get_contents (path, &brush_json, NULL, NULL))
        {
          if (! mypaint_brush_from_string (brush, brush_json))
            {
              g_printerr ("Failed to deserialize MyPaint brush\n");
              g_free (brush_json);
              brush_json = NULL;
            }
        }

      state->brush_json = brush_json;
      state->radius = mypaint_brush_get_base_value (brush, MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC);
      state->opaque = mypaint_brush_get_base_value (brush, MYPAINT_BRUSH_SETTING_OPAQUE);
      state->hardness = mypaint_brush_get_base_value (brush, MYPAINT_BRUSH_SETTING_HARDNESS);

      g_hash_table_insert (loaded_myb, g_strdup(path), state);
    }

  options->radius = state->radius;
  options->opaque = state->opaque;
  options->hardness = state->hardness;

  g_object_notify (object, "radius");
  g_object_notify (object, "opaque");
  g_object_notify (object, "hardness");
}

static void
gimp_mybrush_options_init (GimpMybrushOptions *options)
{
}

static void
gimp_mybrush_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RADIUS:
      options->radius = g_value_get_double (value);
      break;
    case PROP_HARDNESS:
      options->hardness = g_value_get_double (value);
      break;
    case PROP_OPAQUE:
      options->opaque = g_value_get_double (value);
      break;
    case PROP_MYBRUSH:
      g_free (options->mybrush);
      options->mybrush = g_value_dup_string (value);
      if (options->mybrush)
        gimp_mybrush_options_load_path (object, options->mybrush);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mybrush_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RADIUS:
      g_value_set_double (value, options->radius);
      break;
    case PROP_OPAQUE:
      g_value_set_double (value, options->opaque);
      break;
    case PROP_HARDNESS:
      g_value_set_double (value, options->hardness);
      break;
    case PROP_MYBRUSH:
      g_value_set_string (value, options->mybrush);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

const gchar *
gimp_mybrush_options_get_brush_data (GimpMybrushOptions *options)
{
  OptionsState *state = g_hash_table_lookup (loaded_myb, options->mybrush);
  if (state)
    return state->brush_json;
  return NULL;
}

#endif
