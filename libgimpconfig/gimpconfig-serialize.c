/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Object properties serialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-params.h"
#include "gimpconfig-path.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-utils.h"


/**
 * SECTION: gimpconfig-serialize
 * @title: GimpConfig-serialize
 * @short_description: Serializing for libgimpconfig.
 *
 * Serializing interface for libgimpconfig.
 **/


static gboolean gimp_config_serialize_strv  (const GValue *value,
                                             GString      *str);
static gboolean gimp_config_serialize_array (const GValue *value,
                                             GString      *str);


/**
 * gimp_config_serialize_properties:
 * @config: a #GimpConfig.
 * @writer: a #GimpConfigWriter.
 *
 * This function writes all object properties to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
gimp_config_serialize_properties (GimpConfig       *config,
                                  GimpConfigWriter *writer)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  gboolean       success = TRUE;

  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return success;

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
        continue;

      /* Some properties may fail writing, which shouldn't break serializing
       * more properties, yet final result would be a (partial) failure.
       */
      if (! gimp_config_serialize_property (config, prop_spec, writer))
        success = FALSE;
    }

  g_free (property_specs);

  return success;
}

/**
 * gimp_config_serialize_changed_properties:
 * @config: a #GimpConfig.
 * @writer: a #GimpConfigWriter.
 *
 * This function writes all object properties that have been changed from
 * their default values to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
gimp_config_serialize_changed_properties (GimpConfig       *config,
                                          GimpConfigWriter *writer)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  GValue         value   = G_VALUE_INIT;
  gboolean       success = TRUE;

  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return success;

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
        continue;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (config), prop_spec->name, &value);

      if (! g_param_value_defaults (prop_spec, &value))
        {
          /* Some properties may fail writing, which shouldn't break serializing
           * more properties, yet final result would be a (partial) failure.
           */
          if (! gimp_config_serialize_property (config, prop_spec, writer))
            success = FALSE;
        }

      g_value_unset (&value);
    }

  g_free (property_specs);

  return success;
}

