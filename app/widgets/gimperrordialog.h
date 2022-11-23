/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaerrordialog.c
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_ERROR_DIALOG_H__
#define __LIGMA_ERROR_DIALOG_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_ERROR_DIALOG            (ligma_error_dialog_get_type ())
#define LIGMA_ERROR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ERROR_DIALOG, LigmaErrorDialog))
#define LIGMA_ERROR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ERROR_DIALOG, LigmaErrorDialogClass))
#define LIGMA_IS_ERROR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ERROR_DIALOG))
#define LIGMA_IS_ERROR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ERROR_DIALOG))
#define LIGMA_ERROR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ERROR_DIALOG, LigmaErrorDialogClass))


typedef struct _LigmaErrorDialogClass  LigmaErrorDialogClass;

struct _LigmaErrorDialog
{
  LigmaDialog       parent_instance;

  GtkWidget       *vbox;

  GList           *messages;
  gboolean         overflow;
};

struct _LigmaErrorDialogClass
{
  LigmaDialogClass  parent_class;
};


GType       ligma_error_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_error_dialog_new      (const gchar     *title);
void        ligma_error_dialog_add      (LigmaErrorDialog *dialog,
                                        const gchar     *icon_name,
                                        const gchar     *domain,
                                        const gchar     *message);



G_END_DECLS

#endif /* __LIGMA_ERROR_DIALOG_H__ */
