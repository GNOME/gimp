/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Utitility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-utils.h"


/**
 * gimp_config_diff:
 * @a: a #GimpConfig
 * @b: another #GimpConfig of the same type as @a
 * @flags: a mask of GParamFlags
 *
 * Compares all properties of @a and @b that have all @flags set. If
 * @flags is 0, all properties are compared.
 *
 * Return value: a GList of differing GParamSpecs.
 **/
GList *
gimp_config_diff (GimpConfig      *a,
                  GimpConfig      *b,
                  GParamFlags   flags)
{
  GParamSpec **param_specs;
  guint        n_param_specs;
  gint         i;
  GList       *list = NULL;

  g_return_val_if_fail (G_IS_OBJECT (a), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (b), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b),
                        FALSE);

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (a),
                                                &n_param_specs);

  for (i = 0; i < n_param_specs; i++)
    {
      if (! flags || ((param_specs[i]->flags & flags) == flags))
        {
          GValue a_value = { 0, };
          GValue b_value = { 0, };

          g_value_init (&a_value, param_specs[i]->value_type);
          g_value_init (&b_value, param_specs[i]->value_type);

          g_object_get_property (G_OBJECT (a), param_specs[i]->name, &a_value);
          g_object_get_property (G_OBJECT (b), param_specs[i]->name, &b_value);

          if (G_IS_PARAM_SPEC_OBJECT (param_specs[i]) &&
              (param_specs[i]->flags & GIMP_PARAM_AGGREGATE))
            {
              GimpConfig *a_object = g_value_get_object (&a_value);
              GimpConfig *b_object = g_value_get_object (&b_value);

              if (a_object && b_object &&
                  G_TYPE_FROM_INSTANCE (a_object) ==
                  G_TYPE_FROM_INSTANCE (b_object))
                {
                  GList *diff = gimp_config_diff (a_object, b_object, flags);

                  if (diff)
                    {
                      g_list_free (diff);
                      list = g_list_prepend (list, param_specs[i]);
                    }
                }
              else
                {
                  list = g_list_prepend (list, param_specs[i]);
                }
            }
          else
            {

              if (g_param_values_cmp (param_specs[i], &a_value, &b_value))
                list = g_list_prepend (list, param_specs[i]);
            }

          g_value_unset (&a_value);
          g_value_unset (&b_value);
        }
    }

  g_free (param_specs);

  return g_list_reverse (list);
}


static void
gimp_config_connect_notify (GObject    *src,
                            GParamSpec *param_spec,
                            GObject    *dest)
{
  if ((param_spec->flags & G_PARAM_READABLE) &&
      (param_spec->flags & G_PARAM_WRITABLE) &&
      ! (param_spec->flags & G_PARAM_CONSTRUCT_ONLY))
    {
      GValue value = { 0, };

      g_value_init (&value, param_spec->value_type);

      g_object_get_property (src,  param_spec->name, &value);
      g_object_set_property (dest, param_spec->name, &value);

      g_value_unset (&value);
    }
}

/**
 * gimp_config_connect:
 * @src: a #GObject
 * @dest: another #GObject of the same type as @src
 *
 * Connects @dest with @src so that all property changes of @src are
 * applied to @dest using a "notify" handler.
 **/
void
gimp_config_connect (GObject *src,
                     GObject *dest)
{
  g_return_if_fail (G_IS_OBJECT (src));
  g_return_if_fail (G_IS_OBJECT (dest));
  g_return_if_fail (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest));

  g_signal_connect_object (src, "notify",
                           G_CALLBACK (gimp_config_connect_notify),
                           dest, 0);
}

/**
 * gimp_config_disconnect:
 * @src: a #GObject
 * @dest: another #GObject of the same type as @src
 *
 * Removes a connection between @dest and @src that was previously set
 * up using gimp_config_connect().
 **/
void
gimp_config_disconnect (GObject *src,
                        GObject *dest)
{
  g_return_if_fail (G_IS_OBJECT (src));
  g_return_if_fail (G_IS_OBJECT (dest));

  g_signal_handlers_disconnect_by_func (src,
                                        G_CALLBACK (gimp_config_connect_notify),
                                        dest);
}

/**
 * gimp_config_copy_properties:
 * @src: a #GimpConfig
 * @dest: another #GimpConfig of the same type as @src
 *
 * Retrieves all read and writeable property settings from @src and
 * applies the values to @dest.
 **/
