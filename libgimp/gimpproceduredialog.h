/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpproceduredialog.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_PROCEDURE_DIALOG_H__
#define __GIMP_PROCEDURE_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PROCEDURE_DIALOG            (gimp_procedure_dialog_get_type ())
#define GIMP_PROCEDURE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROCEDURE_DIALOG, GimpProcedureDialog))
#define GIMP_PROCEDURE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROCEDURE_DIALOG, GimpProcedureDialogClass))
#define GIMP_IS_PROCEDURE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROCEDURE_DIALOG))
#define GIMP_IS_PROCEDURE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROCEDURE_DIALOG))
#define GIMP_PROCEDURE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROCEDURE_DIALOG, GimpProcedureDialogClass))


typedef struct _GimpProcedureDialogClass   GimpProcedureDialogClass;
typedef struct _GimpProcedureDialogPrivate GimpProcedureDialogPrivate;

struct _GimpProcedureDialog
{
  GimpDialog                  parent_instance;

  GimpProcedureDialogPrivate *priv;
};

struct _GimpProcedureDialogClass
{
  GimpDialogClass  parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
};


GType       gimp_procedure_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_procedure_dialog_new      (GimpProcedure       *procedure,
                                            GimpProcedureConfig *config,
                                            const gchar         *title);

gboolean    gimp_procedure_dialog_run      (GimpProcedureDialog *dialog);


G_END_DECLS

#endif /* __GIMP_PROCEDURE_DIALOG_H__ */