/**
 * gimp_config_serialize_property:
 * @config:     a #GimpConfig.
 * @param_spec: a #GParamSpec.
 * @writer:     a #GimpConfigWriter.
 *
 * This function serializes a single object property to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
gimp_config_serialize_property (GimpConfig       *config,
                                GParamSpec       *param_spec,
                                GimpConfigWriter *writer)
{
  GimpConfigInterface *config_iface = NULL;
  GimpConfigInterface *parent_iface = NULL;
  GValue               value        = G_VALUE_INIT;
  gboolean             success      = FALSE;

  if (! (param_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
    return FALSE;

  if (param_spec->flags & GIMP_CONFIG_PARAM_IGNORE ||
      param_spec->flags & GIMP_PARAM_DONT_SERIALIZE)
    return TRUE;

  g_value_init (&value, param_spec->value_type);
  g_object_get_property (G_OBJECT (config), param_spec->name, &value);

  if (param_spec->flags & GIMP_CONFIG_PARAM_DEFAULTS &&
      g_param_value_defaults (param_spec, &value))
    {
      g_value_unset (&value);
      return TRUE;
    }

  if (G_TYPE_IS_OBJECT (param_spec->owner_type))
    {
      GTypeClass *owner_class = g_type_class_peek (param_spec->owner_type);

      config_iface = g_type_interface_peek (owner_class, GIMP_TYPE_CONFIG);

      /*  We must call serialize_property() *only* if the *exact* class
       *  which implements it is param_spec->owner_type's class.
       *
       *  Therefore, we ask param_spec->owner_type's immediate parent class
       *  for it's GimpConfigInterface and check if we get a different
       *  pointer.
       *
       *  (if the pointers are the same, param_spec->owner_type's
       *   GimpConfigInterface is inherited from one of it's parent classes
       *   and thus not able to handle param_spec->owner_type's properties).
       */
      if (config_iface)
        {
          GTypeClass *owner_parent_class;

          owner_parent_class = g_type_class_peek_parent (owner_class);

          parent_iface = g_type_interface_peek (owner_parent_class,
                                                GIMP_TYPE_CONFIG);
        }
    }

  if (config_iface                     &&
      config_iface != parent_iface     && /* see comment above */
      config_iface->serialize_property &&
      config_iface->serialize_property (config,
                                        param_spec->param_id,
                                        (const GValue *) &value,
                                        param_spec,
                                        writer))
    {
      success = TRUE;
    }

  /*  If there is no serialize_property() method *or* if it returned
   *  FALSE, continue with the default implementation
   */

  if (! success)
    {
      if (G_VALUE_TYPE (&value) == GIMP_TYPE_PARASITE)
        {
          GimpParasite *parasite = g_value_get_boxed (&value);

          gimp_config_writer_open (writer, param_spec->name);

          if (parasite)
            {
              const gchar   *name;
              gconstpointer  data;
              guint32        data_length;
              gulong         flags;

              name = gimp_parasite_get_name (parasite);
              gimp_config_writer_string (writer, name);

              flags = gimp_parasite_get_flags (parasite);
              data = gimp_parasite_get_data (parasite, &data_length);
              gimp_config_writer_printf (writer, "%lu %u", flags, data_length);
              gimp_config_writer_data (writer, data_length, data);

              success = TRUE;
            }

          if (success)
            gimp_config_writer_close (writer);
          else
            gimp_config_writer_revert (writer);
        }
      else if (G_VALUE_TYPE (&value) == G_TYPE_BYTES)
        {
          GBytes *bytes = g_value_get_boxed (&value);

          gimp_config_writer_open (writer, param_spec->name);

          if (bytes)
            {
              gconstpointer data;
              gsize         data_length;

              data = g_bytes_get_data (bytes, &data_length);

              gimp_config_writer_printf (writer, "%" G_GSIZE_FORMAT, data_length);
              gimp_config_writer_data (writer, data_length, data);
            }
          else
            {
              gimp_config_writer_printf (writer, "%s", "NULL");
            }

          success = TRUE;
          gimp_config_writer_close (writer);
        }
      else if (GIMP_VALUE_HOLDS_COLOR (&value))
        {
          GeglColor *color      = g_value_get_object (&value);
          gboolean   free_color = FALSE;

          gimp_config_writer_open (writer, param_spec->name);

          if (color)
            {
              const gchar   *encoding;
              const Babl    *format = gegl_color_get_format (color);
              const Babl    *space;
              GBytes        *bytes;
              gconstpointer  data;
              gsize          data_length;
              int            profile_length = 0;

              gimp_config_writer_open (writer, "color");

              if (babl_format_is_palette (format))
                {
                  guint8 pixel[40];

                  /* As a special case, we don't want to serialize
                   * palette colors, because they are just too much
                   * dependent on external data and cannot be
                   * deserialized back safely. So we convert them first.
                   */
                   free_color = TRUE;
                   color = gegl_color_duplicate (color);

                   format = babl_format_with_space ("R'G'B'A u8", format);
                   gegl_color_get_pixel (color, format, pixel);
                   gegl_color_set_pixel (color, format, pixel);
                }

              encoding = babl_format_get_encoding (format);
              gimp_config_writer_string (writer, encoding);

              bytes = gegl_color_get_bytes (color, format);
              data  = g_bytes_get_data (bytes, &data_length);

              gimp_config_writer_printf (writer, "%" G_GSIZE_FORMAT, data_length);
              gimp_config_writer_data (writer, data_length, data);

              space = babl_format_get_space (format);
              if (space != babl_space ("sRGB"))
                {
                  guint8 *profile_data;

                  profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                &profile_length);
                  gimp_config_writer_printf (writer, "%u", profile_length);
                  if (profile_data)
                    gimp_config_writer_data (writer, profile_length, profile_data);
                }
              else
                {
                  gimp_config_writer_printf (writer, "%u", profile_length);
                }

              g_bytes_unref (bytes);

              gimp_config_writer_close (writer);
            }
          else
            {
              gimp_config_writer_printf (writer, "%s", "NULL");
            }

          success = TRUE;
          gimp_config_writer_close (writer);

          if (free_color)
            g_object_unref (color);
        }
      else if (GIMP_VALUE_HOLDS_UNIT (&value))
        {
          GimpUnit *unit = g_value_get_object (&value);

          gimp_config_writer_open (writer, param_spec->name);

          if (unit)
            gimp_config_writer_printf (writer, "%s", gimp_unit_get_name (unit));
          else
            gimp_config_writer_printf (writer, "%s", "NULL");

          success = TRUE;
          gimp_config_writer_close (writer);
        }
      else if (G_VALUE_HOLDS_OBJECT (&value) &&
               G_VALUE_TYPE (&value) != G_TYPE_FILE)
        {
          GimpConfigInterface *config_iface = NULL;
          GimpConfig          *prop_object;

          prop_object = g_value_get_object (&value);

          if (prop_object)
            config_iface = GIMP_CONFIG_GET_IFACE (prop_object);
          else
            success = TRUE;

          if (config_iface)
            {
              gimp_config_writer_open (writer, param_spec->name);

              /*  if the object property is not GIMP_CONFIG_PARAM_AGGREGATE,
               *  deserializing will need to know the exact type
               *  in order to create the object
               */
              if (! (param_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE))
                {
                  GType object_type = G_TYPE_FROM_INSTANCE (prop_object);

                  gimp_config_writer_string (writer, g_type_name (object_type));
                }

              success = config_iface->serialize (prop_object, writer, NULL);

              if (success)
                gimp_config_writer_close (writer);
              else
                gimp_config_writer_revert (writer);
            }
        }
      else
        {
          GString *str = g_string_new (NULL);

          success = gimp_config_serialize_value (&value, str, TRUE);

          if (success)
            {
              gimp_config_writer_open (writer, param_spec->name);
              gimp_config_writer_print (writer, str->str, str->len);
              gimp_config_writer_close (writer);
            }

          g_string_free (str, TRUE);
        }

      if (! success)
        {
          /* don't warn for empty string properties */
          if (G_VALUE_HOLDS_STRING (&value))
            {
              success = TRUE;
            }
          else
            {
              g_warning ("couldn't serialize property %s::%s of type %s",
                         g_type_name (G_TYPE_FROM_INSTANCE (config)),
                         param_spec->name,
                         g_type_name (param_spec->value_type));
            }
        }
    }

  g_value_unset (&value);

  return success;
}

