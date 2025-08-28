/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-open.h
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

#pragma once


GimpImage * file_open_image                 (Gimp                *gimp,
                                             GimpContext         *context,
                                             GimpProgress        *progress,
                                             GFile               *file,
                                             gint                 vector_width,
                                             gint                 vector_height,
                                             gboolean             vector_keep_ratio,
                                             gboolean             as_new,
                                             GimpPlugInProcedure *file_proc,
                                             GimpRunMode          run_mode,
                                             gboolean            *file_proc_handles_vector,
                                             GimpPDBStatusType   *status,
                                             const gchar        **mime_type,
                                             GError             **error);

GimpImage * file_open_thumbnail             (Gimp                *gimp,
                                             GimpContext         *context,
                                             GimpProgress        *progress,
                                             GFile               *file,
                                             gint                 size,
                                             const gchar        **mime_type,
                                             gint                *image_width,
                                             gint                *image_height,
                                             const Babl         **format,
                                             gint                *num_layers,
                                             GError             **error);
GimpImage * file_open_with_display          (Gimp                *gimp,
                                             GimpContext         *context,
                                             GimpProgress        *progress,
                                             GFile               *file,
                                             gboolean             as_new,
                                             GObject             *monitor,
                                             GimpPDBStatusType   *status,
                                             GError             **error);

GimpImage * file_open_with_proc_and_display (Gimp                *gimp,
                                             GimpContext         *context,
                                             GimpProgress        *progress,
                                             GFile               *file,
                                             gboolean             as_new,
                                             gboolean             as_link,
                                             GimpPlugInProcedure *file_proc,
                                             GObject             *monitor,
                                             GimpPDBStatusType   *status,
                                             GError             **error);

GList     * file_open_layers                (Gimp                *gimp,
                                             GimpContext         *context,
                                             GimpProgress        *progress,
                                             GimpImage           *dest_image,
                                             gboolean             merge_visible,
                                             gboolean             as_link,
                                             GFile               *file,
                                             GimpRunMode          run_mode,
                                             GimpPlugInProcedure *file_proc,
                                             GimpPDBStatusType   *status,
                                             GError             **error);

gboolean    file_open_from_command_line     (Gimp                *gimp,
                                             GFile               *file,
                                             gboolean             as_new,
                                             GObject             *monitor);
