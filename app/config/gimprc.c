/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpRc
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-deserialize.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void     gimp_rc_config_iface_init  (gpointer  iface,
                                            gpointer  iface_data);

static void     gimp_rc_serialize                    (GObject      *object,
                                                      gint          fd);
static gboolean gimp_rc_deserialize                  (GObject      *object,
                                                      GScanner     *scanner);
static void     gimp_rc_serialize_changed_properties (GimpRc       *new,
                                                      GimpRc       *old,
                                                      gint          fd);
static gboolean gimp_values_equal                    (const GValue *a,
                                                      const GValue *b);


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
}

static void
gimp_rc_serialize (GObject *object,
                   gint     fd)
{
  gimp_config_serialize_properties (object, fd);
  gimp_config_serialize_unknown_tokens (object, fd);
}

static gboolean
gimp_rc_deserialize (GObject  *object,
                     GScanner *scanner)
{
  return gimp_config_deserialize_properties (object, scanner, TRUE);
}

GimpRc *
gimp_rc_new (void)
{
  return GIMP_RC (g_object_new (GIMP_TYPE_RC, NULL));
}

gboolean
gimp_rc_write_changes (GimpRc      *new,
                       GimpRc      *old,
                       const gchar *filename)
{
  gint fd;

  g_return_val_if_fail (GIMP_IS_RC (new), FALSE);
  g_return_val_if_fail (GIMP_IS_RC (old), FALSE);

  if (filename)
    fd = open (filename, O_WRONLY | O_CREAT, 
               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  else
    fd = 1; /* stdout */

  if (fd == -1)
    {
      g_message (_("Failed to open file '%s': %s"),
                 filename, g_strerror (errno));
      return FALSE;
    }

  gimp_rc_serialize_changed_properties (new, old, fd);
  gimp_config_serialize_unknown_tokens (G_OBJECT (new), fd);

  if (filename)
    close (fd);

  return TRUE;
}

static void
gimp_rc_serialize_changed_properties (GimpRc *new,
                                      GimpRc *old,
                                      gint    fd)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  GString       *str;

  klass = G_OBJECT_GET_CLASS (new);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return;

  str = g_string_new (NULL);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec  *prop_spec;
      GValue       new_value = { 0, };
      GValue       old_value = { 0, };

      prop_spec = property_specs[i];

      if (! (prop_spec->flags & G_PARAM_READWRITE))
        continue;

      g_value_init (&new_value, prop_spec->value_type);
      g_value_init (&old_value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (new), prop_spec->name, &new_value);
      g_object_get_property (G_OBJECT (old), prop_spec->name, &old_value);

      if (!gimp_values_equal (&new_value, &old_value))
        {
          g_string_assign (str, "(");
          g_string_append (str, prop_spec->name);
      
          if (gimp_config_serialize_value (&new_value, str))
            {
              g_string_append (str, ")\n");
              write (fd, str->str, str->len);
            }
          else if (prop_spec->value_type != G_TYPE_STRING)
            {
              g_warning ("couldn't serialize property %s::%s of type %s",
                         g_type_name (G_TYPE_FROM_INSTANCE (new)),
                         prop_spec->name, 
                         g_type_name (prop_spec->value_type));
            }
        }

      g_value_unset (&new_value);
      g_value_unset (&old_value);
    }

  g_free (property_specs);
  g_string_free (str, TRUE);
}
  
static gboolean
gimp_values_equal (const GValue *a,
                   const GValue *b)
{
  g_return_val_if_fail (G_VALUE_TYPE (a) == G_VALUE_TYPE (b), FALSE);

  if (g_value_fits_pointer (a))
    {
      if (a->data[0].v_pointer == b->data[0].v_pointer)
        return TRUE;

      if (G_VALUE_HOLDS_STRING (a))
        {
          const gchar *a_str = g_value_get_string (a);
          const gchar *b_str = g_value_get_string (b);

          if (a_str && b_str)
            return (strcmp (a_str, b_str) == 0);
          else
            return FALSE;
        }
      else
        {
          g_warning ("%s: Can not compare values of type %s.", 
                     G_STRLOC, G_VALUE_TYPE_NAME (a));
          return FALSE;
        }
    }
  else
    {
      return (a->data[0].v_uint64 == b->data[0].v_uint64); 
    }
}
