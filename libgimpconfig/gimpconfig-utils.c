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
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-params.h"
#include "gimpconfig-utils.h"

#include "gimp-intl.h"


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
          GValue value = { 0, };

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

static gboolean
gimp_config_diff_property (GObject    *a,
                           GObject    *b,
                           GParamSpec *prop_spec)
{
  GValue    a_value = { 0, };
  GValue    b_value = { 0, };
  gboolean  retval  = FALSE;

  g_value_init (&a_value, prop_spec->value_type);
  g_value_init (&b_value, prop_spec->value_type);

  g_object_get_property (a, prop_spec->name, &a_value);
  g_object_get_property (b, prop_spec->name, &b_value);

  if (g_param_values_cmp (prop_spec, &a_value, &b_value))
    {
      if ((prop_spec->flags & GIMP_PARAM_AGGREGATE) &&
          G_IS_PARAM_SPEC_OBJECT (prop_spec)        &&
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
gimp_config_diff_same (GimpConfig  *a,
                       GimpConfig  *b,
                       GParamFlags  flags)
{
  GParamSpec **param_specs;
  guint        n_param_specs;
  gint         i;
  GList       *list = NULL;

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (a),
                                                &n_param_specs);

  for (i = 0; i < n_param_specs; i++)
    {
      GParamSpec *prop_spec = param_specs[i];

      if (! flags || ((prop_spec->flags & flags) == flags))
        {
          if (gimp_config_diff_property (G_OBJECT (a),
                                         G_OBJECT (b), prop_spec))
            list = g_list_prepend (list, prop_spec);
        }
    }

  g_free (param_specs);

  return list;
}

static GList *
gimp_config_diff_other (GimpConfig  *a,
                        GimpConfig  *b,
                        GParamFlags  flags)
{
  GParamSpec **param_specs;
  guint        n_param_specs;
  gint         i;
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
          if (gimp_config_diff_property (G_OBJECT (a), G_OBJECT (b), b_spec))
            list = g_list_prepend (list, b_spec);
        }
    }

  g_free (param_specs);

  return list;
}


/**
 * gimp_config_diff:
 * @a: a #GimpConfig object
 * @b: another #GimpConfig object
 * @flags: a mask of GParamFlags
 *
 * Compares all properties of @a and @b that have all @flags set. If
 * @flags is 0, all properties are compared.
 *
 * If the two objects are not of the same type, only properties that
 * exist in both object classes and are of the same value_type are
 * compared.
 *
 * Return value: a GList of differing GParamSpecs.
 **/
GList *
gimp_config_diff (GimpConfig  *a,
                  GimpConfig  *b,
                  GParamFlags  flags)
{
  GList *diff;

  g_return_val_if_fail (GIMP_IS_CONFIG (a), NULL);
  g_return_val_if_fail (GIMP_IS_CONFIG (b), NULL);

  if (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b))
    diff = gimp_config_diff_same (a, b, flags);
  else
    diff = gimp_config_diff_other (a, b, flags);

  return g_list_reverse (diff);
}

/**
 * gimp_config_sync:
 * @src: a #GimpConfig object
 * @dest: another #GimpConfig object
 * @flags: a mask of GParamFlags
 *
 * Compares all read- and write-able properties from @src and @dest
 * that have all @flags set. Differing values are then copied from
 * @src to @dest. If @flags is 0, all differing read/write properties.
 *
 * Properties marked as "construct-only" are not touched.
 *
 * If the two objects are not of the same type, only
 * properties that exist in both object classes and are of the same
 * value_type are synchronized
 *
 * Return value: %TRUE if @dest was modified, %FALSE otherwise
 **/
gboolean
gimp_config_sync (GimpConfig  *src,
                  GimpConfig  *dest,
                  GParamFlags  flags)
{
  GList *diff;
  GList *list;

  g_return_val_if_fail (GIMP_IS_CONFIG (src), FALSE);
  g_return_val_if_fail (GIMP_IS_CONFIG (dest), FALSE);

  /* we use the internal versions here for a number of reasons:
   *  - it saves a g_list_reverse()
   *  - it avoids duplicated parameter checks
   *  - it makes GimpTemplateEditor work (resolution is set before size)
   */
  if (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest))
    diff = gimp_config_diff_same (src, dest, (flags | G_PARAM_READWRITE));
  else
    diff = gimp_config_diff_other (src, dest, flags);

  if (!diff)
    return FALSE;

  for (list = diff; list; list = list->next)
    {
      GParamSpec *prop_spec = list->data;

      if (! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
        {
          GValue value = { 0, };

          g_value_init (&value, prop_spec->value_type);

          g_object_get_property (G_OBJECT (src),  prop_spec->name, &value);
          g_object_set_property (G_OBJECT (dest), prop_spec->name, &value);

          g_value_unset (&value);
        }
    }

  g_list_free (diff);

  return TRUE;
}

/**
 * gimp_config_reset_properties:
 * @config: a #GimpConfig
 *
 * Resets all writable properties of @object to the default values as
 * defined in their #GParamSpec. Properties marked as "construct-only"
 * are not touched.
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
gimp_config_build_writable_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name, NULL);
}

gchar *
gimp_config_build_plug_in_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${gimp_plug_in_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}


/*
 * GimpConfig file utilities
 */

gboolean
gimp_config_file_copy (const gchar  *source,
                       const gchar  *dest,
                       GError      **error)
{
  gchar        buffer[8192];
  FILE        *sfile;
  FILE        *dfile;
  struct stat  stat_buf;
  gint         nbytes;

  sfile = fopen (source, "rb");
  if (sfile == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (source), g_strerror (errno));
      return FALSE;
    }

  dfile = fopen (dest, "wb");
  if (dfile == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (dest), g_strerror (errno));
      fclose (sfile);
      return FALSE;
    }

  while ((nbytes = fread (buffer, 1, sizeof (buffer), sfile)) > 0)
    {
      if (fwrite (buffer, 1, nbytes, dfile) < nbytes)
	{
	  g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Error while writing '%s': %s"),
                       gimp_filename_to_utf8 (dest), g_strerror (errno));
	  fclose (sfile);
	  fclose (dfile);
	  return FALSE;
	}
    }

  if (ferror (sfile))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error while reading '%s': %s"),
                   gimp_filename_to_utf8 (source), g_strerror (errno));
      fclose (sfile);
      fclose (dfile);
      return FALSE;
    }

  fclose (sfile);

  if (fclose (dfile) == EOF)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error while writing '%s': %s"),
                   gimp_filename_to_utf8 (dest), g_strerror (errno));
      return FALSE;
    }

  if (stat (source, &stat_buf) == 0)
    {
      chmod (dest, stat_buf.st_mode);
    }

  return TRUE;
}

gboolean
gimp_config_file_backup_on_error (const gchar  *filename,
                                  const gchar  *name,
                                  GError      **error)
{
  gchar    *backup;
  gboolean  success;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  backup = g_strconcat (filename, "~", NULL);

  success = gimp_config_file_copy (filename, backup, error);

  if (success)
    g_message (_("There was an error parsing your '%s' file. "
                 "Default values will be used. A backup of your "
                 "configuration has been created at '%s'."),
               name, gimp_filename_to_utf8 (backup));

  g_free (backup);

  return success;
}
