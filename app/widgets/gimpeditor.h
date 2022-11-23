/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_EDITOR_H__
#define __LIGMA_EDITOR_H__


#define LIGMA_TYPE_EDITOR            (ligma_editor_get_type ())
#define LIGMA_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EDITOR, LigmaEditor))
#define LIGMA_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EDITOR, LigmaEditorClass))
#define LIGMA_IS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EDITOR))
#define LIGMA_IS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EDITOR))
#define LIGMA_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_EDITOR, LigmaEditorClass))


typedef struct _LigmaEditorClass    LigmaEditorClass;
typedef struct _LigmaEditorPrivate  LigmaEditorPrivate;

struct _LigmaEditor
{
  GtkBox            parent_instance;

  LigmaEditorPrivate *priv;
};

struct _LigmaEditorClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_editor_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_editor_new               (void);

void        ligma_editor_create_menu       (LigmaEditor           *editor,
                                           LigmaMenuFactory      *menu_factory,
                                           const gchar          *menu_identifier,
                                           const gchar          *ui_path,
                                           gpointer              popup_data);
gboolean    ligma_editor_popup_menu        (LigmaEditor           *editor,
                                           LigmaMenuPositionFunc  position_func,
                                           gpointer              position_data);
gboolean    ligma_editor_popup_menu_at_pointer
                                          (LigmaEditor           *editor,
                                           const GdkEvent       *trigger_event);
gboolean   ligma_editor_popup_menu_at_rect (LigmaEditor           *editor,
                                           GdkWindow            *window,
                                           const GdkRectangle   *rect,
                                           GdkGravity            rect_anchor,
                                           GdkGravity            menu_anchor,
                                           const GdkEvent       *trigger_event);

GtkWidget * ligma_editor_add_button        (LigmaEditor           *editor,
                                           const gchar          *icon_name,
                                           const gchar          *tooltip,
                                           const gchar          *help_id,
                                           GCallback             callback,
                                           GCallback             extended_callback,
                                           gpointer              callback_data);
GtkWidget * ligma_editor_add_icon_box      (LigmaEditor           *editor,
                                           GType                 enum_type,
                                           const gchar          *icon_prefix,
                                           GCallback             callback,
                                           gpointer              callback_data);

GtkWidget * ligma_editor_add_action_button (LigmaEditor           *editor,
                                           const gchar          *group_name,
                                           const gchar          *action_name,
                                           ...) G_GNUC_NULL_TERMINATED;

void        ligma_editor_set_show_name       (LigmaEditor         *editor,
                                             gboolean            show);
void        ligma_editor_set_name            (LigmaEditor         *editor,
                                             const gchar        *name);

void        ligma_editor_set_box_style       (LigmaEditor         *editor,
                                             GtkBox             *box);
LigmaUIManager *
            ligma_editor_get_ui_manager      (LigmaEditor         *editor);
GtkBox    * ligma_editor_get_button_box      (LigmaEditor         *editor);
LigmaMenuFactory *
            ligma_editor_get_menu_factory    (LigmaEditor         *editor);
gpointer *  ligma_editor_get_popup_data      (LigmaEditor         *editor);
gchar *     ligma_editor_get_ui_path         (LigmaEditor         *editor);

#endif  /*  __LIGMA_EDITOR_H__  */
