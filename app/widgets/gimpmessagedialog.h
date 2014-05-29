/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmessagedialog.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_MESSAGE_DIALOG_H__
#define __GIMP_MESSAGE_DIALOG_H__

G_BEGIN_DECLS


#define GIMP_TYPE_MESSAGE_DIALOG            (gimp_message_dialog_get_type ())
#define GIMP_MESSAGE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MESSAGE_DIALOG, GimpMessageDialog))
#define GIMP_MESSAGE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MESSAGE_DIALOG, GimpMessageDialogClass))
#define GIMP_IS_MESSAGE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MESSAGE_DIALOG))
#define GIMP_IS_MESSAGE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MESSAGE_DIALOG))
#define GIMP_MESSAGE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MESSAGE_DIALOG, GimpMessageDialogClass))


typedef struct _GimpMessageDialogClass  GimpMessageDialogClass;

struct _GimpMessageDialog
{
  GimpDialog       parent_instance;

  GimpMessageBox  *box;
};

struct _GimpMessageDialogClass
{
  GimpDialogClass  parent_class;
};


GType       gimp_message_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_message_dialog_new      (const gchar       *title,
                                          const gchar       *icon_name,
                                          GtkWidget         *parent,
                                          GtkDialogFlags     flags,
                                          GimpHelpFunc       help_func,
                                          const gchar       *help_id,
                                          ...) G_GNUC_NULL_TERMINATED;


G_END_DECLS

#endif /* __GIMP_MESSAGE_DIALOG_H__ */
