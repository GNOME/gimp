/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * URI plug-in, GIO/GVfs backend
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

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


static gchar    * get_protocols          (void);
static gboolean   copy_uri               (const gchar  *src_uri,
                                          const gchar  *dest_uri,
                                          Mode          mode,
                                          GimpRunMode   run_mode,
                                          GError      **error);
static gboolean   mount_enclosing_volume (GFile        *file,
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
      gboolean success = copy_uri (uri, dest_uri, DOWNLOAD, run_mode, error);

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
      gboolean success = copy_uri (src_uri, uri, UPLOAD, run_mode, error);

      g_free (src_uri);

      return success;
    }

  return FALSE;
}

gchar *
uri_backend_map_image (const gchar  *uri,
                       GimpRunMode   run_mode)
{
  GFile    *file    = g_file_new_for_uri (uri);
  gchar    *path    = NULL;
  gboolean  success = TRUE;

  if (! file)
    return NULL;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      GError *error = NULL;

      if (! mount_enclosing_volume (file, &error))
        {

          if (error->domain == G_IO_ERROR   &&
              error->code   == G_IO_ERROR_ALREADY_MOUNTED)
            success = TRUE;

          g_error_free (error);
        }
    }

  if (success)
    path = g_file_get_path (file);

  g_object_unref (file);

  return path;
}


/*  private functions  */

static gchar *
get_protocols (void)
{
  const gchar * const *schemes;
  GString             *string = g_string_new (NULL);
  gint                 i;

  schemes = g_vfs_get_supported_uri_schemes (g_vfs_get_default ());

  for (i = 0; schemes && schemes[i]; i++)
    {
      if (string->len > 0)
        g_string_append_c (string, ',');

      g_string_append (string, schemes[i]);
      g_string_append_c (string, ':');
    }

  return g_string_free (string, FALSE);
}

typedef struct
{
  Mode     mode;
  GTimeVal last_time;
} UriProgress;

static void
uri_progress_callback (goffset  current_num_bytes,
                       goffset  total_num_bytes,
                       gpointer user_data)
{
  UriProgress *progress = user_data;
  GTimeVal     now;

  /*  update the progress only up to 10 times a second  */
  g_get_current_time (&now);

  if (progress->last_time.tv_sec &&
      ((now.tv_sec - progress->last_time.tv_sec) * 1000 +
       (now.tv_usec - progress->last_time.tv_usec) / 1000) < 100)
    return;

  progress->last_time = now;

  if (total_num_bytes > 0)
    {
      const gchar *format;
      gchar       *done  = g_format_size_for_display (current_num_bytes);
      gchar       *total = g_format_size_for_display (total_num_bytes);

      switch (progress->mode)
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
      gimp_progress_update ((gdouble) current_num_bytes /
                            (gdouble) total_num_bytes);

      g_free (total);
      g_free (done);
    }
  else
    {
      const gchar *format;
      gchar       *done = g_format_size_for_display (current_num_bytes);

      switch (progress->mode)
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

static void
mount_volume_ready (GFile         *file,
                    GAsyncResult  *res,
                    GError       **error)
{
  g_file_mount_enclosing_volume_finish (file, res, error);

  gtk_main_quit ();
}

static gboolean
mount_enclosing_volume (GFile   *file,
                        GError **error)
{
  GMountOperation *operation = gtk_mount_operation_new (NULL);

  g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE,
                                 operation,
                                 NULL,
                                 (GAsyncReadyCallback) mount_volume_ready,
                                 error);
  gtk_main ();

  g_object_unref (operation);

  return (*error == NULL);
}

static gboolean
copy_uri (const gchar  *src_uri,
          const gchar  *dest_uri,
          Mode          mode,
          GimpRunMode   run_mode,
          GError      **error)
{
  GFile       *src_file;
  GFile       *dest_file;
  UriProgress  progress = { 0, };
  gboolean     success;

  gimp_progress_init (_("Connecting to server"));

  progress.mode = mode;

  src_file  = g_file_new_for_uri (src_uri);
  dest_file = g_file_new_for_uri (dest_uri);

  success = g_file_copy (src_file, dest_file, G_FILE_COPY_OVERWRITE, NULL,
                         uri_progress_callback, &progress,
                         error);

  if (! success &&
      run_mode == GIMP_RUN_INTERACTIVE &&
      (*error)->domain == G_IO_ERROR   &&
      (*error)->code   == G_IO_ERROR_NOT_MOUNTED)
    {
      g_clear_error (error);

      if (mount_enclosing_volume (mode == DOWNLOAD ? src_file : dest_file,
                                  error))
        {
          success = g_file_copy (src_file, dest_file, 0, NULL,
                                 uri_progress_callback, &progress,
                                 error);

        }
    }

  g_object_unref (src_file);
  g_object_unref (dest_file);

  return success;
}
