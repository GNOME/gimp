/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmessagebox.h
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define GIMP_TYPE_MESSAGE_BOX            (gimp_message_box_get_type ())
#define GIMP_MESSAGE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MESSAGE_BOX, GimpMessageBox))
#define GIMP_MESSAGE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MESSAGE_BOX, GimpMessageBoxClass))
#define GIMP_IS_MESSAGE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MESSAGE_BOX))
#define GIMP_IS_MESSAGE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MESSAGE_BOX))
#define GIMP_MESSAGE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MESSAGE_BOX, GimpMessageBoxClass))


typedef struct _GimpMessageBoxClass  GimpMessageBoxClass;

struct _GimpMessageBox
{
  GtkBox     parent_instance;

  gchar     *icon_name;
  gint       repeat;
  GtkWidget *label[3];
  GtkWidget *image;

  guint      idle_id;
};

struct _GimpMessageBoxClass
{
  GtkBoxClass  parent_class;
};


GType       gimp_message_box_get_type         (void) G_GNUC_CONST;

GtkWidget * gimp_message_box_new              (const gchar    *icon_name);
void        gimp_message_box_set_primary_text (GimpMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void        gimp_message_box_set_text         (GimpMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void        gimp_message_box_set_markup       (GimpMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
gint        gimp_message_box_repeat           (GimpMessageBox *box);
