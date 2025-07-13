/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-progress.h
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


gint       gimp_plug_in_progress_attach        (GimpProgress        *progress);
gint       gimp_plug_in_progress_detach        (GimpProgress        *progress);

void       gimp_plug_in_progress_start         (GimpPlugIn          *plug_in,
                                                const gchar         *message,
                                                GimpDisplay         *display);
void       gimp_plug_in_progress_end           (GimpPlugIn          *plug_in,
                                                GimpPlugInProcFrame *proc_frame);
void       gimp_plug_in_progress_set_text      (GimpPlugIn          *plug_in,
                                                const gchar         *message);
void       gimp_plug_in_progress_set_value     (GimpPlugIn          *plug_in,
                                                gdouble              percentage);
void       gimp_plug_in_progress_pulse         (GimpPlugIn          *plug_in);
GBytes   * gimp_plug_in_progress_get_window_id (GimpPlugIn          *plug_in);

gboolean   gimp_plug_in_progress_install       (GimpPlugIn          *plug_in,
                                                const gchar         *progress_callback);
gboolean   gimp_plug_in_progress_uninstall     (GimpPlugIn          *plug_in,
                                                const gchar         *progress_callback);
gboolean   gimp_plug_in_progress_cancel        (GimpPlugIn          *plug_in,
                                                const gchar         *progress_callback);
