/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __DIALOGS_CONSTRUCTORS_H__
#define __DIALOGS_CONSTRUCTORS_H__


/*  toplevel dialogs  */

GtkWidget * dialogs_image_new_new          (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_file_open_new          (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_file_open_location_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_file_save_new          (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_preferences_get        (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_keyboard_shortcuts_get (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_module_get             (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_palette_import_get     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_tips_get               (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_about_get              (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_error_get              (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_close_all_get          (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_quit_get               (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);


/*  docks  */

GtkWidget * dialogs_toolbox_get            (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_dock_new               (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);


/*  dockables  */

GtkWidget * dialogs_dockable_constructor   (GimpDialogFactory      *factory,
                                            GimpDialogFactoryEntry *entry,
                                            GimpContext            *context,
                                            gint                    view_size);

GtkWidget * dialogs_tool_options_new       (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_device_status_new      (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_error_console_new      (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_cursor_view_new        (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);

GtkWidget * dialogs_image_list_view_new    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_brush_list_view_new    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_pattern_list_view_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_gradient_list_view_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_palette_list_view_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_font_list_view_new     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_tool_list_view_new     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_buffer_list_view_new   (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_document_list_view_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_template_list_view_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);

GtkWidget * dialogs_image_grid_view_new    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_brush_grid_view_new    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_pattern_grid_view_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_gradient_grid_view_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_palette_grid_view_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_font_grid_view_new     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_tool_grid_view_new     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_buffer_grid_view_new   (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_document_grid_view_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_template_grid_view_new (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);

GtkWidget * dialogs_layer_list_view_new    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_channel_list_view_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_vectors_list_view_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_path_list_view_new     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_colormap_editor_new    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_histogram_editor_new   (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_selection_editor_new   (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_undo_editor_new        (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_sample_point_editor_new(GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);

GtkWidget * dialogs_navigation_editor_new  (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);

GtkWidget * dialogs_color_editor_new       (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);

GtkWidget * dialogs_brush_editor_get       (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_gradient_editor_get    (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);
GtkWidget * dialogs_palette_editor_get     (GimpDialogFactory *factory,
                                            GimpContext       *context,
                                            gint               view_size);


#endif /* __DIALOGS_CONSTRUCTORS_H__ */
