/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * file-remote.c
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *
 * Based on: URI plug-in, GIO/GVfs backend
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpprogress.h"

#include "file-remote.h"

#include "gimp-intl.h"


typedef enum
{
  DOWNLOAD,
  UPLOAD
} RemoteCopyMode;

typedef struct
{
  GimpProgress *progress;
  GCancellable *cancellable;
  gboolean      cancel;
  GMainLoop    *main_loop;
  GError       *error;
} RemoteMount;

typedef struct
{
  RemoteCopyMode  mode;
  GimpProgress   *progress;
  GCancellable   *cancellable;
  gboolean        cancel;
  gint64          last_time;
} RemoteProgress;


static void       file_remote_mount_volume_ready (GFile           *file,
                                                  GAsyncResult    *result,
                                                  RemoteMount     *mount);
static void       file_remote_mount_file_cancel  (GimpProgress    *progress,
                                                  RemoteMount     *mount);

static GFile    * file_remote_get_temp_file      (Gimp            *gimp,
                                                  GFile           *file);
static gboolean   file_remote_copy_file          (Gimp            *gimp,
                                                  GFile           *src_file,
                                                  GFile           *dest_file,
                                                  RemoteCopyMode   mode,
                                                  GimpProgress    *progress,
                                                  GError         **error);
static void       file_remote_copy_file_cancel   (GimpProgress    *progress,
                                                  RemoteProgress  *remote_progress);

static void       file_remote_progress_callback  (goffset          current_num_bytes,
                                                  goffset          total_num_bytes,
                                                  gpointer         user_data);


/*  public functions  */

gboolean
file_remote_mount_file (Gimp          *gimp,
                        GFile         *file,
                        GimpProgress  *progress,
                        GError       **error)
{
  GMountOperation *operation;
  RemoteMount      mount = { 0, };

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  mount.progress  = progress;
  mount.main_loop = g_main_loop_new (NULL, FALSE);

  operation = gimp_get_mount_operation (gimp, progress);

  if (progress)
    {
      gimp_progress_start (progress, TRUE, _("Mounting remote volume"));

      mount.cancellable = g_cancellable_new ();

      g_signal_connect (progress, "cancel",
                        G_CALLBACK (file_remote_mount_file_cancel),
                        &mount);
    }

  g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE,
                                 operation, mount.cancellable,
                                 (GAsyncReadyCallback) file_remote_mount_volume_ready,
                                 &mount);

  g_main_loop_run (mount.main_loop);
  g_main_loop_unref (mount.main_loop);

  if (progress)
    {
      g_signal_handlers_disconnect_by_func (progress,
                                            file_remote_mount_file_cancel,
                                            &mount);

      g_object_unref (mount.cancellable);

      gimp_progress_end (progress);
    }

  g_object_unref (operation);

  if (mount.error)
    {
      if (mount.error->domain != G_IO_ERROR ||
          mount.error->code   != G_IO_ERROR_ALREADY_MOUNTED)
        {
          g_propagate_error (error, mount.error);
          return FALSE;
        }
      else
        {
          g_clear_error (&mount.error);
        }
    }

  return TRUE;
}

GFile *
file_remote_download_image (Gimp          *gimp,
                            GFile         *file,
                            GimpProgress  *progress,
                            GError       **error)
{
  GFile *local_file;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  local_file = file_remote_get_temp_file (gimp, file);

  if (! file_remote_copy_file (gimp, file, local_file, DOWNLOAD,
                               progress, error))
    {
      g_object_unref (local_file);
      return NULL;
    }

  return local_file;
}

GFile *
file_remote_upload_image_prepare (Gimp          *gimp,
                                  GFile         *file,
                                  GimpProgress  *progress,
                                  GError       **error)
{
  GFile *local_file;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  local_file = file_remote_get_temp_file (gimp, file);

  return local_file;
}

gboolean
file_remote_upload_image_finish (Gimp          *gimp,
                                 GFile         *file,
                                 GFile         *local_file,
                                 GimpProgress  *progress,
                                 GError       **error)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (G_IS_FILE (local_file), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! file_remote_copy_file (gimp, local_file, file, UPLOAD,
                               progress, error))
    {
      return FALSE;
    }

  return TRUE;
}


/*  private functions  */

