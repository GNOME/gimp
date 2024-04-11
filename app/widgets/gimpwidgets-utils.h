/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwidgets-utils.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __APP_GIMP_WIDGETS_UTILS_H__
#define __APP_GIMP_WIDGETS_UTILS_H__


GtkWidget       * gimp_menu_item_get_image         (GtkMenuItem          *item);
void              gimp_menu_item_set_image         (GtkMenuItem          *item,
                                                    GtkWidget            *image,
                                                    GimpAction           *action);

void              gimp_menu_position               (GtkMenu              *menu,
                                                    gint                 *x,
                                                    gint                 *y);
void              gimp_grid_attach_icon            (GtkGrid              *grid,
                                                    gint                  row,
                                                    const gchar          *icon_name,
                                                    GtkWidget            *widget,
                                                    gint                  columns);
void              gimp_enum_radio_box_add          (GtkBox               *box,
                                                    GtkWidget            *widget,
                                                    gint                  enum_value,
                                                    gboolean              below);
void              gimp_enum_radio_frame_add        (GtkFrame             *frame,
                                                    GtkWidget            *widget,
                                                    gint                  enum_value,
                                                    gboolean              below);
GdkPixbuf       * gimp_widget_load_icon            (GtkWidget            *widget,
                                                    const gchar          *icon_name,
                                                    gint                  size);
GimpTabStyle      gimp_preview_tab_style_to_icon   (GimpTabStyle          tab_style);

const gchar     * gimp_get_mod_string              (GdkModifierType       modifiers);
gchar           * gimp_suggest_modifiers           (const gchar          *message,
                                                    GdkModifierType       modifiers,
                                                    const gchar          *extend_selection_format,
                                                    const gchar          *toggle_behavior_format,
                                                    const gchar          *alt_format);
GimpChannelOps    gimp_modifiers_to_channel_op     (GdkModifierType       modifiers);
GdkModifierType   gimp_replace_virtual_modifiers   (GdkModifierType       modifiers);
GdkModifierType   gimp_get_primary_accelerator_mask(void);
GdkModifierType   gimp_get_extend_selection_mask   (void);
GdkModifierType   gimp_get_modify_selection_mask   (void);
GdkModifierType   gimp_get_toggle_behavior_mask    (void);
GdkModifierType   gimp_get_constrain_behavior_mask (void);
GdkModifierType   gimp_get_all_modifiers_mask      (void);

void              gimp_get_monitor_resolution      (GdkMonitor           *monitor,
                                                    gdouble              *xres,
                                                    gdouble              *yres);
gboolean          gimp_get_style_color             (GtkWidget            *widget,
                                                    const gchar          *property_name,
                                                    GdkRGBA              *color);
void              gimp_window_set_hint             (GtkWindow            *window,
                                                    GimpWindowHint        hint);
void              gimp_window_set_transient_for    (GtkWindow            *window,
                                                    GimpProgress         *progress);
void          gimp_window_set_transient_for_handle (GtkWindow            *window,
                                                    GBytes               *handle);
void              gimp_widget_set_accel_help       (GtkWidget            *widget,
                                                    GimpAction           *action);

const gchar     * gimp_get_message_icon_name       (GimpMessageSeverity   severity);
gboolean          gimp_get_color_tag_color         (GimpColorTag          color_tag,
                                                    GeglColor            *color,
                                                    gboolean              inherited);

void              gimp_pango_layout_set_scale      (PangoLayout          *layout,
                                                    double                scale);
void              gimp_pango_layout_set_weight     (PangoLayout          *layout,
                                                    PangoWeight           weight);

void              gimp_highlight_widget            (GtkWidget            *widget,
                                                    gboolean              highlight,
                                                    GdkRectangle         *rect);
void              gimp_widget_blink_rect           (GtkWidget            *widget,
                                                    GdkRectangle         *rect);
void              gimp_widget_blink                (GtkWidget             *widget);
void              gimp_widget_blink_cancel         (GtkWidget             *widget);

void              gimp_blink_toolbox               (Gimp                  *gimp,
                                                    const gchar           *action_name,
                                                    GList               **blink_script);
void              gimp_blink_dockable              (Gimp                  *gimp,
                                                    const gchar           *dockable_identifier,
                                                    const gchar           *widget_identifier,
                                                    const gchar           *settings_value,
                                                    GList               **blink_script);

void              gimp_widget_script_blink         (GtkWidget            *widget,
                                                    const gchar          *widget_identifier,
                                                    GList               **blink_script);
void              gimp_blink_play_script           (GList                *blink_script);


GtkWidget       * gimp_dock_with_window_new        (GimpDialogFactory    *factory,
                                                    GdkMonitor           *monitor,
                                                    gboolean              toolbox);
GtkWidget       * gimp_tools_get_tool_options_gui  (GimpToolOptions      *tool_options);
void              gimp_tools_set_tool_options_gui  (GimpToolOptions      *tool_options,
                                                    GtkWidget            *widget);
void              gimp_tools_set_tool_options_gui_func
                                                   (GimpToolOptions      *tool_options,
                                                    GimpToolOptionsGUIFunc func);

gboolean          gimp_widget_get_fully_opaque     (GtkWidget            *widget);
void              gimp_widget_set_fully_opaque     (GtkWidget            *widget,
                                                    gboolean              fully_opaque);

void              gimp_gtk_container_clear         (GtkContainer         *container);

void              gimp_button_set_suggested        (GtkWidget            *button,
                                                    gboolean              suggested,
                                                    GtkReliefStyle        default_relief);
void              gimp_button_set_destructive      (GtkWidget            *button,
                                                    gboolean              destructive,
                                                    GtkReliefStyle        default_relief);

void              gimp_gtk_adjustment_chain        (GtkAdjustment        *adjustment1,
                                                    GtkAdjustment        *adjustment2);

const gchar     * gimp_print_event                 (const GdkEvent       *event);

gboolean          gimp_color_profile_store_add_defaults
                                                   (GimpColorProfileStore *store,
                                                    GimpColorConfig       *config,
                                                    GimpImageBaseType      base_type,
                                                    GimpPrecision          precision,
                                                    GError               **error);

void              gimp_color_profile_chooser_dialog_connect_path
                                                   (GtkWidget             *dialog,
                                                    GObject               *config,
                                                    const gchar           *property_name);

void              gimp_widget_flush_expose         (void);

void              gimp_make_valid_action_name      (gchar                 *action_name);

gchar           * gimp_utils_make_canonical_menu_label (const gchar       *path);
gchar          ** gimp_utils_break_menu_path           (const gchar       *path,
                                                        gchar            **mnemonic_path1,
                                                        gchar            **section_name);
gboolean          gimp_utils_are_menu_path_identical   (const gchar       *path1,
                                                        const gchar       *path2,
                                                        gchar            **canonical_path1,
                                                        gchar            **mnemonic_path1,
                                                        gchar            **path1_section_name);
#ifdef G_OS_WIN32
void              gimp_window_set_title_bar_theme      (Gimp              *gimp,
                                                        GtkWidget         *dialog);
#endif

#endif /* __APP_GIMP_WIDGETS_UTILS_H__ */
