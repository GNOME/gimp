/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpRc, the object for GIMPs user configuration file gimprc.
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase.h"

#include "gimpconfig.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-params.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-utils.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void       gimp_rc_config_iface_init (gpointer  iface,
                                             gpointer  iface_data);
static gboolean   gimp_rc_serialize         (GObject  *object,
                                             gint      fd,
                                             gint      indent_level,
                                             gpointer  data);
static gboolean   gimp_rc_deserialize       (GObject  *object,
                                             GScanner *scanner,
                                             gint      nest_level,
                                             gpointer  data);
static GObject  * gimp_rc_duplicate         (GObject  *object);


GType 
gimp_rc_get_type (void)
{
  static GType rc_type = 0;

  if (! rc_type)
    {
      static const GTypeInfo rc_info =
      {
        sizeof (GimpRcClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	NULL,           /* class_init     */
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpRc),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo rc_iface_info = 
      { 
        gimp_rc_config_iface_init,
        NULL,           /* iface_finalize */ 
        NULL            /* iface_data     */
      };

      rc_type = g_type_register_static (GIMP_TYPE_GUI_CONFIG, 
                                        "GimpRc", 
                                        &rc_info, 0);

      g_type_add_interface_static (rc_type,
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &rc_iface_info);
    }

  return rc_type;
}

static void
gimp_rc_config_iface_init (gpointer  iface,
                           gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  config_iface->serialize   = gimp_rc_serialize;
  config_iface->deserialize = gimp_rc_deserialize;
  config_iface->duplicate   = gimp_rc_duplicate;
}

static gboolean
gimp_rc_serialize (GObject *object,
                   gint     fd,
                   gint     indent_level,
                   gpointer data)
{
  gboolean success;

  if (data && GIMP_IS_RC (data))
    success = gimp_config_serialize_changed_properties (object,
                                                        G_OBJECT (data),
                                                        fd,
                                                        indent_level);
  else
    success = gimp_config_serialize_properties (object, fd, indent_level);

  if (success)
    success = (write (fd, "\n", 1) != -1);
      
  if (success)
    success = gimp_config_serialize_unknown_tokens (object, fd, indent_level);

  return success;
}

static gboolean
gimp_rc_deserialize (GObject  *object,
                     GScanner *scanner,
                     gint      nest_level,
                     gpointer  data)
{
  return gimp_config_deserialize_properties (object, scanner, nest_level, TRUE);
}

static void
gimp_rc_duplicate_unknown_token (const gchar *key,
                                 const gchar *value,
                                 gpointer     user_data)
{
  gimp_config_add_unknown_token (G_OBJECT (user_data), key, value);
}

static GObject *
gimp_rc_duplicate (GObject *object)
{
  GObject *dup = g_object_new (GIMP_TYPE_RC, NULL);

  gimp_config_copy_properties (object, dup);

  gimp_config_foreach_unknown_token (object,
                                     gimp_rc_duplicate_unknown_token,
                                     dup);

  return dup;
}

/**
 * gimp_rc_new:
 * 
 * Creates a new #GimpRc object with default configuration values.
 * 
 * Return value: the newly generated #GimpRc object.
 **/
GimpRc *
gimp_rc_new (void)
{
  return GIMP_RC (g_object_new (GIMP_TYPE_RC, NULL));
}

/**
 * gimp_rc_query:
 * @rc: a #GimpRc object.
 * @key: a string used as a key for the lookup.
 * 
 * This function looks up @key in the object properties of @rc. If
 * there's a matching property, a string representation of its value
 * is returned. If no property is found, the list of unknown tokens
 * attached to the @rc object is searched.
 * 
 * Return value: a newly allocated string representing the value or %NULL
 *               if the key couldn't be found.
 **/
gchar *
gimp_rc_query (GimpRc      *rc,
               const gchar *key)
{
  GObjectClass  *klass;
  GObject       *rc_object;
  GParamSpec   **property_specs;
  GParamSpec    *prop_spec;
  guint          i, n_property_specs;
  gchar         *retval = NULL;

  g_return_val_if_fail (GIMP_IS_RC (rc), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  rc_object = G_OBJECT (rc);
  klass = G_OBJECT_GET_CLASS (rc);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return NULL;

  for (i = 0, prop_spec = NULL; i < n_property_specs && !prop_spec; i++)
    {
      prop_spec = property_specs[i];

      if (! (prop_spec->flags & GIMP_PARAM_SERIALIZE) ||
          strcmp (prop_spec->name, key))
        {
          prop_spec = NULL;
        }
    }

  if (prop_spec)
    {
      GString *str   = g_string_new (NULL);
      GValue   value = { 0, };

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (rc_object, prop_spec->name, &value);

      if (gimp_config_serialize_value (&value, str, FALSE))
        retval = g_string_free (str, FALSE);
      else
        g_string_free (str, TRUE);

      g_value_unset (&value);
    }
  else
    {
      retval = g_strdup (gimp_config_lookup_unknown_token (rc_object, key));
    }

  g_free (property_specs);

  return retval;
}

/**
 * gimp_rc_save:
 * @user_rc: the current #GimpRc.
 * @global_rc: the global #GimpRC.
 *
 * Saves the users gimprc file. If you pass a global GimpRC, only the
 * differences between the global and the users configuration is saved.
 **/
void
gimp_rc_save (GimpRc *user_rc,
              GimpRc *global_rc)
{
  const gchar *top = 
    "# GIMP gimprc\n"
    "#\n"
    "# This is your personal gimprc file.  Any variable defined in this file\n"
    "# takes precedence over the value defined in the system-wide gimprc:\n"
    "#   ";
  const gchar *bottom =
    "\n"
    "# Most values can be set within The GIMP by changing some options in\n"
    "# the Preferences dialog.\n";
  const gchar *footer =
    "# end of gimprc\n";

  gchar  *header;
  gchar  *filename;
  gchar  *system_filename;
  GError *error = NULL;
  
  g_return_if_fail (GIMP_IS_RC (user_rc));
  g_return_if_fail (global_rc == NULL || GIMP_IS_RC (global_rc));
  
  system_filename = g_build_filename (gimp_sysconf_directory (),
                                      "gimprc", NULL);
  header = g_strconcat (top, system_filename, bottom, NULL);
  g_free (system_filename);

  filename = gimp_personal_rc_file ("gimprc");

  if (! gimp_config_serialize (G_OBJECT (user_rc),
                               filename, header, footer, global_rc, &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  g_free (filename);
  g_free (header);
}
