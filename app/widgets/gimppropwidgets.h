/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropwidgets.h
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


/*  GParamBoolean  */

GtkWidget * gimp_prop_expanding_frame_new   (GObject       *config,
                                             const gchar   *property_name,
                                             const gchar   *button_label,
                                             GtkWidget     *child,
                                             GtkWidget    **button);

GtkWidget * gimp_prop_boolean_icon_box_new  (GObject      *config,
                                             const gchar  *property_name,
                                             const gchar  *true_icon,
                                             const gchar  *false_icon,
                                             const gchar  *true_tooltip,
                                             const gchar  *false_tooltip);


/*  GParamEnum  */

GtkWidget * gimp_prop_layer_mode_box_new    (GObject       *config,
                                             const gchar   *property_name,
                                             GimpLayerModeContext  context);


/*  GeglParamColor  */

GtkWidget * gimp_prop_color_button_new      (GObject       *config,
                                             const gchar   *property_name,
                                             const gchar   *title,
                                             gint           width,
                                             gint           height,
                                             GimpColorAreaType  type);


/*  GParamDouble  */

GtkWidget * gimp_prop_angle_dial_new        (GObject       *config,
                                             const gchar   *property_name);
GtkWidget * gimp_prop_angle_range_dial_new  (GObject       *config,
                                             const gchar   *alpha_property_name,
                                             const gchar   *beta_property_name,
                                             const gchar   *clockwise_property_name);

GtkWidget * gimp_prop_polar_new             (GObject       *config,
                                             const gchar   *angle_property_name,
                                             const gchar   *radius_property_name);

GtkWidget * gimp_prop_range_new             (GObject       *config,
                                             const gchar   *lower_property_name,
                                             const gchar   *upper_property_name,
                                             gdouble        step_increment,
                                             gdouble        page_increment,
                                             gint           digits,
                                             gboolean       sorted);
void        gimp_prop_range_set_ui_limits   (GtkWidget     *widget,
                                             gdouble        lower,
                                             gdouble        upper);


/*  GParamObject (GimpViewable)  */

GtkWidget * gimp_prop_view_new              (GObject       *config,
                                             const gchar   *property_name,
                                             GimpContext   *context,
                                             gint           size);


/*  GParamDouble, GParamDouble, GParamDouble, GParamDouble, GParamBoolean  */

GtkWidget * gimp_prop_number_pair_entry_new (GObject     *config,
                                             const gchar *left_number_property,
                                             const gchar *right_number_property,
                                             const gchar *default_left_number_property,
                                             const gchar *default_right_number_property,
                                             const gchar *user_override_property,
                                             gboolean     connect_numbers_changed,
                                             gboolean     connect_ratio_changed,
                                             const gchar *separators,
                                             gboolean     allow_simplification,
                                             gdouble      min_valid_value,
                                             gdouble      max_valid_value);


/*  GParamString  */

GtkWidget * gimp_prop_language_combo_box_new    (GObject      *config,
                                                 const gchar  *property_name);
GtkWidget * gimp_prop_language_entry_new        (GObject      *config,
                                                 const gchar  *property_name);

GtkWidget * gimp_prop_profile_combo_box_new     (GObject      *config,
                                                 const gchar  *property_name,
                                                 GtkListStore *profile_store,
                                                 const gchar  *dialog_title,
                                                 GObject      *profile_path_config,
                                                 const gchar  *profile_path_property_name);

GtkWidget * gimp_prop_compression_combo_box_new (GObject     *config,
                                                 const gchar *property_name);

GtkWidget * gimp_prop_icon_picker_new           (GimpViewable *viewable,
                                                 Gimp         *gimp);


/*  Utility functions  */

gboolean _gimp_prop_widgets_get_numeric_values (GObject     *object,
                                                GParamSpec  *param_spec,
                                                gdouble     *value,
                                                gdouble     *lower,
                                                gdouble     *upper,
                                                const gchar *strloc);
