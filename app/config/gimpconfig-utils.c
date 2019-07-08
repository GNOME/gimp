/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Utitility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimpconfig-utils.h"


static void
gimp_config_connect_notify (GObject    *src,
                            GParamSpec *param_spec,
                            GObject    *dest)
{
  if (param_spec->flags & G_PARAM_READABLE)
    {
      GParamSpec *dest_spec;

      dest_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dest),
                                                param_spec->name);

      if (dest_spec                                         &&
          (dest_spec->value_type == param_spec->value_type) &&
          (dest_spec->flags & G_PARAM_WRITABLE)             &&
          (dest_spec->flags & G_PARAM_CONSTRUCT_ONLY) == 0)
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, param_spec->value_type);

          g_object_get_property (src,  param_spec->name, &value);

          g_signal_handlers_block_by_func (dest,
                                           gimp_config_connect_notify, src);
          g_object_set_property (dest, param_spec->name, &value);
          g_signal_handlers_unblock_by_func (dest,
                                             gimp_config_connect_notify, src);

          g_value_unset (&value);
        }
    }
}

/**
 * gimp_config_connect:
 * @a: a #GObject
 * @b: another #GObject
 * @property_name: the name of a property to connect or %NULL for all
 *
 * Connects the two object @a and @b in a way that property changes of
 * one are propagated to the other. This is a two-way connection.
 *
 * If @property_name is %NULL the connection is setup for all
 * properties. It is not required that @a and @b are of the same type.
 * Only changes on properties that exist in both object classes and
 * are of the same value_type are propagated.
 **/
void
gimp_config_connect (GObject     *a,
                     GObject     *b,
                     const gchar *property_name)
{
  gchar *signal_name;

  g_return_if_fail (a != b);
  g_return_if_fail (G_IS_OBJECT (a) && G_IS_OBJECT (b));

  if (property_name)
    signal_name = g_strconcat ("notify::", property_name, NULL);
  else
    signal_name = "notify";

  g_signal_connect_object (a, signal_name,
                           G_CALLBACK (gimp_config_connect_notify),
                           b, 0);
  g_signal_connect_object (b, signal_name,
                           G_CALLBACK (gimp_config_connect_notify),
                           a, 0);

  if (property_name)
    g_free (signal_name);
}

static void
gimp_config_connect_full_notify (GObject    *src,
                                 GParamSpec *param_spec,
                                 GObject    *dest)
{
  if (param_spec->flags & G_PARAM_READABLE)
    {
      gchar      *attach_key;
      gchar      *dest_prop_name;
      GParamSpec *dest_spec = NULL;

      attach_key = g_strdup_printf ("%p-%s", dest, param_spec->name);
      dest_prop_name = g_object_get_data (src, attach_key);
      g_free (attach_key);

      if (dest_prop_name)
        dest_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dest),
                                                  dest_prop_name);

      if (dest_spec                                         &&
          (dest_spec->value_type == param_spec->value_type) &&
          (dest_spec->flags & G_PARAM_WRITABLE)             &&
          (dest_spec->flags & G_PARAM_CONSTRUCT_ONLY) == 0)
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, param_spec->value_type);

          g_object_get_property (src,  param_spec->name, &value);

          g_signal_handlers_block_by_func (dest,
                                           gimp_config_connect_full_notify, src);
          g_object_set_property (dest, dest_prop_name, &value);
          g_signal_handlers_unblock_by_func (dest,
                                             gimp_config_connect_full_notify, src);

          g_value_unset (&value);
        }
    }
}

/**
 * gimp_config_connect_full:
 * @a: a #GObject
 * @b: another #GObject
 * @property_name_a: the name of a property of @a to connect
 * @property_name_b: the name of a property of @b to connect
 *
 * Connects the two object @a and @b in a way that property changes of
 * one are propagated to the other. This is a two-way connection.
 *
 * If @property_name is %NULL the connection is setup for all
 * properties. It is not required that @a and @b are of the same type.
 * Only changes on properties that exist in both object classes and
 * are of the same value_type are propagated.
 **/
void
gimp_config_connect_full (GObject     *a,
                          GObject     *b,
                          const gchar *property_name_a,
                          const gchar *property_name_b)
{
  gchar *signal_name;
  gchar *attach_key;

  g_return_if_fail (a != b);
  g_return_if_fail (G_IS_OBJECT (a) && G_IS_OBJECT (b));
  g_return_if_fail (property_name_a != NULL);
  g_return_if_fail (property_name_b != NULL);

  signal_name = g_strconcat ("notify::", property_name_a, NULL);
  attach_key  = g_strdup_printf ("%p-%s", b, property_name_a);

  g_signal_connect_object (a, signal_name,
                           G_CALLBACK (gimp_config_connect_full_notify),
                           b, 0);
  g_object_set_data_full (a, attach_key, g_strdup (property_name_b),
                          (GDestroyNotify) g_free);

  g_free (signal_name);
  g_free (attach_key);

  signal_name = g_strconcat ("notify::", property_name_b, NULL);
  attach_key  = g_strdup_printf ("%p-%s", a, property_name_b);

  g_signal_connect_object (b, signal_name,
                           G_CALLBACK (gimp_config_connect_full_notify),
                           a, 0);
  g_object_set_data_full (b, attach_key, g_strdup (property_name_a),
                          (GDestroyNotify) g_free);

  g_free (signal_name);
  g_free (attach_key);
}

/**
 * gimp_config_disconnect:
 * @a: a #GObject
 * @b: another #GObject
 *
 * Removes a connection between @dest and @src that was previously set
 * up using gimp_config_connect().
 **/
void
gimp_config_disconnect (GObject *a,
                        GObject *b)
{
  g_return_if_fail (G_IS_OBJECT (a) && G_IS_OBJECT (b));

  g_signal_handlers_disconnect_by_func (b,
                                        G_CALLBACK (gimp_config_connect_notify),
                                        a);
  g_signal_handlers_disconnect_by_func (a,
                                        G_CALLBACK (gimp_config_connect_notify),
                                        b);
}

