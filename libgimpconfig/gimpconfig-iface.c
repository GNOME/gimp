/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
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

#include <fcntl.h>
#include <unistd.h>

#include <glib-object.h>

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-deserialize.h"


static void  gimp_config_iface_init (GimpConfigInterface  *gimp_config_iface);

static void      gimp_config_iface_serialize    (GObject  *object,
                                                 FILE     *file);
static gboolean  gimp_config_iface_deserialize  (GObject  *object,
                                                 GScanner *scanner);


GType
gimp_config_interface_get_type (void)
{
  static GType config_iface_type = 0;

  if (!config_iface_type)
    {
      static const GTypeInfo config_iface_info =
      {
        sizeof (GimpConfigInterface),
	(GBaseInitFunc)     gimp_config_iface_init,
	(GBaseFinalizeFunc) NULL,
      };

      config_iface_type = g_type_register_static (G_TYPE_INTERFACE, 
                                                  "GimpConfigInterface", 
                                                  &config_iface_info, 
                                                  0);

      g_type_interface_add_prerequisite (config_iface_type, 
                                         G_TYPE_OBJECT);
    }
  
  return config_iface_type;
}

static void
gimp_config_iface_init (GimpConfigInterface *gimp_config_iface)
{
  gimp_config_iface->serialize   = gimp_config_iface_serialize;
  gimp_config_iface->deserialize = gimp_config_iface_deserialize;
}

static void
gimp_config_iface_serialize (GObject *object,
                             FILE    *file)
{
  gimp_config_serialize_properties (object, file);
}

static gboolean
gimp_config_iface_deserialize (GObject  *object,
                               GScanner *scanner)
{
  return gimp_config_deserialize_properties (object, scanner);
}

gboolean
gimp_config_serialize (GObject     *object,
                       const gchar *filename)
{
  GimpConfigInterface *gimp_config_iface;
  FILE                *file;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (object);

  g_return_val_if_fail (gimp_config_iface != NULL, FALSE);

  file = fopen (filename, "w");

  if (!file)
    return FALSE;

  gimp_config_iface->serialize (object, file);

  fclose (file);

  return TRUE;
}

gboolean
gimp_config_deserialize (GObject     *object,
                         const gchar *filename)
{
  GimpConfigInterface *gimp_config_iface;
  gint                 fd;
  GScanner            *scanner;
  gboolean             success;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  
  gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (object);

  g_return_val_if_fail (gimp_config_iface != NULL, FALSE);

  fd = open (filename, O_RDONLY);

  if (fd == -1)
    return FALSE;

  scanner = g_scanner_new (NULL);

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z "-" G_CSET_A_2_Z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z "-" G_CSET_A_2_Z );

  g_scanner_input_file (scanner, fd);
  scanner->input_name = filename;

  success = gimp_config_iface->deserialize (object, scanner);

  g_scanner_destroy (scanner);
  close (fd);

  return success;
}
