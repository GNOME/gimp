/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin.h
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

#ifndef __LIGMA_PLUG_IN_H__
#define __LIGMA_PLUG_IN_H__


#include "core/ligmaobject.h"
#include "ligmapluginprocframe.h"


#define WRITE_BUFFER_SIZE  512


#define LIGMA_TYPE_PLUG_IN            (ligma_plug_in_get_type ())
#define LIGMA_PLUG_IN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUG_IN, LigmaPlugIn))
#define LIGMA_PLUG_IN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUG_IN, LigmaPlugInClass))
#define LIGMA_IS_PLUG_IN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUG_IN))
#define LIGMA_IS_PLUG_IN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUG_IN))


typedef struct _LigmaPlugInClass LigmaPlugInClass;

struct _LigmaPlugIn
{
  LigmaObject           parent_instance;

  LigmaPlugInManager   *manager;
  GFile               *file;            /*  Plug-in's full path name          */

  LigmaPlugInCallMode   call_mode;       /*  QUERY, INIT or RUN                */
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

  LigmaPlugInProcFrame  main_proc_frame;

  GList               *temp_proc_frames;

  LigmaPlugInDef       *plug_in_def;     /*  Valid during query() and init()   */
};

struct _LigmaPlugInClass
{
  LigmaObjectClass  parent_class;
};


GType         ligma_plug_in_get_type          (void) G_GNUC_CONST;

LigmaPlugIn  * ligma_plug_in_new               (LigmaPlugInManager      *manager,
                                              LigmaContext            *context,
                                              LigmaProgress           *progress,
                                              LigmaPlugInProcedure    *procedure,
                                              GFile                  *file);

gboolean      ligma_plug_in_open              (LigmaPlugIn             *plug_in,
                                              LigmaPlugInCallMode      call_mode,
                                              gboolean                synchronous);
void          ligma_plug_in_close             (LigmaPlugIn             *plug_in,
                                              gboolean                kill_it);

LigmaPlugInProcFrame *
              ligma_plug_in_get_proc_frame    (LigmaPlugIn             *plug_in);

LigmaPlugInProcFrame *
              ligma_plug_in_proc_frame_push   (LigmaPlugIn             *plug_in,
                                              LigmaContext            *context,
                                              LigmaProgress           *progress,
                                              LigmaTemporaryProcedure *procedure);
void          ligma_plug_in_proc_frame_pop    (LigmaPlugIn             *plug_in);

void          ligma_plug_in_main_loop         (LigmaPlugIn             *plug_in);
void          ligma_plug_in_main_loop_quit    (LigmaPlugIn             *plug_in);

const gchar * ligma_plug_in_get_undo_desc     (LigmaPlugIn             *plug_in);

void          ligma_plug_in_add_temp_proc     (LigmaPlugIn             *plug_in,
                                              LigmaTemporaryProcedure *procedure);
void          ligma_plug_in_remove_temp_proc  (LigmaPlugIn             *plug_in,
                                              LigmaTemporaryProcedure *procedure);

void          ligma_plug_in_set_error_handler (LigmaPlugIn             *plug_in,
                                              LigmaPDBErrorHandler     handler);
LigmaPDBErrorHandler
              ligma_plug_in_get_error_handler (LigmaPlugIn             *plug_in);


#endif /* __LIGMA_PLUG_IN_H__ */
