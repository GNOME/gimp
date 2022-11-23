/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamessagedialog.h
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_MESSAGE_DIALOG_H__
#define __LIGMA_MESSAGE_DIALOG_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_MESSAGE_DIALOG            (ligma_message_dialog_get_type ())
#define LIGMA_MESSAGE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MESSAGE_DIALOG, LigmaMessageDialog))
#define LIGMA_MESSAGE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MESSAGE_DIALOG, LigmaMessageDialogClass))
#define LIGMA_IS_MESSAGE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MESSAGE_DIALOG))
#define LIGMA_IS_MESSAGE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MESSAGE_DIALOG))
#define LIGMA_MESSAGE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MESSAGE_DIALOG, LigmaMessageDialogClass))


typedef struct _LigmaMessageDialogClass  LigmaMessageDialogClass;

struct _LigmaMessageDialog
{
  LigmaDialog       parent_instance;

  LigmaMessageBox  *box;
};

struct _LigmaMessageDialogClass
{
  LigmaDialogClass  parent_class;
};


GType       ligma_message_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_message_dialog_new      (const gchar       *title,
                                          const gchar       *icon_name,
                                          GtkWidget         *parent,
                                          GtkDialogFlags     flags,
                                          LigmaHelpFunc       help_func,
                                          const gchar       *help_id,
                                          ...) G_GNUC_NULL_TERMINATED;


G_END_DECLS

#endif /* __LIGMA_MESSAGE_DIALOG_H__ */
