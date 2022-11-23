/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmawidgets-utils.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __APP_LIGMA_WIDGETS_UTILS_H__
#define __APP_LIGMA_WIDGETS_UTILS_H__


GtkWidget       * ligma_menu_item_get_image         (GtkMenuItem          *item);
void              ligma_menu_item_set_image         (GtkMenuItem          *item,
                                                    GtkWidget            *image);

void              ligma_menu_position               (GtkMenu              *menu,
                                                    gint                 *x,
                                                    gint                 *y);
void              ligma_grid_attach_icon            (GtkGrid              *grid,
                                                    gint                  row,
                                                    const gchar          *icon_name,
                                                    GtkWidget            *widget,
                                                    gint                  columns);
void              ligma_enum_radio_box_add          (GtkBox               *box,
                                                    GtkWidget            *widget,
                                                    gint                  enum_value,
                                                    gboolean              below);
void              ligma_enum_radio_frame_add        (GtkFrame             *frame,
                                                    GtkWidget            *widget,
                                                    gint                  enum_value,
                                                    gboolean              below);
GdkPixbuf       * ligma_widget_load_icon            (GtkWidget            *widget,
                                                    const gchar          *icon_name,
                                                    gint                  size);
LigmaTabStyle      ligma_preview_tab_style_to_icon   (LigmaTabStyle          tab_style);

const gchar     * ligma_get_mod_string              (GdkModifierType       modifiers);
gchar           * ligma_suggest_modifiers           (const gchar          *message,
                                                    GdkModifierType       modifiers,
                                                    const gchar          *extend_selection_format,
                                                    const gchar          *toggle_behavior_format,
                                                    const gchar          *alt_format);
LigmaChannelOps    ligma_modifiers_to_channel_op     (GdkModifierType       modifiers);
GdkModifierType   ligma_replace_virtual_modifiers   (GdkModifierType       modifiers);
GdkModifierType   ligma_get_primary_accelerator_mask(void);
GdkModifierType   ligma_get_extend_selection_mask   (void);
GdkModifierType   ligma_get_modify_selection_mask   (void);
GdkModifierType   ligma_get_toggle_behavior_mask    (void);
GdkModifierType   ligma_get_constrain_behavior_mask (void);
GdkModifierType   ligma_get_all_modifiers_mask      (void);

void              ligma_get_monitor_resolution      (GdkMonitor           *monitor,
                                                    gdouble              *xres,
                                                    gdouble              *yres);
gboolean          ligma_get_style_color             (GtkWidget            *widget,
                                                    const gchar          *property_name,
                                                    GdkRGBA              *color);
void              ligma_window_set_hint             (GtkWindow            *window,
                                                    LigmaWindowHint        hint);
guint32           ligma_window_get_native_id        (GtkWindow            *window);
void              ligma_window_set_transient_for    (GtkWindow            *window,
                                                    guint32               parent_ID);
void              ligma_widget_set_accel_help       (GtkWidget            *widget,
                                                    LigmaAction           *action);

const gchar     * ligma_get_message_icon_name       (LigmaMessageSeverity   severity);
gboolean          ligma_get_color_tag_color         (LigmaColorTag          color_tag,
                                                    LigmaRGB              *color,
                                                    gboolean              inherited);

void              ligma_pango_layout_set_scale      (PangoLayout          *layout,
                                                    double                scale);
void              ligma_pango_layout_set_weight     (PangoLayout          *layout,
                                                    PangoWeight           weight);

void              ligma_highlight_widget            (GtkWidget            *widget,
                                                    gboolean              highlight,
                                                    GdkRectangle         *rect);
void              ligma_widget_blink_rect           (GtkWidget            *widget,
                                                    GdkRectangle         *rect);
void              ligma_widget_blink                (GtkWidget             *widget);
void              ligma_widget_blink_cancel         (GtkWidget             *widget);

void              ligma_blink_toolbox               (Ligma                  *ligma,
                                                    const gchar           *action_name,
                                                    GList               **blink_script);
void              ligma_blink_dockable              (Ligma                  *ligma,
                                                    const gchar           *dockable_identifier,
                                                    const gchar           *widget_identifier,
                                                    const gchar           *settings_value,
                                                    GList               **blink_script);

void              ligma_widget_script_blink         (GtkWidget            *widget,
                                                    const gchar          *widget_identifier,
                                                    GList               **blink_script);
void              ligma_blink_play_script           (GList                *blink_script);


GtkWidget       * ligma_dock_with_window_new        (LigmaDialogFactory    *factory,
                                                    GdkMonitor           *monitor,
                                                    gboolean              toolbox);
GtkWidget       * ligma_tools_get_tool_options_gui  (LigmaToolOptions      *tool_options);
void              ligma_tools_set_tool_options_gui  (LigmaToolOptions      *tool_options,
                                                    GtkWidget            *widget);
void              ligma_tools_set_tool_options_gui_func
                                                   (LigmaToolOptions      *tool_options,
                                                    LigmaToolOptionsGUIFunc func);

gboolean          ligma_widget_get_fully_opaque     (GtkWidget            *widget);
void              ligma_widget_set_fully_opaque     (GtkWidget            *widget,
                                                    gboolean              fully_opaque);

void              ligma_gtk_container_clear         (GtkContainer         *container);

void              ligma_button_set_suggested        (GtkWidget            *button,
                                                    gboolean              suggested,
                                                    GtkReliefStyle        default_relief);
void              ligma_button_set_destructive      (GtkWidget            *button,
                                                    gboolean              destructive,
                                                    GtkReliefStyle        default_relief);

void              ligma_gtk_adjustment_chain        (GtkAdjustment        *adjustment1,
                                                    GtkAdjustment        *adjustment2);

const gchar     * ligma_print_event                 (const GdkEvent       *event);

gboolean          ligma_color_profile_store_add_defaults
                                                   (LigmaColorProfileStore *store,
                                                    LigmaColorConfig       *config,
                                                    LigmaImageBaseType      base_type,
                                                    LigmaPrecision          precision,
                                                    GError               **error);

void              ligma_color_profile_chooser_dialog_connect_path
                                                   (GtkWidget             *dialog,
                                                    GObject               *config,
                                                    const gchar           *property_name);

void              ligma_widget_flush_expose         (void);


#endif /* __APP_LIGMA_WIDGETS_UTILS_H__ */
