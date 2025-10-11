/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


extern GimpActionFactory *global_action_factory;


void               actions_init            (Gimp                 *gimp);
void               actions_exit            (Gimp                 *gimp);

void               actions_filter_set_aux  (const gchar          *action_name);
gboolean           actions_filter_get_aux  (const gchar          *action_name);

Gimp             * action_data_get_gimp    (gpointer              data);
GimpContext      * action_data_get_context (gpointer              data);
GimpImage        * action_data_get_image   (gpointer              data);
GimpDisplay      * action_data_get_display (gpointer              data);
GimpDisplayShell * action_data_get_shell   (gpointer              data);
GtkWidget        * action_data_get_widget  (gpointer              data);
gint               action_data_sel_count   (gpointer              data);

gdouble            action_select_value     (GimpActionSelectType  select_type,
                                            gdouble               value,
                                            gdouble               min,
                                            gdouble               max,
                                            gdouble               def,
                                            gdouble               small_inc,
                                            gdouble               inc,
                                            gdouble               skip_inc,
                                            gdouble               delta_factor,
                                            gboolean              wrap);
void               action_select_property  (GimpActionSelectType  select_type,
                                            GimpDisplay          *display,
                                            GObject              *object,
                                            const gchar          *property_name,
                                            gdouble               small_inc,
                                            gdouble               inc,
                                            gdouble               skip_inc,
                                            gdouble               delta_factor,
                                            gboolean              wrap);
GimpObject       * action_select_object    (GimpActionSelectType  select_type,
                                            GimpContainer        *container,
                                            GimpObject           *current);
void               action_message          (GimpDisplay          *display,
                                            GObject              *object,
                                            const gchar          *format,
                                            ...) G_GNUC_PRINTF(3,4);


#define return_if_no_gimp(gimp,data) \
  gimp = action_data_get_gimp (data); \
  if (! gimp) \
    return

#define return_if_no_context(context,data) \
  context = action_data_get_context (data); \
  if (! context) \
    return

#define return_if_no_image(image,data) \
  image = action_data_get_image (data); \
  if (! image) \
    return

#define return_if_no_display(display,data) \
  display = action_data_get_display (data); \
  if (! display) \
    return

#define return_if_no_shell(shell,data) \
  shell = action_data_get_shell (data); \
  if (! shell) \
    return

#define return_if_no_widget(widget,data) \
  widget = action_data_get_widget (data); \
  if (! widget) \
    return

#define return_if_no_drawables(image,drawables,data) \
  return_if_no_image (image,data); \
  drawables = gimp_image_get_selected_drawables (image); \
  if (! drawables) \
    return

#define return_if_no_layers(image,layers,data) \
  return_if_no_image (image,data); \
  layers = gimp_image_get_selected_layers (image); \
  if (! layers) \
    return

#define return_if_no_channels(image,channels,data) \
  return_if_no_image (image,data); \
  channels = gimp_image_get_selected_channels (image); \
  if (! channels) \
    return

#define return_if_no_paths(image,paths,data) \
  return_if_no_image (image,data); \
  paths = gimp_image_get_selected_paths (image); \
  if (! paths) \
    return
