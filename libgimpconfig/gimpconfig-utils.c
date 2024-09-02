/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Utility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-params.h"
#include "gimpconfig-utils.h"


/**
 * SECTION: gimpconfig-utils
 * @title: GimpConfig-utils
 * @short_description: Miscellaneous utility functions for libgimpconfig.
 *
 * Miscellaneous utility functions for libgimpconfig.
 **/


static gboolean
gimp_config_diff_property (GObject    *a,
                           GObject    *b,
                           GParamSpec *prop_spec)
{
  GValue    a_value = G_VALUE_INIT;
  GValue    b_value = G_VALUE_INIT;
  gboolean  retval  = FALSE;

  g_value_init (&a_value, prop_spec->value_type);
  g_value_init (&b_value, prop_spec->value_type);

  g_object_get_property (a, prop_spec->name, &a_value);
  g_object_get_property (b, prop_spec->name, &b_value);

  /* TODO: temporary hack to handle case of NULL GeglColor in a param value.
   * This got fixed in commit c0477bcb0 which should be available for GEGL
   * 0.4.50. In the meantime, this will do.
   */
  if (GEGL_IS_PARAM_SPEC_COLOR (prop_spec) &&
      (! g_value_get_object (&a_value) ||
       ! g_value_get_object (&b_value)))
    {
      retval = (g_value_get_object (&a_value) != g_value_get_object (&b_value));
    }
  else if (g_param_values_cmp (prop_spec, &a_value, &b_value))
    {
      if ((prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE) &&
          G_IS_PARAM_SPEC_OBJECT (prop_spec)               &&
          g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                 GIMP_TYPE_CONFIG))
        {
          if (! gimp_config_is_equal_to (g_value_get_object (&a_value),
                                         g_value_get_object (&b_value)))
            {
              retval = TRUE;
            }
        }
      else
        {
          retval = TRUE;
        }
    }

  g_value_unset (&a_value);
  g_value_unset (&b_value);

  return retval;
}

static GList *
gimp_config_diff_same (GObject     *a,
                       GObject     *b,
                       GParamFlags  flags)
{
  GParamSpec **param_specs;
  guint        n_param_specs;
  guint        i;
  GList       *list = NULL;

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (a),
                                                &n_param_specs);

  for (i = 0; i < n_param_specs; i++)
    {
      GParamSpec *prop_spec = param_specs[i];

      if (! flags || ((prop_spec->flags & flags) == flags))
        {
          if (gimp_config_diff_property (a, b, prop_spec))
            list = g_list_prepend (list, prop_spec);
        }
    }

  g_free (param_specs);

  return list;
}

static GList *
gimp_config_diff_other (GObject     *a,
                        GObject     *b,
                        GParamFlags  flags)
{
  GParamSpec **param_specs;
  guint        n_param_specs;
  guint        i;
  GList       *list = NULL;

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (a),
                                                &n_param_specs);

  for (i = 0; i < n_param_specs; i++)
    {
      GParamSpec *a_spec = param_specs[i];
      GParamSpec *b_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (b),
                                                         a_spec->name);

      if (b_spec &&
          (a_spec->value_type == b_spec->value_type) &&
          (! flags || (a_spec->flags & b_spec->flags & flags) == flags))
        {
          if (gimp_config_diff_property (a, b, b_spec))
            list = g_list_prepend (list, b_spec);
        }
    }

  g_free (param_specs);

  return list;
}


/**
 * gimp_config_diff:
 * @a: a #GObject
 * @b: another #GObject object
 * @flags: a mask of GParamFlags
 *
 * Compares all properties of @a and @b that have all @flags set. If
 * @flags is 0, all properties are compared.
 *
 * If the two objects are not of the same type, only properties that
 * exist in both object classes and are of the same value_type are
 * compared.
 *
 * Returns: (transfer container) (element-type GParamSpec): a GList of differing GParamSpecs.
 *
 * Since: 2.4
 **/
GList *
gimp_config_diff (GObject     *a,
                  GObject     *b,
                  GParamFlags  flags)
{
  GList *diff;

  g_return_val_if_fail (G_IS_OBJECT (a), NULL);
  g_return_val_if_fail (G_IS_OBJECT (b), NULL);

  if (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b))
    diff = gimp_config_diff_same (a, b, flags);
  else
    diff = gimp_config_diff_other (a, b, flags);

  return g_list_reverse (diff);
}

