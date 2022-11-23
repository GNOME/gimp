/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-cleanup.h
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

#ifndef __LIGMA_PLUG_IN_CLEANUP_H__
#define __LIGMA_PLUG_IN_CLEANUP_H__


gboolean   ligma_plug_in_cleanup_undo_group_start (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);
gboolean   ligma_plug_in_cleanup_undo_group_end   (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);

gboolean   ligma_plug_in_cleanup_layers_freeze    (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);
gboolean   ligma_plug_in_cleanup_layers_thaw      (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);

gboolean   ligma_plug_in_cleanup_channels_freeze  (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);
gboolean   ligma_plug_in_cleanup_channels_thaw    (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);

gboolean   ligma_plug_in_cleanup_vectors_freeze   (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);
gboolean   ligma_plug_in_cleanup_vectors_thaw     (LigmaPlugIn          *plug_in,
                                                  LigmaImage           *image);

gboolean   ligma_plug_in_cleanup_add_shadow       (LigmaPlugIn          *plug_in,
                                                  LigmaDrawable        *drawable);
gboolean   ligma_plug_in_cleanup_remove_shadow    (LigmaPlugIn          *plug_in,
                                                  LigmaDrawable        *drawable);

void       ligma_plug_in_cleanup                  (LigmaPlugIn          *plug_in,
                                                  LigmaPlugInProcFrame *proc_frame);


#endif /* __LIGMA_PLUG_IN_CLEANUP_H__ */
