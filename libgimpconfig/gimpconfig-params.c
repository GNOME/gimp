/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpconfig-params.c
 * Copyright (C) 2008-2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gegl.h>
#include <gegl-paramspecs.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpconfig.h"


/**
 * SECTION: gimpconfig-params
 * @title: GimpConfig-params
 * @short_description: Macros and defines to install config properties.
 *
 * Macros and defines to install config properties.
 **/


static gboolean
gimp_gegl_param_spec_has_key (GParamSpec  *pspec,
                              const gchar *key,
                              const gchar *value)
{
  const gchar *v = gegl_param_spec_get_property_key (pspec, key);

  if (v && ! strcmp (v, value))
    return TRUE;

  return FALSE;
}


/**
 * gimp_config_param_spec_duplicate:
 * @pspec: the #GParamSpec to duplicate
 *
 * Creates an exact copy of @pspec, with all its properties, returns
 * %NULL if @pspec is of an unknown type that can't be duplicated.
 *
 * Return: (transfer full): The new #GParamSpec, or %NULL.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_config_param_spec_duplicate (GParamSpec *pspec)
{
  GParamSpec  *copy = NULL;
  const gchar *name;
  const gchar *nick;
  const gchar *blurb;
  GParamFlags  flags;

  g_return_val_if_fail (pspec != NULL, NULL);

  name  = pspec->name;
  nick  = g_param_spec_get_nick (pspec);
  blurb = g_param_spec_get_blurb (pspec);
  flags = pspec->flags;

  /*  this special case exists for the GEGL tool, we don't want this
   *  property serialized
   */
  if (! gimp_gegl_param_spec_has_key (pspec, "role", "output-extent"))
    flags |= GIMP_CONFIG_PARAM_SERIALIZE;

  if (G_IS_PARAM_SPEC_STRING (pspec))
    {
      GParamSpecString *spec = G_PARAM_SPEC_STRING (pspec);

      if (GEGL_IS_PARAM_SPEC_FILE_PATH (pspec))
        {
          copy = gimp_param_spec_config_path (name, nick, blurb,
                                              GIMP_CONFIG_PATH_FILE,
                                              spec->default_value,
                                              flags);
        }
      else if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (pspec))
        {
          copy = gimp_param_spec_config_path (name, nick, blurb,
                                              gimp_param_spec_config_path_type (pspec),
                                              spec->default_value,
                                              flags);
        }
      else
        {
          copy = g_param_spec_string (name, nick, blurb,
                                      spec->default_value,
                                      flags);
        }
    }
  else if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
    {
      GParamSpecBoolean *spec = G_PARAM_SPEC_BOOLEAN (pspec);

      copy = g_param_spec_boolean (name, nick, blurb,
                                   spec->default_value,
                                   flags);
    }
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      GParamSpecEnum *spec = G_PARAM_SPEC_ENUM (pspec);

      copy = g_param_spec_enum (name, nick, blurb,
                                G_TYPE_FROM_CLASS (spec->enum_class),
                                spec->default_value,
                                flags);
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (pspec))
    {
      GParamSpecDouble *spec = G_PARAM_SPEC_DOUBLE (pspec);

      if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
        {
          GeglParamSpecDouble *gspec = GEGL_PARAM_SPEC_DOUBLE (pspec);

          copy = gegl_param_spec_double (name, nick, blurb,
                                         spec->minimum,
                                         spec->maximum,
                                         spec->default_value,
                                         gspec->ui_minimum,
                                         gspec->ui_maximum,
                                         gspec->ui_gamma,
                                         flags);
          gegl_param_spec_double_set_steps (GEGL_PARAM_SPEC_DOUBLE (copy),
                                            gspec->ui_step_small,
                                            gspec->ui_step_big);
          gegl_param_spec_double_set_digits (GEGL_PARAM_SPEC_DOUBLE (copy),
                                             gspec->ui_digits);
        }
      else
        {
          copy = g_param_spec_double (name, nick, blurb,
                                      spec->minimum,
                                      spec->maximum,
                                      spec->default_value,
                                      flags);
        }
    }
  else if (G_IS_PARAM_SPEC_FLOAT (pspec))
    {
      GParamSpecFloat *spec = G_PARAM_SPEC_FLOAT (pspec);

      copy = g_param_spec_float (name, nick, blurb,
                                 spec->minimum,
                                 spec->maximum,
                                 spec->default_value,
                                 flags);
    }
  else if (G_IS_PARAM_SPEC_INT (pspec))
    {
      GParamSpecInt *spec = G_PARAM_SPEC_INT (pspec);

      if (GEGL_IS_PARAM_SPEC_INT (pspec))
        {
          GeglParamSpecInt *gspec = GEGL_PARAM_SPEC_INT (pspec);

          copy = gegl_param_spec_int (name, nick, blurb,
                                      spec->minimum,
                                      spec->maximum,
                                      spec->default_value,
                                      gspec->ui_minimum,
                                      gspec->ui_maximum,
                                      gspec->ui_gamma,
                                      flags);
          gegl_param_spec_int_set_steps (GEGL_PARAM_SPEC_INT (copy),
                                         gspec->ui_step_small,
                                         gspec->ui_step_big);
        }
      else if (GIMP_IS_PARAM_SPEC_UNIT (pspec))
        {
          GimpParamSpecUnit *spec  = GIMP_PARAM_SPEC_UNIT (pspec);
          GParamSpecInt     *ispec = G_PARAM_SPEC_INT (pspec);

          copy = gimp_param_spec_unit (name, nick, blurb,
                                       ispec->minimum == GIMP_UNIT_PIXEL,
                                       spec->allow_percent,
                                       ispec->default_value,
                                       flags);
        }
      else
        {
          copy = g_param_spec_int (name, nick, blurb,
                                   spec->minimum,
                                   spec->maximum,
                                   spec->default_value,
                                   flags);
        }
    }
  else if (G_IS_PARAM_SPEC_UINT (pspec))
    {
      GParamSpecUInt *spec = G_PARAM_SPEC_UINT (pspec);

      if (GEGL_IS_PARAM_SPEC_SEED (pspec))
        {
          GeglParamSpecSeed *gspec = GEGL_PARAM_SPEC_SEED (pspec);

          copy = gegl_param_spec_seed (name, nick, blurb,
                                       flags);

          G_PARAM_SPEC_UINT (copy)->minimum       = spec->minimum;
          G_PARAM_SPEC_UINT (copy)->maximum       = spec->maximum;
          G_PARAM_SPEC_UINT (copy)->default_value = spec->default_value;

          GEGL_PARAM_SPEC_SEED (copy)->ui_minimum = gspec->ui_minimum;
          GEGL_PARAM_SPEC_SEED (copy)->ui_maximum = gspec->ui_maximum;
        }
      else
        {
          copy = g_param_spec_uint (name, nick, blurb,
                                    spec->minimum,
                                    spec->maximum,
                                    spec->default_value,
                                    flags);
        }
    }
  else if (GIMP_IS_PARAM_SPEC_RGB (pspec))
    {
      GimpRGB color;

      gimp_param_spec_rgb_get_default (pspec, &color);

      copy = gimp_param_spec_rgb (name, nick, blurb,
                                  gimp_param_spec_rgb_has_alpha (pspec),
                                  &color,
                                  flags);
    }
  else if (GEGL_IS_PARAM_SPEC_COLOR (pspec))
    {
      GeglColor *gegl_color;
      GimpRGB    gimp_color;
      gdouble    r = 0.0;
      gdouble    g = 0.0;
      gdouble    b = 0.0;
      gdouble    a = 1.0;
      GValue     value = G_VALUE_INIT;

      g_value_init (&value, GEGL_TYPE_COLOR);
      g_param_value_set_default (pspec, &value);

      gegl_color = g_value_get_object (&value);
      if (gegl_color)
        gegl_color_get_rgba (gegl_color, &r, &g, &b, &a);

      gimp_rgba_set (&gimp_color, r, g, b, a);

      g_value_unset (&value);

      copy = gimp_param_spec_rgb (name, nick, blurb,
                                  TRUE,
                                  &gimp_color,
                                  flags);
    }
  else if (G_IS_PARAM_SPEC_PARAM (pspec))
    {
      copy = g_param_spec_param (name, nick, blurb,
                                 G_PARAM_SPEC_VALUE_TYPE (pspec),
                                 flags);
    }
  else if (GIMP_IS_PARAM_SPEC_PARASITE (pspec))
    {
      copy = gimp_param_spec_parasite (name, nick, blurb,
                                       flags);
    }
  else if (GIMP_IS_PARAM_SPEC_ARRAY (pspec))
    {
      if (GIMP_IS_PARAM_SPEC_UINT8_ARRAY (pspec))
        {
          copy = gimp_param_spec_uint8_array (name, nick, blurb,
                                              flags);
        }
      else if (GIMP_IS_PARAM_SPEC_INT16_ARRAY (pspec))
        {
          copy = gimp_param_spec_int16_array (name, nick, blurb,
                                              flags);
        }
      else if (GIMP_IS_PARAM_SPEC_INT32_ARRAY (pspec))
        {
          copy = gimp_param_spec_int32_array (name, nick, blurb,
                                              flags);
        }
      else if (GIMP_IS_PARAM_SPEC_FLOAT_ARRAY (pspec))
        {
          copy = gimp_param_spec_float_array (name, nick, blurb,
                                              flags);
        }
      else if (GIMP_IS_PARAM_SPEC_STRING_ARRAY (pspec))
        {
          copy = gimp_param_spec_string_array (name, nick, blurb,
                                               flags);
        }
      else if (GIMP_IS_PARAM_SPEC_RGB_ARRAY (pspec))
        {
          copy = gimp_param_spec_rgb_array (name, nick, blurb,
                                            flags);
        }
      else if (GIMP_IS_PARAM_SPEC_OBJECT_ARRAY (pspec))
        {
          GimpParamSpecObjectArray *spec = GIMP_PARAM_SPEC_OBJECT_ARRAY (pspec);

          copy = gimp_param_spec_object_array (name, nick, blurb,
                                               spec->object_type,
                                               flags);
        }
    }
  else if (G_IS_PARAM_SPEC_OBJECT (pspec))
    {
      GType value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (value_type == G_TYPE_FILE)
        {
          copy = g_param_spec_object (name, nick, blurb,
                                      value_type,
                                      flags);
        }
    }

  if (copy)
    {
      GQuark      quark = g_quark_from_static_string ("gegl-property-keys");
      GHashTable *keys  = g_param_spec_get_qdata (pspec, quark);

      if (keys)
        g_param_spec_set_qdata_full (copy, quark, g_hash_table_ref (keys),
                                     (GDestroyNotify) g_hash_table_unref);
    }

  return copy;
}
