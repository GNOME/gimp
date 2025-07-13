/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_EDITOR (gimp_editor_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpEditor,
                          gimp_editor,
                          GIMP, EDITOR,
                          GtkBox)


struct _GimpEditorClass
{
  GtkBoxClass  parent_class;
};


GType       gimp_editor_get_type          (void) G_GNUC_CONST;

GtkWidget * gimp_editor_new               (void);

void        gimp_editor_create_menu       (GimpEditor           *editor,
                                           GimpMenuFactory      *menu_factory,
                                           const gchar          *menu_identifier,
                                           const gchar          *ui_path,
                                           gpointer              popup_data);
gboolean    gimp_editor_popup_menu        (GimpEditor           *editor,
                                           GimpMenuPositionFunc  position_func,
                                           gpointer              position_data);
gboolean    gimp_editor_popup_menu_at_pointer
                                          (GimpEditor           *editor,
                                           const GdkEvent       *trigger_event);
gboolean   gimp_editor_popup_menu_at_rect (GimpEditor           *editor,
                                           GdkWindow            *window,
                                           const GdkRectangle   *rect,
                                           GdkGravity            rect_anchor,
                                           GdkGravity            menu_anchor,
                                           const GdkEvent       *trigger_event);

GtkWidget * gimp_editor_add_button        (GimpEditor           *editor,
                                           const gchar          *icon_name,
                                           const gchar          *tooltip,
                                           const gchar          *help_id,
                                           GCallback             callback,
                                           GCallback             extended_callback,
                                           GObject              *callback_data);
GtkWidget * gimp_editor_add_icon_box      (GimpEditor           *editor,
                                           GType                 enum_type,
                                           const gchar          *icon_prefix,
                                           GCallback             callback,
                                           gpointer              callback_data);

GtkWidget * gimp_editor_get_top_box       (GimpEditor           *editor);

GtkWidget * gimp_editor_add_action_button (GimpEditor           *editor,
                                           const gchar          *group_name,
                                           const gchar          *action_name,
                                           ...) G_GNUC_NULL_TERMINATED;
void        gimp_editor_set_action_sensitive
                                          (GimpEditor          *editor,
                                           const gchar         *group_name,
                                           const gchar         *action_name,
                                           gboolean             sensitive,
                                           const gchar         *reason);

void        gimp_editor_set_show_name     (GimpEditor          *editor,
                                           gboolean             show);
void        gimp_editor_set_name          (GimpEditor          *editor,
                                           const gchar         *name);

void        gimp_editor_set_box_style     (GimpEditor          *editor,
                                           GtkBox              *box);
GimpUIManager *
            gimp_editor_get_ui_manager    (GimpEditor          *editor);
GtkBox    * gimp_editor_get_button_box    (GimpEditor          *editor);
GimpMenuFactory *
            gimp_editor_get_menu_factory  (GimpEditor          *editor);
gpointer  * gimp_editor_get_popup_data    (GimpEditor          *editor);
gchar     * gimp_editor_get_ui_path       (GimpEditor          *editor);
