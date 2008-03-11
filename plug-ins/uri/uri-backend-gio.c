/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * URI plug-in, GIO backend
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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


typedef enum
{
  DOWNLOAD,
  UPLOAD
} Mode;


static gchar    * get_protocols (void);
static gboolean   copy_uri      (const gchar  *src_uri,
                                 const gchar  *dest_uri,
                                 Mode          mode,
                                 GError      **error);

static gchar *supported_protocols = NULL;


gboolean
uri_backend_init (const gchar  *plugin_name,
                  gboolean      run,
                  GimpRunMode   run_mode,
                  GError      **error)
{
  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (plugin_name, FALSE);
    }

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
  gchar *dest_uri = g_filename_to_uri (tmpname, NULL, error);

  if (dest_uri)
    {
      gboolean success = copy_uri (uri, dest_uri, DOWNLOAD, error);

      g_free (dest_uri);

      return success;
    }

  return FALSE;
}

gboolean
uri_backend_save_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar *src_uri = g_filename_to_uri (tmpname, NULL, error);

  if (src_uri)
    {
      gboolean success = copy_uri (src_uri, uri, UPLOAD, error);

      g_free (src_uri);

      return success;
    }

  return FALSE;
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
  Mode mode = GPOINTER_TO_INT (user_data);

  if (total_num_bytes > 0)
    {
      const gchar *format;
      gchar       *done  = gimp_memsize_to_string (current_num_bytes);
      gchar       *total = gimp_memsize_to_string (total_num_bytes);

      switch (mode)
        {
        case DOWNLOAD:
          format = _("Downloading image (%s of %s)");
          break;
        case UPLOAD:
          format = _("Uploading image (%s of %s)");
          break;
        default:
          g_assert_not_reached ();
        }

      gimp_progress_set_text_printf (format, done, total);
      gimp_progress_update (current_num_bytes / total_num_bytes);

      g_free (total);
      g_free (done);
    }
  else
    {
      const gchar *format;
      gchar       *done = gimp_memsize_to_string (current_num_bytes);

      switch (mode)
        {
        case DOWNLOAD:
          format = _("Downloaded %s of image data");
          break;
        case UPLOAD:
          format = _("Uploaded %s of image data");
          break;
        default:
          g_assert_not_reached ();
        }

      gimp_progress_set_text_printf (format, done);
      gimp_progress_pulse ();

      g_free (done);
    }
}

static gboolean
copy_uri (const gchar  *src_uri,
          const gchar  *dest_uri,
          Mode          mode,
          GError      **error)
{
  GVfs     *vfs = g_vfs_get_default ();
  GFile    *src_file;
  GFile    *dest_file;
  gboolean  success;

  vfs = g_vfs_get_default ();

  if (! g_vfs_is_active (vfs))
    {
      g_set_error (error, 0, 0, "Initialization of GVfs failed");
      return FALSE;
    }

  src_file  = g_vfs_get_file_for_uri (vfs, src_uri);
  dest_file = g_vfs_get_file_for_uri (vfs, dest_uri);

  gimp_progress_init (_("Connecting to server"));

  success = g_file_copy (src_file, dest_file, G_FILE_COPY_OVERWRITE, NULL,
                         uri_progress_callback, GINT_TO_POINTER (mode),
                         error);

  g_object_unref (src_file);
  g_object_unref (dest_file);

  return success;
}
