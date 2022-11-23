/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamessagebox.h
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

#ifndef __LIGMA_MESSAGE_BOX_H__
#define __LIGMA_MESSAGE_BOX_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_MESSAGE_BOX            (ligma_message_box_get_type ())
#define LIGMA_MESSAGE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MESSAGE_BOX, LigmaMessageBox))
#define LIGMA_MESSAGE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MESSAGE_BOX, LigmaMessageBoxClass))
#define LIGMA_IS_MESSAGE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MESSAGE_BOX))
#define LIGMA_IS_MESSAGE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MESSAGE_BOX))
#define LIGMA_MESSAGE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MESSAGE_BOX, LigmaMessageBoxClass))


typedef struct _LigmaMessageBoxClass  LigmaMessageBoxClass;

struct _LigmaMessageBox
{
  GtkBox     parent_instance;

  gchar     *icon_name;
  gint       repeat;
  GtkWidget *label[3];
  GtkWidget *image;

  guint      idle_id;
};

struct _LigmaMessageBoxClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_message_box_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_message_box_new              (const gchar    *icon_name);
void        ligma_message_box_set_primary_text (LigmaMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void        ligma_message_box_set_text         (LigmaMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void        ligma_message_box_set_markup       (LigmaMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
gint        ligma_message_box_repeat           (LigmaMessageBox *box);


G_END_DECLS

#endif /* __LIGMA_MESSAGE_BOX_H__ */
