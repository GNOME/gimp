/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-proc-frame.h
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

#ifndef __PLUG_IN_PROC_FRAME_H__
#define __PLUG_IN_PRON_FRAME_H__


struct _PlugInProcFrame
{
  gint          ref_count;

  GimpContext  *main_context;
  GList        *context_stack;

  ProcRecord   *proc_rec;
  GMainLoop    *main_loop;

  Argument     *return_vals;
  gint          n_return_vals;

  GimpProgress *progress;
  gboolean      progress_created;
  gulong        progress_cancel_id;
};


PlugInProcFrame * plug_in_proc_frame_new     (GimpContext     *context,
                                              GimpProgress    *progress,
                                              ProcRecord      *proc_rec);
void              plug_in_proc_frame_init    (PlugInProcFrame *proc_frame,
                                              GimpContext     *context,
                                              GimpProgress    *progress,
                                              ProcRecord      *proc_rec);

void              plug_in_proc_frame_dispose (PlugInProcFrame *proc_frame,
                                              PlugIn          *plug_in);

PlugInProcFrame * plug_in_proc_frame_ref     (PlugInProcFrame *proc_frame);
void              plug_in_proc_frame_unref   (PlugInProcFrame *proc_frame,
                                              PlugIn          *plug_in);


#endif /* __PLUG_IN_PROC_FRAME_H__ */
