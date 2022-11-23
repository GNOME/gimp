/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewabledialog.h
 * Copyright (C) 2000 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEWABLE_DIALOG_H__
#define __LIGMA_VIEWABLE_DIALOG_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_VIEWABLE_DIALOG            (ligma_viewable_dialog_get_type ())
#define LIGMA_VIEWABLE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEWABLE_DIALOG, LigmaViewableDialog))
#define LIGMA_VIEWABLE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEWABLE_DIALOG, LigmaViewableDialogClass))
#define LIGMA_IS_VIEWABLE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VIEWABLE_DIALOG))
#define LIGMA_IS_VIEWABLE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEWABLE_DIALOG))
#define LIGMA_VIEWABLE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEWABLE_DIALOG, LigmaViewableDialogClass))


typedef struct _LigmaViewableDialogClass  LigmaViewableDialogClass;

struct _LigmaViewableDialog
{
  LigmaDialog   parent_instance;

  LigmaContext *context;

  GList       *viewables;

  GtkWidget   *icon;
  GtkWidget   *view;
  GtkWidget   *desc_label;
  GtkWidget   *viewable_label;
};

struct _LigmaViewableDialogClass
{
  LigmaDialogClass  parent_class;
};


GType       ligma_viewable_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_viewable_dialog_new      (GList              *viewables,
                                           LigmaContext        *context,
                                           const gchar        *title,
                                           const gchar        *role,
                                           const gchar        *icon_name,
                                           const gchar        *desc,
                                           GtkWidget          *parent,
                                           LigmaHelpFunc        help_func,
                                           const gchar        *help_id,
                                           ...) G_GNUC_NULL_TERMINATED;

void    ligma_viewable_dialog_set_viewables (LigmaViewableDialog *dialog,
                                            GList              *viewables,
                                            LigmaContext        *context);


G_END_DECLS

#endif /* __LIGMA_VIEWABLE_DIALOG_H__ */