static void
file_remote_mount_volume_ready (GFile        *file,
                                GAsyncResult *result,
                                RemoteMount  *mount)
{
  g_file_mount_enclosing_volume_finish (file, result, &mount->error);

  g_main_loop_quit (mount->main_loop);
}

static void
file_remote_mount_file_cancel (GimpProgress *progress,
                               RemoteMount  *mount)
{
  mount->cancel = TRUE;

  g_cancellable_cancel (mount->cancellable);
}

static GFile *
file_remote_get_temp_file (Gimp  *gimp,
                           GFile *file)
{
  gchar *basename;
  GFile *temp_file = NULL;

  basename = g_path_get_basename (gimp_file_get_utf8_name (file));

  if (basename)
    {
      const gchar *ext = strchr (basename, '.');

      if (ext && strlen (ext))
        temp_file = gimp_get_temp_file (gimp, ext + 1);

      g_free (basename);
    }

  if (! temp_file)
    temp_file = gimp_get_temp_file (gimp, "xxx");

  return temp_file;
}

static gboolean
file_remote_copy_file (Gimp            *gimp,
                       GFile           *src_file,
                       GFile           *dest_file,
                       RemoteCopyMode   mode,
                       GimpProgress    *progress,
                       GError         **error)
{
  RemoteProgress  remote_progress = { 0, };
  gboolean        success;
  GError         *my_error = NULL;

  remote_progress.mode     = mode;
  remote_progress.progress = progress;

  if (progress)
    {
      gimp_progress_start (progress, TRUE, _("Opening remote file"));

      remote_progress.cancellable = g_cancellable_new ();

      g_signal_connect (progress, "cancel",
                        G_CALLBACK (file_remote_copy_file_cancel),
                        &remote_progress);

      success = g_file_copy (src_file, dest_file, G_FILE_COPY_OVERWRITE,
                             remote_progress.cancellable,
                             file_remote_progress_callback, &remote_progress,
                             &my_error);

      g_signal_handlers_disconnect_by_func (progress,
                                            file_remote_copy_file_cancel,
                                            &remote_progress);

      g_object_unref (remote_progress.cancellable);

      gimp_progress_set_value (progress, 1.0);
      gimp_progress_end (progress);
    }
  else
    {
      success = g_file_copy (src_file, dest_file, G_FILE_COPY_OVERWRITE,
                             NULL, NULL, NULL,
                             &my_error);
    }

  return success;
}

static void
file_remote_copy_file_cancel (GimpProgress   *progress,
                              RemoteProgress *remote_progress)
{
  remote_progress->cancel = TRUE;

  g_cancellable_cancel (remote_progress->cancellable);
}

static void
file_remote_progress_callback (goffset  current_num_bytes,
                               goffset  total_num_bytes,
                               gpointer user_data)
{
  RemoteProgress *progress = user_data;
  gint64          now;

  /*  update the progress only up to 10 times a second  */
  now = g_get_monotonic_time ();

  if ((now - progress->last_time) / 1000 < 100)
    return;

  progress->last_time = now;

  if (total_num_bytes > 0)
    {
      const gchar *format;
      gchar       *done  = g_format_size (current_num_bytes);
      gchar       *total = g_format_size (total_num_bytes);

      switch (progress->mode)
        {
        case DOWNLOAD:
          format = _("Downloading image (%s of %s)");
          break;

        case UPLOAD:
          format = _("Uploading image (%s of %s)");
          break;

        default:
          gimp_assert_not_reached ();
        }

      gimp_progress_set_text (progress->progress, format, done, total);
      g_free (total);
      g_free (done);

      gimp_progress_set_value (progress->progress,
                               (gdouble) current_num_bytes /
                               (gdouble) total_num_bytes);
    }
  else
    {
      const gchar *format;
      gchar       *done = g_format_size (current_num_bytes);

      switch (progress->mode)
        {
        case DOWNLOAD:
          format = _("Downloaded %s of image data");
          break;

        case UPLOAD:
          format = _("Uploaded %s of image data");
          break;

        default:
          gimp_assert_not_reached ();
        }

      gimp_progress_set_text (progress->progress, format, done);
      g_free (done);

      gimp_progress_pulse (progress->progress);
    }

  while (! progress->cancel && g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}
