/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginprocedure.h
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

#include "pdb/gimpprocedure.h"


#define GIMP_TYPE_PLUG_IN_PROCEDURE            (gimp_plug_in_procedure_get_type ())
#define GIMP_PLUG_IN_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN_PROCEDURE, GimpPlugInProcedure))
#define GIMP_PLUG_IN_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN_PROCEDURE, GimpPlugInProcedureClass))
#define GIMP_IS_PLUG_IN_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN_PROCEDURE))
#define GIMP_IS_PLUG_IN_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN_PROCEDURE))
#define GIMP_PLUG_IN_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PLUG_IN_PROCEDURE, GimpPlugInProcedureClass))


typedef struct _GimpPlugInProcedureClass GimpPlugInProcedureClass;

struct _GimpPlugInProcedure
{
  GimpProcedure        parent_instance;

  /*  common members  */
  GFile               *file;
  GQuark               help_domain;
  gchar               *menu_label;
  GList               *menu_paths;
  gchar               *help_id_with_domain;
  GimpIconType         icon_type;
  gint                 icon_data_length;
  guint8              *icon_data;
  gchar               *image_types;
  GimpPlugInImageType  image_types_val;
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
  gboolean             handles_vector;
  gboolean             batch_interpreter;
  gchar               *batch_interpreter_name;
  GSList              *extensions_list;
  GSList              *prefixes_list;
  GSList              *magics_list;
  GSList              *mime_types_list;
  gchar               *thumb_loader;
};

struct _GimpPlugInProcedureClass
{
  GimpProcedureClass parent_class;

  /*  virtual functions  */
  GFile * (* get_file)        (GimpPlugInProcedure *procedure);

  /*  signals  */
  void    (* menu_path_added) (GimpPlugInProcedure *procedure,
                               const gchar         *menu_path);
};


GType           gimp_plug_in_procedure_get_type        (void) G_GNUC_CONST;

GimpProcedure * gimp_plug_in_procedure_new             (GimpPDBProcType      proc_type,
                                                        GFile               *file);

GimpPlugInProcedure * gimp_plug_in_procedure_find      (GSList              *list,
                                                        const gchar         *proc_name);

GFile       * gimp_plug_in_procedure_get_file          (GimpPlugInProcedure *proc);

void          gimp_plug_in_procedure_set_help_domain   (GimpPlugInProcedure *proc,
                                                        const gchar         *help_domain);
const gchar * gimp_plug_in_procedure_get_help_domain   (GimpPlugInProcedure *proc);

gboolean      gimp_plug_in_procedure_set_menu_label    (GimpPlugInProcedure *proc,
                                                        const gchar         *menu_label,
                                                        GError             **error);

gboolean      gimp_plug_in_procedure_add_menu_path     (GimpPlugInProcedure *proc,
                                                        const gchar         *menu_path,
                                                        GError             **error);

gboolean      gimp_plug_in_procedure_set_icon          (GimpPlugInProcedure *proc,
                                                        GimpIconType         type,
                                                        const guint8        *data,
                                                        gint                 data_length,
                                                        GError             **error);
gboolean      gimp_plug_in_procedure_take_icon         (GimpPlugInProcedure *proc,
                                                        GimpIconType         type,
                                                        guint8              *data,
                                                        gint                 data_length,
                                                        GError             **error);

void          gimp_plug_in_procedure_set_image_types   (GimpPlugInProcedure *proc,
                                                        const gchar         *image_types);
void       gimp_plug_in_procedure_set_sensitivity_mask (GimpPlugInProcedure *proc,
                                                        gint                 sensitivity_mask);

void          gimp_plug_in_procedure_set_file_proc     (GimpPlugInProcedure *proc,
                                                        const gchar         *extensions,
                                                        const gchar         *prefixes,
                                                        const gchar         *magics);
void      gimp_plug_in_procedure_set_generic_file_proc (GimpPlugInProcedure *proc,
                                                        gboolean             is_generic_file_proc);
void          gimp_plug_in_procedure_set_priority      (GimpPlugInProcedure *proc,
                                                        gint                 priority);
void          gimp_plug_in_procedure_set_mime_types    (GimpPlugInProcedure *proc,
                                                        const gchar         *mime_ypes);
void          gimp_plug_in_procedure_set_handles_remote(GimpPlugInProcedure *proc);
void          gimp_plug_in_procedure_set_handles_raw   (GimpPlugInProcedure *proc);
void         gimp_plug_in_procedure_set_handles_vector (GimpPlugInProcedure *proc);
void          gimp_plug_in_procedure_set_thumb_loader  (GimpPlugInProcedure *proc,
                                                        const gchar         *thumbnailer);
void      gimp_plug_in_procedure_set_batch_interpreter (GimpPlugInProcedure *proc,
                                                        const gchar         *name);

void       gimp_plug_in_procedure_handle_return_values (GimpPlugInProcedure *proc,
                                                        Gimp                *gimp,
                                                        GimpProgress        *progress,

                                                        GimpValueArray      *return_vals);

