/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-cleanup.h
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

#ifndef __GIMP_PLUG_IN_CLEANUP_H__
#define __GIMP_PLUG_IN_CLEANUP_H__


gboolean   gimp_plug_in_cleanup_undo_group_start (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);
gboolean   gimp_plug_in_cleanup_undo_group_end   (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);

gboolean   gimp_plug_in_cleanup_layers_freeze    (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);
gboolean   gimp_plug_in_cleanup_layers_thaw      (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);

gboolean   gimp_plug_in_cleanup_channels_freeze  (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);
gboolean   gimp_plug_in_cleanup_channels_thaw    (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);

gboolean   gimp_plug_in_cleanup_paths_freeze     (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);
gboolean   gimp_plug_in_cleanup_paths_thaw       (GimpPlugIn          *plug_in,
                                                  GimpImage           *image);

gboolean   gimp_plug_in_cleanup_add_shadow       (GimpPlugIn          *plug_in,
                                                  GimpDrawable        *drawable);
gboolean   gimp_plug_in_cleanup_remove_shadow    (GimpPlugIn          *plug_in,
                                                  GimpDrawable        *drawable);

void       gimp_plug_in_cleanup                  (GimpPlugIn          *plug_in,
                                                  GimpPlugInProcFrame *proc_frame);


#endif /* __GIMP_PLUG_IN_CLEANUP_H__ */
