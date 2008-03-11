/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gio/gio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "uri-backend.h"

#include "libgimp/stdplugins-intl.h"


/*  local function prototypes  */

static gchar    * get_protocols (void);
static gboolean   copy_uri      (const gchar  *src_uri,
                                 const gchar  *dest_uri,
                                 const gchar  *format,
                                 GError      **error);


/*  private variables  */

static gchar *supported_protocols = NULL;


/*  public functions  */

gboolean
uri_backend_init (const gchar  *plugin_name,
                  gboolean      run,
                  GimpRunMode   run_mode,
                  GError      **error)
{
  return TRUE;
}

void
uri_backend_shutdown (void)
{
}

const gchar *
uri_backend_get_load_help (void)
{
  return "Loads a file using the GIO Virtual File System";
}

const gchar *
uri_backend_get_save_help (void)
{
  return "Saves a file using the GIO Virtual File System";
}

const gchar *
uri_backend_get_load_protocols (void)
{
  if (! supported_protocols)
    supported_protocols = get_protocols ();

  return supported_protocols;
}

const gchar *
uri_backend_get_save_protocols (void)
{
  if (! supported_protocols)
    supported_protocols = get_protocols ();

  return supported_protocols;
}

gboolean
uri_backend_load_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar    *dest_uri;
  gboolean  success;

  dest_uri = g_filename_to_uri (tmpname, NULL, NULL);
  success = copy_uri (uri, dest_uri,
                      _("Downloading image (%s of %s)..."),
                      error);
  g_free (dest_uri);

  if (*error)
    {
      g_printerr ("uri_backend_load_image: %s\n", (*error)->message);
    }

  return success;
}

gboolean
uri_backend_save_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar    *src_uri;
  gboolean  success;

  src_uri = g_filename_to_uri (tmpname, NULL, NULL);
  success = copy_uri (src_uri, uri,
                      _("Uploading image (%s of %s)..."),
                      error);
  g_free (src_uri);

  if (*error)
    {
      g_printerr ("uri_backend_save_image: %s\n", (*error)->message);
    }

  return success;
}


/*  private functions  */

static gchar *
get_protocols (void)
{
  static const gchar *protocols[] =
  {
    "http:",
    "https:",
    "ftp:",
    "sftp:",
    "ssh:",
    "smb:",
    "dav:",
    "davs:"
  };

  GString *string = g_string_new (NULL);
  gint     i;

  for (i = 0; i < G_N_ELEMENTS (protocols); i++)
    {
      if (string->len > 0)
        g_string_append_c (string, ',');

      g_string_append (string, protocols[i]);
    }

  return g_string_free (string, FALSE);
}

static void
uri_progress_callback (goffset  current_num_bytes,
                       goffset  total_num_bytes,
                       gpointer user_data)
{
  gchar *done  = gimp_memsize_to_string (current_num_bytes);
  gchar *total = gimp_memsize_to_string (total_num_bytes);

  gimp_progress_set_text_printf ((const gchar *) user_data, done, total);

  if (total_num_bytes)
    gimp_progress_update (current_num_bytes / total_num_bytes);

  g_free (total);
  g_free (done);
}

static gboolean
copy_uri (const gchar  *src_uri,
          const gchar  *dest_uri,
          const gchar  *format,
          GError      **error)
{
  GVfs     *vfs;
  GFile    *src_file;
  GFile    *dest_file;
  gboolean  success;

  vfs = g_vfs_get_default ();

  src_file  = g_vfs_get_file_for_uri (vfs, src_uri);
  dest_file = g_vfs_get_file_for_uri (vfs, dest_uri);

  gimp_progress_init (_("Connecting to server"));

  success = g_file_copy (src_file, dest_file, G_FILE_COPY_OVERWRITE, NULL,
                         uri_progress_callback, (gpointer) format,
                         error);

  g_object_unref (src_file);
  g_object_unref (dest_file);

  return success;
}