/**
 * gimp_config_serialize_property_by_name:
 * @config:    a #GimpConfig.
 * @prop_name: the property's name.
 * @writer:    a #GimpConfigWriter.
 *
 * This function serializes a single object property to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.6
 **/
gboolean
gimp_config_serialize_property_by_name (GimpConfig       *config,
                                        const gchar      *prop_name,
                                        GimpConfigWriter *writer)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        prop_name);

  if (! pspec)
    return FALSE;

  return gimp_config_serialize_property (config, pspec, writer);
}

/**
 * gimp_config_serialize_value:
 * @value: a #GValue.
 * @str: a #GString.
 * @escaped: whether to escape string values.
 *
 * This utility function appends a string representation of #GValue to @str.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
gimp_config_serialize_value (const GValue *value,
                             GString      *str,
                             gboolean      escaped)
{
  if (G_VALUE_TYPE (value) == G_TYPE_STRV)
    {
      return gimp_config_serialize_strv (value, str);
    }

  if (GIMP_VALUE_HOLDS_INT32_ARRAY (value) ||
      GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value))
    {
      return gimp_config_serialize_array (value, str);
    }

  if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      gboolean boolean;

      boolean = g_value_get_boolean (value);
      g_string_append (str, boolean ? "yes" : "no");
      return TRUE;
    }

  if (G_VALUE_HOLDS_ENUM (value))
    {
      GEnumClass *enum_class = g_type_class_peek (G_VALUE_TYPE (value));
      GEnumValue *enum_value = g_enum_get_value (enum_class,
                                                 g_value_get_enum (value));

      if (enum_value && enum_value->value_nick)
        {
          g_string_append (str, enum_value->value_nick);
          return TRUE;
        }
      else
        {
          g_warning ("Couldn't get nick for enum_value of %s",
                     G_ENUM_CLASS_TYPE_NAME (enum_class));
          return FALSE;
        }
    }

  if (G_VALUE_HOLDS_STRING (value))
    {
      const gchar *cstr = g_value_get_string (value);

      if (!cstr)
        return FALSE;

      if (escaped)
        gimp_config_string_append_escaped (str, cstr);
      else
        g_string_append (str, cstr);

      return TRUE;
    }

  if (G_VALUE_HOLDS_DOUBLE (value) || G_VALUE_HOLDS_FLOAT (value))
    {
      gdouble v_double;
      gchar   buf[G_ASCII_DTOSTR_BUF_SIZE];

      if (G_VALUE_HOLDS_DOUBLE (value))
        v_double = g_value_get_double (value);
      else
        v_double = (gdouble) g_value_get_float (value);

      g_ascii_dtostr (buf, sizeof (buf), v_double);
      g_string_append (str, buf);
      return TRUE;
    }

  if (GIMP_VALUE_HOLDS_MATRIX2 (value))
    {
      GimpMatrix2 *trafo;

      trafo = g_value_get_boxed (value);

      if (trafo)
        {
          gchar buf[4][G_ASCII_DTOSTR_BUF_SIZE];
          gint  i, j, k;

          for (i = 0, k = 0; i < 2; i++)
            for (j = 0; j < 2; j++, k++)
              g_ascii_dtostr (buf[k], G_ASCII_DTOSTR_BUF_SIZE,
                              trafo->coeff[i][j]);

          g_string_append_printf (str, "(matrix %s %s %s %s)",
                                  buf[0], buf[1], buf[2], buf[3]);
        }
      else
        {
          g_string_append (str, "(matrix 1.0 1.0 1.0 1.0)");
        }

      return TRUE;
    }

  if (G_VALUE_TYPE (value) == GIMP_TYPE_VALUE_ARRAY)
    {
      GimpValueArray *array;

      array = g_value_get_boxed (value);

      if (array)
        {
          gint length = gimp_value_array_length (array);
          gint i;

          g_string_append_printf (str, "%d", length);

          for (i = 0; i < length; i++)
            {
              g_string_append (str, " ");

              if (! gimp_config_serialize_value (gimp_value_array_index (array,
                                                                         i),
                                                 str, TRUE))
                return FALSE;
            }
        }
      else
        {
          g_string_append (str, "0");
        }

      return TRUE;
    }

  if (G_VALUE_TYPE (value) == G_TYPE_FILE)
    {
      GFile *file = g_value_get_object (value);

      if (file)
        {
          gchar *path     = g_file_get_uri (file);
          gchar *unexpand = NULL;

          if (path)
            {
              unexpand = gimp_config_path_unexpand (path, TRUE, NULL);
              g_free (path);
            }

          if (unexpand)
            {
              if (escaped)
                gimp_config_string_append_escaped (str, unexpand);
              else
                g_string_append (str, unexpand);

              g_free (unexpand);
            }
          else
            {
              g_string_append (str, "NULL");
            }
        }
      else
        {
          g_string_append (str, "NULL");
        }

      return TRUE;
    }

  if (g_value_type_transformable (G_VALUE_TYPE (value), G_TYPE_STRING))
    {
      GValue  tmp_value = G_VALUE_INIT;

      g_value_init (&tmp_value, G_TYPE_STRING);
      g_value_transform (value, &tmp_value);

      g_string_append (str, g_value_get_string (&tmp_value));

      g_value_unset (&tmp_value);
      return TRUE;
    }

  return FALSE;
}


/* Private functions */

