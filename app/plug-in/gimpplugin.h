/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin.h
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

#ifndef __GIMP_PLUG_IN_H__
#define __GIMP_PLUG_IN_H__


#include "core/gimpobject.h"
#include "gimppluginprocframe.h"


#define WRITE_BUFFER_SIZE  512


#define GIMP_TYPE_PLUG_IN            (gimp_plug_in_get_type ())
#define GIMP_PLUG_IN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN, GimpPlugIn))
#define GIMP_PLUG_IN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN, GimpPlugInClass))
#define GIMP_IS_PLUG_IN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN))
#define GIMP_IS_PLUG_IN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN))


typedef struct _GimpPlugInClass GimpPlugInClass;

struct _GimpPlugIn
{
  GimpObject           parent_instance;

  GimpPlugInManager   *manager;
  GFile               *file;            /*  Plug-in's full path name          */

  GimpDisplay         *display;         /*  The display this plug-in was called from. */
  GimpPlugInCallMode   call_mode;       /*  QUERY, INIT or RUN                */
  guint                open : 1;        /*  Is the plug-in open?              */
  guint                hup : 1;         /*  Did we receive a G_IO_HUP         */
  GPid                 pid;             /*  Plug-in's process id              */

  GIOChannel          *my_read;         /*  App's read and write channels     */
  GIOChannel          *my_write;
  GIOChannel          *his_read;        /*  Plug-in's read and write channels */
  GIOChannel          *his_write;

  guint                input_id;        /*  Id of input proc                  */

  gchar                write_buffer[WRITE_BUFFER_SIZE]; /* Buffer for writing */
  gint                 write_buffer_index;              /* Buffer index       */

  GSList              *temp_procedures; /*  Temporary procedures              */

  GMainLoop           *ext_main_loop;   /*  for waiting for extension_ack     */

  GimpPlugInProcFrame  main_proc_frame;

  GList               *temp_proc_frames;

  GimpPlugInDef       *plug_in_def;     /*  Valid during query() and init()   */
};

struct _GimpPlugInClass
{
  GimpObjectClass  parent_class;
};


GType         gimp_plug_in_get_type          (void) G_GNUC_CONST;

GimpPlugIn  * gimp_plug_in_new               (GimpPlugInManager      *manager,
                                              GimpContext            *context,
                                              GimpProgress           *progress,
                                              GimpPlugInProcedure    *procedure,
                                              GFile                  *file,
                                              GimpDisplay            *display);

gboolean      gimp_plug_in_open              (GimpPlugIn             *plug_in,
                                              GimpPlugInCallMode      call_mode,
                                              gboolean                synchronous);
void          gimp_plug_in_close             (GimpPlugIn             *plug_in,
                                              gboolean                kill_it);

GimpPlugInProcFrame *
              gimp_plug_in_get_proc_frame    (GimpPlugIn             *plug_in);

GimpPlugInProcFrame *
              gimp_plug_in_proc_frame_push   (GimpPlugIn             *plug_in,
                                              GimpContext            *context,
                                              GimpProgress           *progress,
                                              GimpTemporaryProcedure *procedure);
void          gimp_plug_in_proc_frame_pop    (GimpPlugIn             *plug_in);

void          gimp_plug_in_main_loop         (GimpPlugIn             *plug_in);
void          gimp_plug_in_main_loop_quit    (GimpPlugIn             *plug_in);

const gchar * gimp_plug_in_get_undo_desc     (GimpPlugIn             *plug_in);

void          gimp_plug_in_add_temp_proc     (GimpPlugIn             *plug_in,
                                              GimpTemporaryProcedure *procedure);
void          gimp_plug_in_remove_temp_proc  (GimpPlugIn             *plug_in,
                                              GimpTemporaryProcedure *procedure);

void          gimp_plug_in_set_error_handler (GimpPlugIn             *plug_in,
                                              GimpPDBErrorHandler     handler);
GimpPDBErrorHandler
              gimp_plug_in_get_error_handler (GimpPlugIn             *plug_in);


#endif /* __GIMP_PLUG_IN_H__ */
