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

  guint         open : 1;         /*  Is the plug-in open*                    */
  guint         query : 1;        /*  Are we querying the plug-in?            */
  guint         init : 1;         /*  Are we initialing the plug-in?          */
  guint         synchronous : 1;  /*  Is the plug-in running synchronously?   */
  guint         recurse : 1;      /*  Have we called 'gtk_main' recursively?  */
  guint         busy : 1;         /*  Is the plug-in busy with a temp proc?   */
  pid_t         pid;              /*  Plug-ins process id                     */
  gchar        *args[7];          /*  Plug-ins command line arguments         */

  GIOChannel   *my_read;          /*  App's read and write channels           */
  GIOChannel   *my_write;
  GIOChannel   *his_read;         /*  Plug-in's read and write channels       */
  GIOChannel   *his_write;
#ifdef G_OS_WIN32
  guint         his_thread_id;    /*  Plug-in's thread ID                     */
  gint          his_read_fd;      /*  Plug-in's read pipe fd                  */
#endif

  guint32       input_id;         /*  Id of input proc                        */

  gchar         write_buffer[WRITE_BUFFER_SIZE]; /*  Buffer for writing       */
  gint          write_buffer_index;              /*  Buffer index for writing */

  GSList       *temp_proc_defs;   /*  Temporary procedures                    */

  GimpProgress *progress;         /*  Progress dialog                         */

  PlugInDef    *user_data;        /*  DON'T USE!!                             */
};


struct _PlugInMenuEntry
{
  PlugInProcDef *proc_def;
  const gchar   *domain;
  const gchar   *help_path;
};


void       plug_in_init       (Gimp       *gimp);
void       plug_in_kill       (Gimp       *gimp);

void       plug_in_call_query (Gimp       *gimp,
                               PlugInDef  *plug_in_def);
void       plug_in_call_init  (Gimp       *gimp,
                               PlugInDef  *plug_in_def);

/*  Create a new plug-in structure
 */
PlugIn   * plug_in_new        (Gimp       *gimp,
                               gchar      *name);

/*  Destroy a plug-in structure.
 *  This will close the plug-in first if necessary.
 */
void       plug_in_destroy    (PlugIn     *plug_in);


/*  Open a plug-in. This cause the plug-in to run.
 *  If returns TRUE, you must destroy the plugin.
 *  If returns FALSE, you must not destroy the plugin.
 */
gboolean   plug_in_open       (PlugIn     *plug_in);

/*  Close a plug-in. This kills the plug-in and releases its resources.
 */
void       plug_in_close      (PlugIn     *plug_in,
                               gboolean    kill_it);

/*  Run a plug-in as if it were a procedure database procedure
 */
Argument * plug_in_run        (Gimp       *gimp,
                               ProcRecord *proc_rec,
                               Argument   *args,
                               gint        argc,
                               gboolean    synchronous,
                               gboolean    destroy_values,
                               gint        gdisp_ID);

/*  Run the last plug-in again with the same arguments. Extensions
 *  are exempt from this "privelege".
 */
void       plug_in_repeat     (Gimp       *gimp,
                               gint        display_ID,
                               gint        image_ID,
                               gint        drawable_ID,
                               gboolean    with_interface);


extern PlugIn     *current_plug_in;
extern ProcRecord *last_plug_in;


#endif /* __PLUG_IN_H__ */
