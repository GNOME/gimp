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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-deserialize.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void      gimp_rc_config_iface_init (gpointer  iface,
                                            gpointer  iface_data);
static void      gimp_rc_serialize         (GObject  *object,
                                            gint      fd);
static gboolean  gimp_rc_deserialize       (GObject  *object,
                                            GScanner *scanner);
static void      gimp_rc_write_header      (gint      fd);


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
 * gimp_rc_write_changes:
 * @new_rc: a #GimpRc object.
 * @old_rc: another #GimpRc object.
 * @filename: the name of the rc file to generate. If it is %NULL, stdout 
 * will be used.
 * 
 * Writes all configuration values of @new_rc that differ from the values
 * set in @old_rc to the file specified by @filename. If the file already
 * exists, it is overwritten.
 * 
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
gimp_rc_write_changes (GimpRc      *new_rc,
                       GimpRc      *old_rc,
                       const gchar *filename)
{
  gint fd;

  g_return_val_if_fail (GIMP_IS_RC (new_rc), FALSE);
  g_return_val_if_fail (GIMP_IS_RC (old_rc), FALSE);

  if (filename)
    fd = open (filename, O_WRONLY | O_CREAT, 
#ifndef G_OS_WIN32
               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
               _S_IREAD | _S_IWRITE);
#endif
  else
    fd = 1; /* stdout */

  if (fd == -1)
    {
      g_message (_("Failed to open file '%s': %s"),
                 filename, g_strerror (errno));
      return FALSE;
    }

  gimp_rc_write_header (fd);
  gimp_config_serialize_changed_properties (G_OBJECT (new_rc), 
                                            G_OBJECT (old_rc), fd);
  gimp_config_serialize_unknown_tokens (G_OBJECT (new_rc), fd);

  if (filename)
    close (fd);

  return TRUE;
}

static void
gimp_rc_write_header (gint fd)
{
  gchar *filename;

  const gchar *top = 
    "# This is your personal gimprc file.  Any variable defined in this file\n"
    "# takes precedence over the value defined in the system-wide gimprc:\n"
    "#   ";
  const gchar *bottom =
    "\n"
    "# Most values can be set within The GIMP by changing some options in\n"
    "# the Preferences dialog.\n\n";

  filename = g_build_filename (gimp_sysconf_directory (), "gimprc", NULL);

  write (fd, top, strlen (top));
  write (fd, filename, strlen (filename));
  write (fd, bottom, strlen (bottom));

  g_free (filename);
}
