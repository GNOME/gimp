/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Object properties serialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"

#include "ligmaconfigtypes.h"

#include "ligmaconfigwriter.h"
#include "ligmaconfig-iface.h"
#include "ligmaconfig-params.h"
#include "ligmaconfig-path.h"
#include "ligmaconfig-serialize.h"
#include "ligmaconfig-utils.h"


/**
 * SECTION: ligmaconfig-serialize
 * @title: LigmaConfig-serialize
 * @short_description: Serializing for libligmaconfig.
 *
 * Serializing interface for libligmaconfig.
 **/


static gboolean  ligma_config_serialize_rgb (const GValue *value,
                                            GString      *str,
                                            gboolean      has_alpha);


/**
 * ligma_config_serialize_properties:
 * @config: a #LigmaConfig.
 * @writer: a #LigmaConfigWriter.
 *
 * This function writes all object properties to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
ligma_config_serialize_properties (LigmaConfig       *config,
                                  LigmaConfigWriter *writer)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;

  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return TRUE;

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (! (prop_spec->flags & LIGMA_CONFIG_PARAM_SERIALIZE))
        continue;

      if (! ligma_config_serialize_property (config, prop_spec, writer))
        return FALSE;
    }

  g_free (property_specs);

  return TRUE;
}

/**
 * ligma_config_serialize_changed_properties:
 * @config: a #LigmaConfig.
 * @writer: a #LigmaConfigWriter.
 *
 * This function writes all object properties that have been changed from
 * their default values to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
ligma_config_serialize_changed_properties (LigmaConfig       *config,
                                          LigmaConfigWriter *writer)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  GValue         value = G_VALUE_INIT;

  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return TRUE;

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (! (prop_spec->flags & LIGMA_CONFIG_PARAM_SERIALIZE))
        continue;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (config), prop_spec->name, &value);

      if (! g_param_value_defaults (prop_spec, &value))
        {
          if (! ligma_config_serialize_property (config, prop_spec, writer))
            return FALSE;
        }

      g_value_unset (&value);
    }

  g_free (property_specs);

  return TRUE;
}

/**
 * ligma_config_serialize_property:
 * @config:     a #LigmaConfig.
 * @param_spec: a #GParamSpec.
 * @writer:     a #LigmaConfigWriter.
 *
 * This function serializes a single object property to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
ligma_config_serialize_property (LigmaConfig       *config,
                                GParamSpec       *param_spec,
                                LigmaConfigWriter *writer)
{
  LigmaConfigInterface *config_iface = NULL;
  LigmaConfigInterface *parent_iface = NULL;
  GValue               value        = G_VALUE_INIT;
  gboolean             success      = FALSE;

  if (! (param_spec->flags & LIGMA_CONFIG_PARAM_SERIALIZE))
    return FALSE;

  if (param_spec->flags & LIGMA_CONFIG_PARAM_IGNORE)
    return TRUE;

  g_value_init (&value, param_spec->value_type);
  g_object_get_property (G_OBJECT (config), param_spec->name, &value);

  if (param_spec->flags & LIGMA_CONFIG_PARAM_DEFAULTS &&
      g_param_value_defaults (param_spec, &value))
    {
      g_value_unset (&value);
      return TRUE;
    }

  if (G_TYPE_IS_OBJECT (param_spec->owner_type))
    {
      GTypeClass *owner_class = g_type_class_peek (param_spec->owner_type);

      config_iface = g_type_interface_peek (owner_class, LIGMA_TYPE_CONFIG);

      /*  We must call serialize_property() *only* if the *exact* class
       *  which implements it is param_spec->owner_type's class.
       *
       *  Therefore, we ask param_spec->owner_type's immediate parent class
       *  for it's LigmaConfigInterface and check if we get a different
       *  pointer.
       *
       *  (if the pointers are the same, param_spec->owner_type's
       *   LigmaConfigInterface is inherited from one of it's parent classes
       *   and thus not able to handle param_spec->owner_type's properties).
       */
      if (config_iface)
        {
          GTypeClass *owner_parent_class;

          owner_parent_class = g_type_class_peek_parent (owner_class);

          parent_iface = g_type_interface_peek (owner_parent_class,
                                                LIGMA_TYPE_CONFIG);
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
      if (G_VALUE_TYPE (&value) == LIGMA_TYPE_PARASITE)
        {
          LigmaParasite *parasite = g_value_get_boxed (&value);

          ligma_config_writer_open (writer, param_spec->name);

          if (parasite)
            {
              const gchar   *name;
              gconstpointer  data;
              guint32        data_length;
              gulong         flags;

              name = ligma_parasite_get_name (parasite);
              ligma_config_writer_string (writer, name);

              flags = ligma_parasite_get_flags (parasite);
              data = ligma_parasite_get_data (parasite, &data_length);
              ligma_config_writer_printf (writer, "%lu %u", flags, data_length);
              ligma_config_writer_data (writer, data_length, data);

              success = TRUE;
            }

          if (success)
            ligma_config_writer_close (writer);
          else
            ligma_config_writer_revert (writer);
        }
      else if (G_VALUE_HOLDS_OBJECT (&value) &&
               G_VALUE_TYPE (&value) != G_TYPE_FILE)
        {
          LigmaConfigInterface *config_iface = NULL;
          LigmaConfig          *prop_object;

          prop_object = g_value_get_object (&value);

          if (prop_object)
            config_iface = LIGMA_CONFIG_GET_IFACE (prop_object);
          else
            success = TRUE;

          if (config_iface)
            {
              ligma_config_writer_open (writer, param_spec->name);

              /*  if the object property is not LIGMA_CONFIG_PARAM_AGGREGATE,
               *  deserializing will need to know the exact type
               *  in order to create the object
               */
              if (! (param_spec->flags & LIGMA_CONFIG_PARAM_AGGREGATE))
                {
                  GType object_type = G_TYPE_FROM_INSTANCE (prop_object);

                  ligma_config_writer_string (writer, g_type_name (object_type));
                }

              success = config_iface->serialize (prop_object, writer, NULL);

              if (success)
                ligma_config_writer_close (writer);
              else
                ligma_config_writer_revert (writer);
            }
        }
      else
        {
          GString *str = g_string_new (NULL);

          if (LIGMA_VALUE_HOLDS_RGB (&value))
            {
              gboolean has_alpha = ligma_param_spec_rgb_has_alpha (param_spec);

              success = ligma_config_serialize_rgb (&value, str, has_alpha);
            }
          else
            {
              success = ligma_config_serialize_value (&value, str, TRUE);
            }

          if (success)
            {
              ligma_config_writer_open (writer, param_spec->name);
              ligma_config_writer_print (writer, str->str, str->len);
              ligma_config_writer_close (writer);
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
 * ligma_config_serialize_property_by_name:
 * @config:    a #LigmaConfig.
 * @prop_name: the property's name.
 * @writer:    a #LigmaConfigWriter.
 *
 * This function serializes a single object property to the @writer.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise
 *
 * Since: 2.6
 **/
gboolean
ligma_config_serialize_property_by_name (LigmaConfig       *config,
                                        const gchar      *prop_name,
                                        LigmaConfigWriter *writer)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        prop_name);

  if (! pspec)
    return FALSE;

  return ligma_config_serialize_property (config, pspec, writer);
}

/**
 * ligma_config_serialize_value:
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
ligma_config_serialize_value (const GValue *value,
                             GString      *str,
                             gboolean      escaped)
{
  if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      gboolean bool;

      bool = g_value_get_boolean (value);
      g_string_append (str, bool ? "yes" : "no");
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
        ligma_config_string_append_escaped (str, cstr);
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

  if (LIGMA_VALUE_HOLDS_RGB (value))
    {
      return ligma_config_serialize_rgb (value, str, TRUE);
    }

  if (LIGMA_VALUE_HOLDS_MATRIX2 (value))
    {
      LigmaMatrix2 *trafo;

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

  if (G_VALUE_TYPE (value) == LIGMA_TYPE_VALUE_ARRAY)
    {
      LigmaValueArray *array;

      array = g_value_get_boxed (value);

      if (array)
        {
          gint length = ligma_value_array_length (array);
          gint i;

          g_string_append_printf (str, "%d", length);

          for (i = 0; i < length; i++)
            {
              g_string_append (str, " ");

              if (! ligma_config_serialize_value (ligma_value_array_index (array,
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
          gchar *path     = g_file_get_path (file);
          gchar *unexpand = NULL;

          if (path)
            {
              unexpand = ligma_config_path_unexpand (path, TRUE, NULL);
              g_free (path);
            }

          if (unexpand)
            {
              if (escaped)
                ligma_config_string_append_escaped (str, unexpand);
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

static gboolean
ligma_config_serialize_rgb (const GValue *value,
                           GString      *str,
                           gboolean      has_alpha)
{
  LigmaRGB *rgb;

  rgb = g_value_get_boxed (value);

  if (rgb)
    {
      gchar buf[4][G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_dtostr (buf[0], G_ASCII_DTOSTR_BUF_SIZE, rgb->r);
      g_ascii_dtostr (buf[1], G_ASCII_DTOSTR_BUF_SIZE, rgb->g);
      g_ascii_dtostr (buf[2], G_ASCII_DTOSTR_BUF_SIZE, rgb->b);

      if (has_alpha)
        {
          g_ascii_dtostr (buf[3], G_ASCII_DTOSTR_BUF_SIZE, rgb->a);

          g_string_append_printf (str, "(color-rgba %s %s %s %s)",
                                  buf[0], buf[1], buf[2], buf[3]);
        }
      else
        {
          g_string_append_printf (str, "(color-rgb %s %s %s)",
                                  buf[0], buf[1], buf[2]);
        }

      return TRUE;
    }

  return FALSE;
}
