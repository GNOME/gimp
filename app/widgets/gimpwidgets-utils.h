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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __APP_GIMP_WIDGETS_UTILS_H__
#define __APP_GIMP_WIDGETS_UTILS_H__


GtkWidget       * gimp_menu_item_get_image         (GtkMenuItem          *item);
void              gimp_menu_item_set_image         (GtkMenuItem          *item,
                                                    GtkWidget            *image);

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
GtkIconSize       gimp_get_icon_size               (GtkWidget            *widget,
                                                    const gchar          *icon_name,
                                                    GtkIconSize           max_size,
                                                    gint                  width,
                                                    gint                  height);
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
guint32           gimp_window_get_native_id        (GtkWindow            *window);
void              gimp_window_set_transient_for    (GtkWindow            *window,
                                                    guint32               parent_ID);
void              gimp_widget_set_accel_help       (GtkWidget            *widget,
                                                    GtkAction            *action);

const gchar     * gimp_get_message_icon_name       (GimpMessageSeverity   severity);
gboolean          gimp_get_color_tag_color         (GimpColorTag          color_tag,
                                                    GimpRGB              *color,
                                                    gboolean              inherited);

void              gimp_pango_layout_set_scale      (PangoLayout          *layout,
                                                    double                scale);
void              gimp_pango_layout_set_weight     (PangoLayout          *layout,
                                                    PangoWeight           weight);
void              gimp_highlight_widget            (GtkWidget            *widget,
                                                    gboolean              highlight);
void              gimp_widget_blink                (GtkWidget             *widget);
void              gimp_widget_blink_cancel         (GtkWidget             *widget);
GtkWidget       * gimp_dock_with_window_new        (GimpDialogFactory    *factory,
                                                    GdkMonitor           *monitor,
                                                    gboolean              toolbox);
GtkWidget       * gimp_tools_get_tool_options_gui  (GimpToolOptions      *tool_options);
void              gimp_tools_set_tool_options_gui  (GimpToolOptions      *tool_options,
                                                    GtkWidget            *widget);

void              gimp_widget_flush_expose         (GtkWidget            *widget);

gboolean          gimp_widget_get_fully_opaque     (GtkWidget            *widget);
void              gimp_widget_set_fully_opaque     (GtkWidget            *widget,
                                                    gboolean              fully_opaque);

void              gimp_button_set_suggested        (GtkWidget            *button,
                                                    gboolean              suggested,
                                                    GtkReliefStyle        default_relief);
void              gimp_button_set_destructive      (GtkWidget            *button,
                                                    gboolean              suggestes,
                                                    GtkReliefStyle        default_relief);

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


#endif /* __APP_GIMP_WIDGETS_UTILS_H__ */
