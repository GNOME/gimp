/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasaveproceduredialog.h
 * Copyright (C) 2020 Jehan
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

#ifndef __LIGMA_SAVE_PROCEDURE_DIALOG_H__
#define __LIGMA_SAVE_PROCEDURE_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_SAVE_PROCEDURE_DIALOG            (ligma_save_procedure_dialog_get_type ())
#define LIGMA_SAVE_PROCEDURE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SAVE_PROCEDURE_DIALOG, LigmaSaveProcedureDialog))
#define LIGMA_SAVE_PROCEDURE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SAVE_PROCEDURE_DIALOG, LigmaSaveProcedureDialogClass))
#define LIGMA_IS_SAVE_PROCEDURE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SAVE_PROCEDURE_DIALOG))
#define LIGMA_IS_SAVE_PROCEDURE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SAVE_PROCEDURE_DIALOG))
#define LIGMA_SAVE_PROCEDURE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SAVE_PROCEDURE_DIALOG, LigmaSaveProcedureDialogClass))


typedef struct _LigmaSaveProcedureDialogClass   LigmaSaveProcedureDialogClass;
typedef struct _LigmaSaveProcedureDialogPrivate LigmaSaveProcedureDialogPrivate;

struct _LigmaSaveProcedureDialog
{
  LigmaProcedureDialog             parent_instance;

  LigmaSaveProcedureDialogPrivate *priv;
};

struct _LigmaSaveProcedureDialogClass
{
  LigmaProcedureDialogClass  parent_class;

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


GType       ligma_save_procedure_dialog_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_save_procedure_dialog_new               (LigmaSaveProcedure   *procedure,
                                                          LigmaProcedureConfig *config,
                                                          LigmaImage           *image);

void        ligma_save_procedure_dialog_add_metadata      (LigmaSaveProcedureDialog *dialog,
                                                          const gchar             *property);


G_END_DECLS

#endif /* __LIGMA_SAVE_PROCEDURE_DIALOG_H__ */
