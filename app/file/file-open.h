/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __FILE_OPEN_H__
#define __FILE_OPEN_H__


LigmaImage * file_open_image                 (Ligma                *ligma,
                                             LigmaContext         *context,
                                             LigmaProgress        *progress,
                                             GFile               *file,
                                             gboolean             as_new,
                                             LigmaPlugInProcedure *file_proc,
                                             LigmaRunMode          run_mode,
                                             LigmaPDBStatusType   *status,
                                             const gchar        **mime_type,
                                             GError             **error);

LigmaImage * file_open_thumbnail             (Ligma                *ligma,
                                             LigmaContext         *context,
                                             LigmaProgress        *progress,
                                             GFile               *file,
                                             gint                 size,
                                             const gchar        **mime_type,
                                             gint                *image_width,
                                             gint                *image_height,
                                             const Babl         **format,
                                             gint                *num_layers,
                                             GError             **error);
LigmaImage * file_open_with_display          (Ligma                *ligma,
                                             LigmaContext         *context,
                                             LigmaProgress        *progress,
                                             GFile               *file,
                                             gboolean             as_new,
                                             GObject             *monitor,
                                             LigmaPDBStatusType   *status,
                                             GError             **error);

LigmaImage * file_open_with_proc_and_display (Ligma                *ligma,
                                             LigmaContext         *context,
                                             LigmaProgress        *progress,
                                             GFile               *file,
                                             gboolean             as_new,
                                             LigmaPlugInProcedure *file_proc,
                                             GObject             *monitor,
                                             LigmaPDBStatusType   *status,
                                             GError             **error);

GList     * file_open_layers                (Ligma                *ligma,
                                             LigmaContext         *context,
                                             LigmaProgress        *progress,
                                             LigmaImage           *dest_image,
                                             gboolean             merge_visible,
                                             GFile               *file,
                                             LigmaRunMode          run_mode,
                                             LigmaPlugInProcedure *file_proc,
                                             LigmaPDBStatusType   *status,
                                             GError             **error);

gboolean    file_open_from_command_line     (Ligma                *ligma,
                                             GFile               *file,
                                             gboolean             as_new,
                                             GObject             *monitor);


#endif /* __FILE_OPEN_H__ */
