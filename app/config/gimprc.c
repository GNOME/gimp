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

#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase.h"

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-params.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-utils.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


enum {
  PROP_0,
  PROP_SYSTEM_GIMPRC,
  PROP_USER_GIMPRC
};


static void       gimp_rc_class_init        (GimpRcClass  *klass);
static void       gimp_rc_config_iface_init (gpointer      iface,
                                             gpointer      iface_data);
static void       gimp_rc_finalize          (GObject      *object);
static void       gimp_rc_set_property      (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void       gimp_rc_get_property      (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);
static gboolean   gimp_rc_serialize         (GObject      *object,
                                             gint          fd,
                                             gint          indent_level,
                                             gpointer      data);
static gboolean   gimp_rc_deserialize       (GObject      *object,
                                             GScanner     *scanner,
                                             gint          nest_level,
                                             gpointer      data);
static GObject  * gimp_rc_duplicate         (GObject      *object);
static void       gimp_rc_load              (GimpRc       *rc);


static GObjectClass *parent_class = NULL;


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
        (GClassInitFunc) gimp_rc_class_init,
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
gimp_rc_class_init (GimpRcClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_rc_finalize;
  object_class->set_property = gimp_rc_set_property;
  object_class->get_property = gimp_rc_get_property;

  g_object_class_install_property (object_class, PROP_SYSTEM_GIMPRC,
				   g_param_spec_string ("system-gimprc",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_USER_GIMPRC,
				   g_param_spec_string ("user-gimprc",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_rc_finalize (GObject *object)
{
  GimpRc *rc = (GimpRc *) object;

  if (rc->system_gimprc)
    {
      g_free (rc->system_gimprc);
      rc->system_gimprc = NULL;
    }
  if (rc->user_gimprc)
    {
      g_free (rc->user_gimprc);
      rc->user_gimprc = NULL;
    }
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_rc_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  GimpRc      *rc       = GIMP_RC (object);
  const gchar *filename = NULL;

  switch (property_id)
    {
    case PROP_SYSTEM_GIMPRC:
    case PROP_USER_GIMPRC:
      filename = g_value_get_string (value);
      g_return_if_fail (filename == NULL || g_path_is_absolute (filename));
      break;
    }

  switch (property_id)
    {
    case PROP_SYSTEM_GIMPRC:
      g_free (rc->system_gimprc);
      
      if (filename)
        rc->system_gimprc = g_strdup (filename);
      else
        rc->system_gimprc = g_build_filename (gimp_sysconf_directory (),
                                              "gimprc", NULL);
      break;
    case PROP_USER_GIMPRC:
      g_free (rc->user_gimprc);

      if (filename)
        rc->user_gimprc = g_strdup (filename);
      else
        rc->user_gimprc = gimp_personal_rc_file ("gimprc");
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_rc_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  GimpRc *rc = GIMP_RC (object);
  
  switch (property_id)
    {
    case PROP_SYSTEM_GIMPRC:
      g_value_set_string (value, rc->system_gimprc);
      break;
    case PROP_USER_GIMPRC:
      g_value_set_string (value, rc->user_gimprc);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
  if (!gimp_config_serialize_unknown_tokens (object, fd, indent_level))
    return FALSE;

  if (write (fd, "\n", 1) < 0)
    return FALSE;

  if (data && GIMP_IS_RC (data))
    return gimp_config_serialize_changed_properties (object, G_OBJECT (data),
                                                     fd, indent_level);
  else
    return gimp_config_serialize_properties (object, fd, indent_level);
}

static gboolean
gimp_rc_deserialize (GObject  *object,
                     GScanner *scanner,
                     gint      nest_level,
                     gpointer  data)
{
  return gimp_config_deserialize_properties (object,
                                             scanner, nest_level, TRUE);
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

static void
gimp_rc_load (GimpRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_RC (rc));

  g_printerr ("parsing '%s'\n", rc->system_gimprc);

  if (! gimp_config_deserialize (G_OBJECT (rc),
                                 rc->system_gimprc, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
	g_message (error->message);
      
      g_clear_error (&error);
    }

  g_printerr ("parsing '%s'\n", rc->user_gimprc);

  if (! gimp_config_deserialize (G_OBJECT (rc),
                                 rc->user_gimprc, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
	g_message (error->message);
      
      g_clear_error (&error);
    }
}

GimpRc *
gimp_rc_new (const gchar *system_gimprc,
             const gchar *user_gimprc)
{
  GimpRc *rc;

  g_return_val_if_fail (system_gimprc == NULL ||
                        g_path_is_absolute (system_gimprc), NULL);
  g_return_val_if_fail (user_gimprc == NULL || 
                        g_path_is_absolute (user_gimprc), NULL);

  rc = GIMP_RC (g_object_new (GIMP_TYPE_RC,
                              "system-gimprc", system_gimprc,
                              "user-gimprc",   user_gimprc,
                              NULL));

  gimp_rc_load (rc);

  return rc;
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

void
gimp_rc_save (GimpRc *rc)
{
  GimpRc *global;
  
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
  GError *error = NULL;
  
  g_return_if_fail (GIMP_IS_RC (rc));
 
  global = g_object_new (GIMP_TYPE_RC, NULL);

  gimp_config_deserialize (G_OBJECT (global), rc->system_gimprc, NULL, NULL);
  
  header = g_strconcat (top, rc->system_gimprc, bottom, NULL);

  g_printerr ("saving '%s'\n", rc->user_gimprc);

  if (! gimp_config_serialize (G_OBJECT (rc),
                               rc->user_gimprc, header, footer, global,
			       &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  g_free (header);
  g_object_unref (global);
}
