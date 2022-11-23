/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-progress.h
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

#ifndef __LIGMA_PLUG_IN_PROGRESS_H__
#define __LIGMA_PLUG_IN_PROGRESS_H__


gint       ligma_plug_in_progress_attach        (LigmaProgress        *progress);
gint       ligma_plug_in_progress_detach        (LigmaProgress        *progress);

void       ligma_plug_in_progress_start         (LigmaPlugIn          *plug_in,
                                                const gchar         *message,
                                                LigmaDisplay         *display);
void       ligma_plug_in_progress_end           (LigmaPlugIn          *plug_in,
                                                LigmaPlugInProcFrame *proc_frame);
void       ligma_plug_in_progress_set_text      (LigmaPlugIn          *plug_in,
                                                const gchar         *message);
void       ligma_plug_in_progress_set_value     (LigmaPlugIn          *plug_in,
                                                gdouble              percentage);
void       ligma_plug_in_progress_pulse         (LigmaPlugIn          *plug_in);
guint32    ligma_plug_in_progress_get_window_id (LigmaPlugIn          *plug_in);

gboolean   ligma_plug_in_progress_install       (LigmaPlugIn          *plug_in,
                                                const gchar         *progress_callback);
gboolean   ligma_plug_in_progress_uninstall     (LigmaPlugIn          *plug_in,
                                                const gchar         *progress_callback);
gboolean   ligma_plug_in_progress_cancel        (LigmaPlugIn          *plug_in,
                                                const gchar         *progress_callback);


#endif /* __LIGMA_PLUG_IN_PROGRESS_H__ */
