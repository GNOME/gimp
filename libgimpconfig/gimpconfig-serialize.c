/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Object properties serialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-types.h"
#include "gimpconfig-utils.h"


static void  serialize_unknown_token (const gchar  *key,
				      const gchar  *value,
				      gpointer      data);


/**
 * gimp_config_serialize_properties:
 * @object: a #GObject. 
 * @fd: a file descriptor to write to.
 * 
 * This function writes all object properties to the file descriptor @fd.
 **/
gboolean
gimp_config_serialize_properties (GObject  *object,
                                  gint      fd,
                                  gint      indent_level)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  
  klass = G_OBJECT_GET_CLASS (object);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return TRUE;

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (! (prop_spec->flags & GIMP_PARAM_SERIALIZE))
        continue;

      if (! gimp_config_serialize_property (object, prop_spec,
                                            fd, indent_level))
        return FALSE;
    }

  g_free (property_specs);

  return TRUE;
}

/**
 * gimp_config_serialize_changed_properties:
 * @object: a #GObject. 
 * @fd: a file descriptor to write to.
 * 
 * This function writes all object properties that have been changed from
 * their default values to the file descriptor @fd.
 **/
gboolean
gimp_config_serialize_changed_properties (GObject *object,
                                          gint     fd,
                                          gint     indent_level)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  GValue         value = { 0, };

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  klass = G_OBJECT_GET_CLASS (object);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return TRUE;

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (! (prop_spec->flags & GIMP_PARAM_SERIALIZE))
        continue;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (object, prop_spec->name, &value);

      if (! g_param_value_defaults (prop_spec, &value))
        {
          if (! gimp_config_serialize_property (object, prop_spec,
                                                fd, indent_level))
            return FALSE;
        }

      g_value_unset (&value);
    }

  g_free (property_specs);

  return TRUE;
}

/**
 * gimp_config_serialize_properties_diff:
 * @object: a #GObject. 
 * @compare: a #GObject of the same type as @object. 
 * @fd: a file descriptor to write to.
 * 
 * This function compares @object and @compare and writes all
 * properties of @object that have different values than @compare to
 * the file descriptor @fd.
 **/
gboolean
gimp_config_serialize_properties_diff (GObject *object,
                                       GObject *compare,
                                       gint     fd,
                                       gint     indent_level)
{
  GObjectClass *klass;
  GList        *diff;
  GList        *list;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (compare), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (object) == 
                        G_TYPE_FROM_INSTANCE (compare), FALSE);

  klass = G_OBJECT_GET_CLASS (object);

  diff = gimp_config_diff (object, compare, GIMP_PARAM_SERIALIZE);

  if (! diff)
    return TRUE;

  for (list = diff; list; list = g_list_next (list))
    {
      GParamSpec *prop_spec = (GParamSpec *) list->data;

      if (! (prop_spec->flags & GIMP_PARAM_SERIALIZE))
        continue;

      if (! gimp_config_serialize_property (object, prop_spec,
                                            fd, indent_level))
        return FALSE;
    }

  g_list_free (diff);

  return TRUE;
}


gboolean
gimp_config_serialize_property (GObject    *object,
                                GParamSpec *param_spec,
                                gint        fd,
                                gint        indent_level)
{
  GTypeClass          *owner_class;
  GimpConfigInterface *config_iface;
  GimpConfigInterface *parent_iface = NULL;
  GString             *str;
  GValue               value   = { 0, };
  gboolean             success = FALSE;

  if (! (param_spec->flags & GIMP_PARAM_SERIALIZE))
    return FALSE;

  str = g_string_new (NULL);
  gimp_config_string_indent (str, indent_level);
  g_string_append_printf (str, "(%s ", param_spec->name);

  g_value_init (&value, param_spec->value_type);
  g_object_get_property (object, param_spec->name, &value);

  owner_class = g_type_class_peek (param_spec->owner_type);

  config_iface = g_type_interface_peek (owner_class,
					GIMP_TYPE_CONFIG_INTERFACE);

  /*  We must call deserialize_property() *only* if the *exact* class
   *  which implements it is param_spec->owner_type's class.
   *
   *  Therefore, we ask param_spec->owner_type's immediate parent class
   *  for it's GimpConfigInterface and check if we get a different pointer.
   *
   *  (if the pointers are the same, param_spec->owner_type's
   *   GimpConfigInterface is inherited from one of it's parent classes
   *   and thus not able to handle param_spec->owner_type's properties).
   */
  if (config_iface)
    {
      GTypeClass *owner_parent_class;

      owner_parent_class = g_type_class_peek_parent (owner_class),

      parent_iface = g_type_interface_peek (owner_parent_class,
                                            GIMP_TYPE_CONFIG_INTERFACE);
    }

  if (config_iface                     &&
      config_iface != parent_iface     && /* see comment above */
      config_iface->serialize_property &&
      config_iface->serialize_property (object,
                                        param_spec->param_id,
                                        (const GValue *) &value,
                                        param_spec,
                                        str))
    {
      success = TRUE;
    }

  /*  If there is no serialize_property() method *or* if it returned
   *  FALSE, continue with the default implementation
   */

  if (! success)
    {
      if (G_VALUE_HOLDS_OBJECT (&value))
        {
          GimpConfigInterface *gimp_config_iface = NULL;
          GObject             *prop_object;

          prop_object = g_value_get_object (&value);

          if (prop_object)
            gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (prop_object);
          else
            success = TRUE;

          if (gimp_config_iface)
            {
              g_string_append_c (str, '\n');
              write (fd, str->str, str->len);
              g_string_truncate (str, 0);

              success = gimp_config_iface->serialize (prop_object,
                                                      fd, indent_level + 1,
                                                      NULL);

              success = TRUE;
            }
        }
      else
        {
          success = gimp_config_serialize_value (&value, str, TRUE);
        }
    }

  if (success)
    {
      g_string_append (str, ")\n");
      
      if (write (fd, str->str, str->len) == -1)
        success = FALSE;
    }

  if (! success)
    {
      /* don't warn for empty string properties */
      if (G_VALUE_HOLDS_STRING (&value))
        success = TRUE;
      else
        g_warning ("couldn't serialize property %s::%s of type %s",
                   g_type_name (G_TYPE_FROM_INSTANCE (object)),
                   param_spec->name, 
                   g_type_name (param_spec->value_type));
    }

  g_value_unset (&value);

  g_string_free (str, TRUE);

  return success;
}