void
gimp_config_copy_properties (GimpConfig *src,
                             GimpConfig *dest)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;

  g_return_if_fail (GIMP_IS_CONFIG (src));
  g_return_if_fail (GIMP_IS_CONFIG (dest));
  g_return_if_fail (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest));

  klass = G_OBJECT_GET_CLASS (src);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return;

  g_object_freeze_notify (G_OBJECT (dest));

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec;

      prop_spec = property_specs[i];

      if ((prop_spec->flags & G_PARAM_READABLE) &&
	  (prop_spec->flags & G_PARAM_WRITABLE) &&
          ! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
	{
          if (G_IS_PARAM_SPEC_OBJECT (prop_spec) &&
              (prop_spec->flags & GIMP_PARAM_AGGREGATE))
            {
              GValue      src_value  = { 0, };
              GValue      dest_value = { 0, };
              GimpConfig *src_object;
              GimpConfig *dest_object;

              g_value_init (&src_value,  prop_spec->value_type);
              g_value_init (&dest_value, prop_spec->value_type);

              g_object_get_property (G_OBJECT (src),
                                     prop_spec->name, &src_value);
              g_object_get_property (G_OBJECT (dest),
                                     prop_spec->name, &dest_value);

              src_object  = g_value_get_object (&src_value);
              dest_object = g_value_get_object (&dest_value);

              if (src_object && dest_object &&
                  G_TYPE_FROM_INSTANCE (src_object) ==
                  G_TYPE_FROM_INSTANCE (dest_object))
                {
                  gimp_config_copy_properties (src_object, dest_object);
                }

              g_value_unset (&src_value);
              g_value_unset (&dest_value);
            }
          else
            {
              GValue value = { 0, };

              g_value_init (&value, prop_spec->value_type);

              g_object_get_property (G_OBJECT (src),  prop_spec->name, &value);
              g_object_set_property (G_OBJECT (dest), prop_spec->name, &value);

              g_value_unset (&value);
            }
        }
    }

  g_free (property_specs);

  g_object_thaw_notify (G_OBJECT (dest));
}

/**
 * gimp_config_reset_properties:
 * @config: a #GimpConfig
 *
 * Resets all writable properties of @object to the default values as
 * defined in their #GParamSpec.
 **/
void
gimp_config_reset_properties (GimpConfig *config)
{
  GObject       *object;
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  GValue         value = { 0, };
  guint          n_property_specs;
  guint          i;

  g_return_if_fail (GIMP_IS_CONFIG (config));

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return;

  object = G_OBJECT (config);

  g_object_freeze_notify (object);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec;

      prop_spec = property_specs[i];

      if ((prop_spec->flags & G_PARAM_WRITABLE) &&
          ! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
        {
          if (G_IS_PARAM_SPEC_OBJECT (prop_spec))
            {
              if ((prop_spec->flags & GIMP_PARAM_SERIALIZE) &&
                  (prop_spec->flags & GIMP_PARAM_AGGREGATE) &&
                  g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                         GIMP_TYPE_CONFIG))
                {
                  g_value_init (&value, prop_spec->value_type);

                  g_object_get_property (object, prop_spec->name, &value);

                  gimp_config_reset (GIMP_CONFIG (g_value_get_object (&value)));

                  g_value_unset (&value);
                }
            }
          else
            {
              g_value_init (&value, prop_spec->value_type);
              g_param_value_set_default (prop_spec, &value);

              g_object_set_property (object, prop_spec->name, &value);

              g_value_unset (&value);
            }
        }
    }

  g_free (property_specs);

  g_object_thaw_notify (object);
}


/*
 * GimpConfig string utilities
 */

/**
 * gimp_config_string_append_escaped:
 * @string: pointer to a #GString
 * @val: a string to append or %NULL
 *
 * Escapes and quotes @val and appends it to @string. The escape
 * algorithm is different from the one used by g_strescape() since it
 * leaves non-ASCII characters intact and thus preserves UTF-8
 * strings. Only control characters and quotes are being escaped.
 **/
void
gimp_config_string_append_escaped (GString     *string,
                                   const gchar *val)
{
  g_return_if_fail (string != NULL);

  if (val)
    {
      const guchar *p;
      gchar         buf[4] = { '\\', 0, 0, 0 };
      gint          len;

      g_string_append_c (string, '\"');

      for (p = val, len = 0; *p; p++)
        {
          if (*p < ' ' || *p == '\\' || *p == '\"')
            {
              g_string_append_len (string, val, len);

              len = 2;
              switch (*p)
                {
                case '\b':
                  buf[1] = 'b';
                  break;
                case '\f':
                  buf[1] = 'f';
                  break;
                case '\n':
                  buf[1] = 'n';
                  break;
                case '\r':
                  buf[1] = 'r';
                  break;
                case '\t':
                  buf[1] = 't';
                  break;
                case '\\':
                case '"':
                  buf[1] = *p;
                  break;

                default:
                  len = 4;
                  buf[1] = '0' + (((*p) >> 6) & 07);
                  buf[2] = '0' + (((*p) >> 3) & 07);
                  buf[3] = '0' + ((*p) & 07);
                  break;
                }

              g_string_append_len (string, buf, len);

              val = p + 1;
              len = 0;
            }
          else
            {
              len++;
            }
        }

      g_string_append_len (string, val, len);
      g_string_append_c   (string, '\"');
    }
  else
    {
      g_string_append_len (string, "\"\"", 2);
    }
}


/*
 * GimpConfig path utilities
 */

gchar *
gimp_config_build_data_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${gimp_data_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}

gchar *
gimp_config_build_plug_in_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${gimp_plug_in_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}