/**
 * gimp_config_serialize_strv:
 * @value: source #GValue holding a #GStrv
 * @str:   destination string
 *
 * Appends a string repr of the #GStrv value of #GValue to @str.
 * Repr is an integer literal greater than or equal to zero,
 * followed by a possibly empty sequence
 * of quoted and escaped string literals.
 *
 * Returns: %TRUE always
 *
 * Since: 3.0
 **/
static gboolean
gimp_config_serialize_strv (const GValue *value,
                            GString      *str)
{
  GStrv gstrv;

  gstrv = g_value_get_boxed (value);

  if (gstrv)
    {
      gint length = g_strv_length (gstrv);

      /* Write length */
      g_string_append_printf (str, "%d", length);

      for (gint i = 0; i < length; i++)
        {
          g_string_append (str, " "); /* separator */
          gimp_config_string_append_escaped (str, gstrv[i]);
        }
    }
  else
    {
      /* GValue has NULL value. Not quite the same as an empty GStrv.
       * But handle it quietly as an empty GStrv: write a length of zero.
       */
      g_string_append (str, "0");
    }

  return TRUE;
}

static gboolean
gimp_config_serialize_array (const GValue *value,
                             GString      *str)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value) ||
                        GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value), FALSE);

  array = g_value_get_boxed (value);

  if (array)
    {
      gint32 *values = (gint32 *) array->data;
      gint    length;

      if (GIMP_VALUE_HOLDS_INT32_ARRAY (value))
        length = array->length / sizeof (gint32);
      else
        length = array->length / sizeof (gdouble);

      /* Write length */
      g_string_append_printf (str, "%d", length);

      for (gint i = 0; i < length; i++)
        {
          gchar *num_str;

          if (GIMP_VALUE_HOLDS_INT32_ARRAY (value))
            {
              num_str = g_strdup_printf (" %d", values[i]);
            }
          else
            {
              gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

              g_ascii_dtostr (buf, sizeof (buf), ((gdouble *) values)[i]);
              num_str = g_strdup_printf (" %s", buf);
            }

          g_string_append (str, num_str);
          g_free (num_str);
        }
    }
  else
    {
      g_string_append (str, "0");
    }

  return TRUE;
}