/**
 * gimp_config_serialize_value:
 * @value: a #GValue.
 * @str: a #Gstring.
 * @escaped: whether to escape string values.
 *
 * This utility function appends a string representation of #GValue to @str.
 * 
 * Return value: %TRUE if serialization succeeded, %FALSE otherwise.
 **/
gboolean
gimp_config_serialize_value (const GValue *value,
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
      GEnumClass *enum_class;
      GEnumValue *enum_value;
      
      enum_class = g_type_class_peek (G_VALUE_TYPE (value));
      enum_value = g_enum_get_value (G_ENUM_CLASS (enum_class),
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
      gchar       *esc;
      const gchar *cstr = g_value_get_string (value);

      if (!cstr)
        return FALSE;

      if (escaped)
        {
          esc = g_strescape (cstr, NULL);
          g_string_append_printf (str, "\"%s\"", esc);
          g_free (esc);
        }
      else
        {
          g_string_append (str, cstr);
        }
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

      g_ascii_formatd (buf, sizeof (buf), "%f", v_double);
      g_string_append (str, buf);
      return TRUE;
    }

  if (GIMP_VALUE_HOLDS_COLOR (value))
    {
      GimpRGB *color;
      gchar    buf[4][G_ASCII_DTOSTR_BUF_SIZE];

      color = g_value_get_boxed (value);

      g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", color->r);
      g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", color->g);
      g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", color->b);
      g_ascii_formatd (buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f", color->a);

      g_string_append_printf (str, "(color-rgba %s %s %s %s)",
                              buf[0], buf[1], buf[2], buf[3]);
      return TRUE;
    }

  if (G_VALUE_TYPE (value) == G_TYPE_VALUE_ARRAY)
    {
      GValueArray *array;

      array = g_value_get_boxed (value);

      if (array)
        {
          gint i;

          g_string_append_printf (str, "%d", array->n_values);

          for (i = 0; i < array->n_values; i++)
            {
              g_string_append (str, " ");

              if (! gimp_config_serialize_value (g_value_array_get_nth (array,
									i),
                                                 str, TRUE))
                return FALSE;
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
      GValue  tmp_value = { 0, };
      
      g_value_init (&tmp_value, G_TYPE_STRING);
      g_value_transform (value, &tmp_value);

      g_string_append (str, g_value_get_string (&tmp_value));

      g_value_unset (&tmp_value);
      return TRUE;
    }
  
  return FALSE;
}


/**
 * gimp_config_serialize_unknown_tokens:
 * @object: a #GObject.
 * @fd: a file descriptor to write to.
 * 
 * Writes all unknown tokens attached to #object to the file descriptor @fd.
 * See gimp_config_add_unknown_token().
 **/
gboolean
gimp_config_serialize_unknown_tokens (GObject *object,
                                      gint     fd,
                                      gint     indent_level)
{
  GString *str;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  str = g_string_new (NULL);

  gimp_config_foreach_unknown_token (object, serialize_unknown_token, str);

  return (write (fd, str->str, str->len) != -1);
}


#define LINE_LENGTH 75

/**
 * gimp_config_serialize_comment:
 * @str: a #GString.
 * @comment: the comment to serialize (ASCII only)
 * 
 * Appends the @comment to @str and inserts linebreaks and hash-marks to
 * format it as a comment. Note that this function does not handle non-ASCII
 * characters.
 **/
void
gimp_config_serialize_comment (GString     *str,
                               const gchar *comment)
{
  const gchar *s;
  gint         i, len, space;

  len = strlen (comment);

  while (len > 0)
    {
      for (s = comment, i = 0, space = 0;
           *s != '\n' && (i <= LINE_LENGTH || space == 0) && i < len;
           s++, i++)
        {
          if (g_ascii_isspace (*s))
            space = i;
        }

      if (i > LINE_LENGTH && space && *s != '\n')
        i = space;

      g_string_append_len (str, "# ", 2);
      g_string_append_len (str, comment, i);
      g_string_append_c (str, '\n');

      i++;

      comment += i;
      len     -= i;
    }
}

static void
serialize_unknown_token (const gchar *key,
                         const gchar *value,
                         gpointer     data)
{
  gchar *escaped = g_strescape (value, NULL);

  g_string_append_printf ((GString *) data, "(%s \"%s\")\n", key, escaped);

  g_free (escaped);
}
