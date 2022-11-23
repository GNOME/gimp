/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginprocedure.h
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

#ifndef __LIGMA_PLUG_IN_PROCEDURE_H__
#define __LIGMA_PLUG_IN_PROCEDURE_H__


#include "pdb/ligmaprocedure.h"


#define LIGMA_TYPE_PLUG_IN_PROCEDURE            (ligma_plug_in_procedure_get_type ())
#define LIGMA_PLUG_IN_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUG_IN_PROCEDURE, LigmaPlugInProcedure))
#define LIGMA_PLUG_IN_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUG_IN_PROCEDURE, LigmaPlugInProcedureClass))
#define LIGMA_IS_PLUG_IN_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUG_IN_PROCEDURE))
#define LIGMA_IS_PLUG_IN_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUG_IN_PROCEDURE))
#define LIGMA_PLUG_IN_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PLUG_IN_PROCEDURE, LigmaPlugInProcedureClass))


typedef struct _LigmaPlugInProcedureClass LigmaPlugInProcedureClass;

struct _LigmaPlugInProcedure
{
  LigmaProcedure        parent_instance;

  /*  common members  */
  GFile               *file;
  GQuark               help_domain;
  gchar               *menu_label;
  GList               *menu_paths;
  gchar               *help_id_with_domain;
  LigmaIconType         icon_type;
  gint                 icon_data_length;
  guint8              *icon_data;
  gchar               *image_types;
  LigmaPlugInImageType  image_types_val;
  gchar               *insensitive_reason;
  gint                 sensitivity_mask;
  gint64               mtime;
  gboolean             installed_during_init;

  /*  file proc specific members  */
  gboolean             file_proc;
  gboolean             generic_file_proc; /* not returning an image. */
  gchar               *extensions;
  gchar               *prefixes;
  gchar               *magics;
  gint                 priority;
  gchar               *mime_types;
  gboolean             handles_remote;
  gboolean             handles_raw;
  gboolean             batch_interpreter;
  gchar               *batch_interpreter_name;
  GSList              *extensions_list;
  GSList              *prefixes_list;
  GSList              *magics_list;
  GSList              *mime_types_list;
  gchar               *thumb_loader;
};

struct _LigmaPlugInProcedureClass
{
  LigmaProcedureClass parent_class;

  /*  virtual functions  */
  GFile * (* get_file)        (LigmaPlugInProcedure *procedure);

  /*  signals  */
  void    (* menu_path_added) (LigmaPlugInProcedure *procedure,
                               const gchar         *menu_path);
};


GType           ligma_plug_in_procedure_get_type        (void) G_GNUC_CONST;

LigmaProcedure * ligma_plug_in_procedure_new             (LigmaPDBProcType      proc_type,
                                                        GFile               *file);

LigmaPlugInProcedure * ligma_plug_in_procedure_find      (GSList              *list,
                                                        const gchar         *proc_name);

GFile       * ligma_plug_in_procedure_get_file          (LigmaPlugInProcedure *proc);

void          ligma_plug_in_procedure_set_help_domain   (LigmaPlugInProcedure *proc,
                                                        const gchar         *help_domain);
const gchar * ligma_plug_in_procedure_get_help_domain   (LigmaPlugInProcedure *proc);

gboolean      ligma_plug_in_procedure_set_menu_label    (LigmaPlugInProcedure *proc,
                                                        const gchar         *menu_label,
                                                        GError             **error);

gboolean      ligma_plug_in_procedure_add_menu_path     (LigmaPlugInProcedure *proc,
                                                        const gchar         *menu_path,
                                                        GError             **error);

gboolean      ligma_plug_in_procedure_set_icon          (LigmaPlugInProcedure *proc,
                                                        LigmaIconType         type,
                                                        const guint8        *data,
                                                        gint                 data_length,
                                                        GError             **error);
gboolean      ligma_plug_in_procedure_take_icon         (LigmaPlugInProcedure *proc,
                                                        LigmaIconType         type,
                                                        guint8              *data,
                                                        gint                 data_length,
                                                        GError             **error);

void          ligma_plug_in_procedure_set_image_types   (LigmaPlugInProcedure *proc,
                                                        const gchar         *image_types);
void       ligma_plug_in_procedure_set_sensitivity_mask (LigmaPlugInProcedure *proc,
                                                        gint                 sensitivity_mask);

void          ligma_plug_in_procedure_set_file_proc     (LigmaPlugInProcedure *proc,
                                                        const gchar         *extensions,
                                                        const gchar         *prefixes,
                                                        const gchar         *magics);
void      ligma_plug_in_procedure_set_generic_file_proc (LigmaPlugInProcedure *proc,
                                                        gboolean             is_generic_file_proc);
void          ligma_plug_in_procedure_set_priority      (LigmaPlugInProcedure *proc,
                                                        gint                 priority);
void          ligma_plug_in_procedure_set_mime_types    (LigmaPlugInProcedure *proc,
                                                        const gchar         *mime_ypes);
void          ligma_plug_in_procedure_set_handles_remote(LigmaPlugInProcedure *proc);
void          ligma_plug_in_procedure_set_handles_raw   (LigmaPlugInProcedure *proc);
void          ligma_plug_in_procedure_set_thumb_loader  (LigmaPlugInProcedure *proc,
                                                        const gchar         *thumbnailer);
void      ligma_plug_in_procedure_set_batch_interpreter (LigmaPlugInProcedure *proc,
                                                        const gchar         *name);

void       ligma_plug_in_procedure_handle_return_values (LigmaPlugInProcedure *proc,
                                                        Ligma                *ligma,
                                                        LigmaProgress        *progress,

                                                        LigmaValueArray      *return_vals);


#endif /* __LIGMA_PLUG_IN_PROCEDURE_H__ */
