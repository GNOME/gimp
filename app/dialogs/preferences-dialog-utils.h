/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __PREFERENCES_DIALOG_UTILS_H__
#define __PREFERENCES_DIALOG_UTILS_H__


#define PREFS_COLOR_BUTTON_WIDTH  40
#define PREFS_COLOR_BUTTON_HEIGHT 24


GtkWidget * prefs_frame_new                   (const gchar    *label,
                                               GtkContainer   *parent,
                                               gboolean        expand);
GtkWidget * prefs_grid_new                    (GtkContainer   *parent);

GtkWidget * prefs_hint_box_new                (const gchar    *icon_name,
                                               const gchar    *text);

GtkWidget * prefs_button_add                  (const gchar    *icon_name,
                                               const gchar    *label,
                                               GtkBox         *box);
GtkWidget * prefs_switch_add                  (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               GtkBox         *vbox,
                                               GtkSizeGroup   *group,
                                               GtkWidget     **switch_out);
GtkWidget * prefs_check_button_add            (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               GtkBox         *box);
GtkWidget * prefs_check_button_add_with_icon  (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               const gchar    *icon_name,
                                               GtkBox         *box,
                                               GtkSizeGroup   *group);

GtkWidget * prefs_widget_add_aligned          (GtkWidget      *widget,
                                               const gchar    *text,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               gboolean        left_align,
                                               GtkSizeGroup   *group);

GtkWidget * prefs_color_button_add            (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               const gchar    *title,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group,
                                               GimpContext    *context);

GtkWidget * prefs_entry_add                   (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);
GtkWidget * prefs_spin_button_add             (GObject        *config,
                                               const gchar    *property_name,
                                               gdouble         step_increment,
                                               gdouble         page_increment,
                                               gint            digits,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);
GtkWidget * prefs_memsize_entry_add           (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);

GtkWidget * prefs_file_chooser_button_add     (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               const gchar    *dialog_title,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);

GtkWidget * prefs_enum_combo_box_add          (GObject        *config,
                                               const gchar    *property_name,
                                               gint            minimum,
                                               gint            maximum,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);
GtkWidget * prefs_boolean_combo_box_add       (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *true_text,
                                               const gchar    *false_text,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);
#ifdef HAVE_ISO_CODES
GtkWidget * prefs_language_combo_box_add      (GObject        *config,
                                               const gchar    *property_name,
                                               GtkBox         *vbox);
#endif
GtkWidget * prefs_profile_combo_box_add       (GObject        *config,
                                               const gchar    *property_name,
                                               GtkListStore   *profile_store,
                                               const gchar    *dialog_title,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group,
                                               GObject        *profile_path_config,
                                               const gchar    *profile_path_property_name);
GtkWidget * prefs_compression_combo_box_add   (GObject        *config,
                                               const gchar    *property_name,
                                               const gchar    *label,
                                               GtkGrid        *grid,
                                               gint            grid_top,
                                               GtkSizeGroup   *group);
 void       prefs_message                     (GtkWidget      *dialog,
                                               GtkMessageType  type,
                                               gboolean        destroy_with_parent,
                                               const gchar    *message);
/* Callbacks */
void        prefs_config_notify               (GObject        *config,
                                               GParamSpec     *param_spec,
                                               GObject        *config_copy);
void        prefs_config_copy_notify          (GObject        *config_copy,
                                               GParamSpec     *param_spec,
                                               GObject        *config);
void        prefs_gui_config_notify_font_size (GObject        *config,
                                               GParamSpec     *pspec,
                                               GtkRange       *range);
void        prefs_font_size_value_changed     (GtkRange       *range,
                                               GimpGuiConfig  *config);
void        prefs_gui_config_notify_icon_size (GObject        *config,
                                               GParamSpec     *pspec,
                                               GtkRange       *range);
void        prefs_icon_size_value_changed     (GtkRange       *range,
                                               GimpGuiConfig  *config);

#endif /* __PREFERENCES_DIALOG_H__ */
