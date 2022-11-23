/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginprocframe.h
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

#ifndef __LIGMA_PLUG_IN_PROC_FRAME_H__
#define __LIGMA_PLUG_IN_PROC_FRAME_H__


struct _LigmaPlugInProcFrame
{
  gint                 ref_count;

  LigmaContext         *main_context;
  GList               *context_stack;

  LigmaProcedure       *procedure;
  GMainLoop           *main_loop;

  LigmaValueArray      *return_vals;

  LigmaProgress        *progress;
  gboolean             progress_created;
  gulong               progress_cancel_id;

  LigmaPDBErrorHandler  error_handler;

  /*  lists of things to clean up on dispose  */
  GList               *image_cleanups;
  GList               *item_cleanups;
};


LigmaPlugInProcFrame * ligma_plug_in_proc_frame_new     (LigmaContext         *context,
                                                       LigmaProgress        *progress,
                                                       LigmaPlugInProcedure *procedure);
void                  ligma_plug_in_proc_frame_init    (LigmaPlugInProcFrame *proc_frame,
                                                       LigmaContext         *context,
                                                       LigmaProgress        *progress,
                                                       LigmaPlugInProcedure *procedure);

void                  ligma_plug_in_proc_frame_dispose (LigmaPlugInProcFrame *proc_frame,
                                                       LigmaPlugIn          *plug_in);

LigmaPlugInProcFrame * ligma_plug_in_proc_frame_ref     (LigmaPlugInProcFrame *proc_frame);
void                  ligma_plug_in_proc_frame_unref   (LigmaPlugInProcFrame *proc_frame,
                                                       LigmaPlugIn          *plug_in);

LigmaValueArray      * ligma_plug_in_proc_frame_get_return_values
                                                      (LigmaPlugInProcFrame *proc_frame);


#endif /* __LIGMA_PLUG_IN_PROC_FRAME_H__ */
