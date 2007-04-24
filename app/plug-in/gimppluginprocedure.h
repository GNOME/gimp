/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginprocedure.h
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

#ifndef __GIMP_PLUG_IN_PROCEDURE_H__
#define __GIMP_PLUG_IN_PROCEDURE_H__

#include <time.h>      /* time_t */

#include <gdk-pixbuf/gdk-pixbuf.h>

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
  gchar               *prog;
  GQuark               locale_domain;
  GQuark               help_domain;
  gchar               *menu_label;
  GList               *menu_paths;
  gchar               *label;
  GimpIconType         icon_type;
  gint                 icon_data_length;
  guint8              *icon_data;
  gchar               *image_types;
  GimpPlugInImageType  image_types_val;
  time_t               mtime;
  gboolean             installed_during_init;

  /*  file proc specific members  */
  gboolean             file_proc;
  gchar               *extensions;
  gchar               *prefixes;
  gchar               *magics;
  gchar               *mime_type;
  GSList              *extensions_list;
  GSList              *prefixes_list;
  GSList              *magics_list;
  gchar               *thumb_loader;
};

struct _GimpPlugInProcedureClass
{
  GimpProcedureClass parent_class;

  /*  virtual functions  */
  const gchar * (* get_progname)    (const GimpPlugInProcedure *procedure);

  /*  signals  */
  void          (* menu_path_added) (GimpPlugInProcedure       *procedure,
                                     const gchar               *menu_path);
};


GType           gimp_plug_in_procedure_get_type      (void) G_GNUC_CONST;

GimpProcedure * gimp_plug_in_procedure_new           (GimpPDBProcType            proc_type,
                                                      const gchar               *prog);

GimpPlugInProcedure * gimp_plug_in_procedure_find    (GSList                    *list,
                                                      const gchar               *proc_name);

const gchar * gimp_plug_in_procedure_get_progname    (const GimpPlugInProcedure *proc);

void          gimp_plug_in_procedure_set_locale_domain (GimpPlugInProcedure     *proc,
                                                        const gchar             *locale_domain);
const gchar * gimp_plug_in_procedure_get_locale_domain (const GimpPlugInProcedure *proc);

void          gimp_plug_in_procedure_set_help_domain (GimpPlugInProcedure       *proc,
                                                      const gchar               *help_domain);
const gchar * gimp_plug_in_procedure_get_help_domain (const GimpPlugInProcedure *proc);

gboolean      gimp_plug_in_procedure_add_menu_path   (GimpPlugInProcedure       *proc,
                                                      const gchar               *menu_path,
                                                      GError                   **error);

const gchar * gimp_plug_in_procedure_get_label       (GimpPlugInProcedure       *proc);
const gchar * gimp_plug_in_procedure_get_blurb       (const GimpPlugInProcedure *proc);

void          gimp_plug_in_procedure_set_icon        (GimpPlugInProcedure       *proc,
                                                      GimpIconType               type,
                                                      const guint8              *data,
                                                      gint                       data_length);
const gchar * gimp_plug_in_procedure_get_stock_id    (const GimpPlugInProcedure *proc);
GdkPixbuf   * gimp_plug_in_procedure_get_pixbuf      (const GimpPlugInProcedure *proc);

gchar       * gimp_plug_in_procedure_get_help_id     (const GimpPlugInProcedure *proc);

gboolean      gimp_plug_in_procedure_get_sensitive   (const GimpPlugInProcedure *proc,
                                                      GimpImageType              image_type);

void          gimp_plug_in_procedure_set_image_types (GimpPlugInProcedure       *proc,
                                                      const gchar               *image_types);
void          gimp_plug_in_procedure_set_file_proc   (GimpPlugInProcedure       *proc,
                                                      const gchar               *extensions,
                                                      const gchar               *prefixes,
                                                      const gchar               *magics);
void          gimp_plug_in_procedure_set_mime_type   (GimpPlugInProcedure       *proc,
                                                      const gchar               *mime_ype);
void          gimp_plug_in_procedure_set_thumb_loader(GimpPlugInProcedure       *proc,
                                                      const gchar               *thumbnailer);


#endif /* __GIMP_PLUG_IN_PROCEDURE_H__ */
