/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaproceduredialog.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_PROCEDURE_DIALOG_H__
#define __LIGMA_PROCEDURE_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PROCEDURE_DIALOG            (ligma_procedure_dialog_get_type ())
#define LIGMA_PROCEDURE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROCEDURE_DIALOG, LigmaProcedureDialog))
#define LIGMA_PROCEDURE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROCEDURE_DIALOG, LigmaProcedureDialogClass))
#define LIGMA_IS_PROCEDURE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROCEDURE_DIALOG))
#define LIGMA_IS_PROCEDURE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROCEDURE_DIALOG))
#define LIGMA_PROCEDURE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROCEDURE_DIALOG, LigmaProcedureDialogClass))


typedef struct _LigmaProcedureDialogClass   LigmaProcedureDialogClass;
typedef struct _LigmaProcedureDialogPrivate LigmaProcedureDialogPrivate;

struct _LigmaProcedureDialog
{
  LigmaDialog                  parent_instance;

  LigmaProcedureDialogPrivate *priv;
};

struct _LigmaProcedureDialogClass
{
  LigmaDialogClass  parent_class;

  void             (* fill_list) (LigmaProcedureDialog *dialog,
                                  LigmaProcedure       *procedure,
                                  LigmaProcedureConfig *config,
                                  GList               *properties);

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType       ligma_procedure_dialog_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_procedure_dialog_new               (LigmaProcedure       *procedure,
                                                     LigmaProcedureConfig *config,
                                                     const gchar         *title);

GtkWidget * ligma_procedure_dialog_get_widget        (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     GType                widget_type);
GtkWidget * ligma_procedure_dialog_get_color_widget  (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gboolean             editable,
                                                     LigmaColorAreaType    type);
GtkWidget * ligma_procedure_dialog_get_int_combo     (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     LigmaIntStore        *store);
GtkWidget * ligma_procedure_dialog_get_int_radio     (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     LigmaIntStore        *store);
GtkWidget * ligma_procedure_dialog_get_spin_scale    (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gdouble              factor);
GtkWidget * ligma_procedure_dialog_get_scale_entry   (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gdouble              factor);
GtkWidget * ligma_procedure_dialog_get_label         (LigmaProcedureDialog *dialog,
                                                     const gchar         *label_id,
                                                     const gchar         *text);
GtkWidget * ligma_procedure_dialog_get_file_chooser  (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     GtkFileChooserAction action);

GtkWidget * ligma_procedure_dialog_fill_box          (LigmaProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *first_property,
                                                     ...) G_GNUC_NULL_TERMINATED;
GtkWidget * ligma_procedure_dialog_fill_box_list     (LigmaProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     GList               *properties);
GtkWidget * ligma_procedure_dialog_fill_flowbox      (LigmaProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *first_property,
                                                     ...) G_GNUC_NULL_TERMINATED;
GtkWidget * ligma_procedure_dialog_fill_flowbox_list (LigmaProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     GList               *properties);

GtkWidget * ligma_procedure_dialog_fill_frame        (LigmaProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *title_id,
                                                     gboolean             invert_title,
                                                     const gchar         *contents_id);

GtkWidget * ligma_procedure_dialog_fill_expander     (LigmaProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *title_id,
                                                     gboolean             invert_title,
                                                     const gchar         *contents_id);

void        ligma_procedure_dialog_fill              (LigmaProcedureDialog *dialog,
                                                     ...) G_GNUC_NULL_TERMINATED;
void        ligma_procedure_dialog_fill_list         (LigmaProcedureDialog *dialog,
                                                     GList               *properties);

void        ligma_procedure_dialog_set_sensitive     (LigmaProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gboolean             sensitive,
                                                     GObject             *config,
                                                     const gchar         *config_property,
                                                     gboolean             config_invert);

gboolean    ligma_procedure_dialog_run               (LigmaProcedureDialog *dialog);


G_END_DECLS

#endif /* __LIGMA_PROCEDURE_DIALOG_H__ */
