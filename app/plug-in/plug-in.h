/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __PLUG_IN_H__
#define __PLUG_IN_H__


#include <sys/types.h> /* pid_t  */


#define WRITE_BUFFER_SIZE  512


struct _PlugIn
{
  Gimp         *gimp;
  GimpContext  *context;

  gint          ref_count;

  ProcRecord   *proc_rec;         /*  The procedure the plug-in is running    */

  guint         open : 1;         /*  Is the plug-in open?                    */
  guint         query : 1;        /*  Are we querying the plug-in?            */
  guint         init : 1;         /*  Are we initialing the plug-in?          */
  guint         synchronous : 1;  /*  Is the plug-in running synchronously?   */

  GPid          pid;              /*  Plug-in's process id                    */

  gchar        *name;             /*  Plug-in's name                          */
  gchar        *prog;             /*  Plug-in's full path name                */

  GIOChannel   *my_read;          /*  App's read and write channels           */
  GIOChannel   *my_write;
  GIOChannel   *his_read;         /*  Plug-in's read and write channels       */
  GIOChannel   *his_write;

  guint         input_id;         /*  Id of input proc                        */

  gchar         write_buffer[WRITE_BUFFER_SIZE]; /*  Buffer for writing       */
  gint          write_buffer_index;              /*  Buffer index for writing */

  GSList       *temp_proc_defs;   /*  Temporary procedures                    */

  GMainLoop    *ext_main_loop;    /*  for waiting for extension_ack           */
  GMainLoop    *recurse_main_loop;/*  for waiting for proc_return             */

  GList        *temp_main_loops;  /*  for waiting for temp_proc_returns       */
  GList        *temp_proc_recs;   /*  The temp procs the plug-in is busy with */

  Argument     *return_vals;      /*  The return value we wait for            */
  gint          n_return_vals;

  GimpProgress *progress;         /*  Progress dialog                         */
  gboolean      progress_created; /*  Was the progress created by the plug-in */
  gulong        progress_cancel_id;

  PlugInDef    *plug_in_def;      /*  Valid only during query() and init()    */
};


void       plug_in_init           (Gimp        *gimp);
void       plug_in_exit           (Gimp        *gimp);

void       plug_in_call_query     (Gimp        *gimp,
                                   GimpContext *context,
                                   PlugInDef   *plug_in_def);
void       plug_in_call_init      (Gimp        *gimp,
                                   GimpContext *context,
                                   PlugInDef   *plug_in_def);

PlugIn   * plug_in_new            (Gimp        *gimp,
                                   GimpContext *context,
                                   ProcRecord  *proc_rec,
                                   const gchar *prog);

void       plug_in_ref            (PlugIn      *plug_in);
void       plug_in_unref          (PlugIn      *plug_in);

gboolean   plug_in_open           (PlugIn      *plug_in);
void       plug_in_close          (PlugIn      *plug_in,
                                   gboolean     kill_it);

void       plug_in_push           (Gimp        *gimp,
                                   PlugIn      *plug_in);
void       plug_in_pop            (Gimp        *gimp);

void       plug_in_main_loop      (PlugIn      *plug_in);
void       plug_in_main_loop_quit (PlugIn      *plug_in);

gchar    * plug_in_get_undo_desc  (PlugIn      *plug_in);


#endif /* __PLUG_IN_H__ */