/**
 * gimp_config_sync:
 * @src: a #GObject
 * @dest: another #GObject
 * @flags: a mask of GParamFlags
 *
 * Compares all read- and write-able properties from @src and @dest
 * that have all @flags set. Differing values are then copied from
 * @src to @dest. If @flags is 0, all differing read/write properties.
 *
 * Properties marked as "construct-only" are not touched.
 *
 * If the two objects are not of the same type, only properties that
 * exist in both object classes and are of the same value_type are
 * synchronized
 *
 * Returns: %TRUE if @dest was modified, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
gimp_config_sync (GObject     *src,
                  GObject     *dest,
                  GParamFlags  flags)
{
  GList *diff;
  GList *list;

  g_return_val_if_fail (G_IS_OBJECT (src), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (dest), FALSE);

  /* we use the internal versions here for a number of reasons:
   *  - it saves a g_list_reverse()
   *  - it avoids duplicated parameter checks
   */
  if (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest))
    diff = gimp_config_diff_same (src, dest, (flags | G_PARAM_READWRITE));
  else
    diff = gimp_config_diff_other (src, dest, flags);

  if (!diff)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (dest));

  for (list = diff; list; list = list->next)
    {
      GParamSpec *prop_spec = list->data;

      if (! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, prop_spec->value_type);

          g_object_get_property (src,  prop_spec->name, &value);
          g_object_set_property (dest, prop_spec->name, &value);

          g_value_unset (&value);
        }
    }

  g_object_thaw_notify (G_OBJECT (dest));

  g_list_free (diff);

  return TRUE;
}

/**
 * gimp_config_reset_properties:
 * @object: a #GObject
 *
 * Resets all writable properties of @object to the default values as
 * defined in their #GParamSpec. Properties marked as "construct-only"
 * are not touched.
 *
 * If you want to reset a #GimpConfig object, please use gimp_config_reset().
 *
 * Since: 2.4
 **/
void
gimp_config_reset_properties (GObject *object)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;

  g_return_if_fail (G_IS_OBJECT (object));

  klass = G_OBJECT_GET_CLASS (object);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);
  if (!property_specs)
    return;

  g_object_freeze_notify (object);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec;
      GValue      value = G_VALUE_INIT;

      prop_spec = property_specs[i];

      if ((prop_spec->flags & G_PARAM_WRITABLE) &&
          ! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
        {
          if (G_IS_PARAM_SPEC_OBJECT (prop_spec)        &&
              ! GIMP_IS_PARAM_SPEC_OBJECT (prop_spec)   &&
              g_type_class_peek (prop_spec->value_type) &&
              g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                     GIMP_TYPE_CONFIG))
            {
              if ((prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE) &&
                  (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE))
                {
                  g_value_init (&value, prop_spec->value_type);

                  g_object_get_property (object, prop_spec->name, &value);

                  gimp_config_reset (g_value_get_object (&value));

                  g_value_unset (&value);
                }
            }
          else
            {
              GValue default_value = G_VALUE_INIT;

              g_value_init (&default_value, prop_spec->value_type);
              g_value_init (&value,         prop_spec->value_type);

              g_param_value_set_default (prop_spec, &default_value);
              g_object_get_property (object, prop_spec->name, &value);

              if (g_param_values_cmp (prop_spec, &default_value, &value))
                {
                  g_object_set_property (object, prop_spec->name,
                                         &default_value);
                }

              g_value_unset (&value);
              g_value_unset (&default_value);
            }
        }
    }

  g_object_thaw_notify (object);

  g_free (property_specs);
}


/**
 * gimp_config_reset_property:
 * @object: a #GObject
 * @property_name: name of the property to reset
 *
 * Resets the property named @property_name to its default value.  The
 * property must be writable and must not be marked as "construct-only".
 *
 * Since: 2.4
 **/
void
gimp_config_reset_property (GObject     *object,
                            const gchar *property_name)
{
  GObjectClass  *klass;
  GParamSpec    *prop_spec;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);

  klass = G_OBJECT_GET_CLASS (object);

  prop_spec = g_object_class_find_property (klass, property_name);

  if (!prop_spec)
    return;

  if ((prop_spec->flags & G_PARAM_WRITABLE) &&
      ! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
    {
      GValue  value = G_VALUE_INIT;

      if (G_IS_PARAM_SPEC_OBJECT (prop_spec))
        {
          if ((prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE) &&
              (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE) &&
              g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                     GIMP_TYPE_CONFIG))
            {
              g_value_init (&value, prop_spec->value_type);

              g_object_get_property (object, prop_spec->name, &value);

              gimp_config_reset (g_value_get_object (&value));

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
 *
 * Since: 2.4
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

      for (p = (const guchar *) val, len = 0; *p; p++)
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

              val = (const gchar *) p + 1;
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
