/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __ACTIONS_H__
#define __ACTIONS_H__


extern LigmaActionFactory *global_action_factory;


void               actions_init            (Ligma                 *ligma);
void               actions_exit            (Ligma                 *ligma);

Ligma             * action_data_get_ligma    (gpointer              data);
LigmaContext      * action_data_get_context (gpointer              data);
LigmaImage        * action_data_get_image   (gpointer              data);
LigmaDisplay      * action_data_get_display (gpointer              data);
LigmaDisplayShell * action_data_get_shell   (gpointer              data);
GtkWidget        * action_data_get_widget  (gpointer              data);
gint               action_data_sel_count   (gpointer              data);

gdouble            action_select_value     (LigmaActionSelectType  select_type,
                                            gdouble               value,
                                            gdouble               min,
                                            gdouble               max,
                                            gdouble               def,
                                            gdouble               small_inc,
                                            gdouble               inc,
                                            gdouble               skip_inc,
                                            gdouble               delta_factor,
                                            gboolean              wrap);
void               action_select_property  (LigmaActionSelectType  select_type,
                                            LigmaDisplay          *display,
                                            GObject              *object,
                                            const gchar          *property_name,
                                            gdouble               small_inc,
                                            gdouble               inc,
                                            gdouble               skip_inc,
                                            gdouble               delta_factor,
                                            gboolean              wrap);
LigmaObject       * action_select_object    (LigmaActionSelectType  select_type,
                                            LigmaContainer        *container,
                                            LigmaObject           *current);
void               action_message          (LigmaDisplay          *display,
                                            GObject              *object,
                                            const gchar          *format,
                                            ...) G_GNUC_PRINTF(3,4);


#define return_if_no_ligma(ligma,data) \
  ligma = action_data_get_ligma (data); \
  if (! ligma) \
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
  drawables = ligma_image_get_selected_drawables (image); \
  if (! drawables) \
    return

#define return_if_no_layer(image,layer,data) \
  return_if_no_image (image,data); \
  layer = ligma_image_get_active_layer (image); \
  if (! layer) \
    return

#define return_if_no_layers(image,layers,data) \
  return_if_no_image (image,data); \
  layers = ligma_image_get_selected_layers (image); \
  if (! layers) \
    return

#define return_if_no_channel(image,channel,data) \
  return_if_no_image (image,data); \
  channel = ligma_image_get_active_channel (image); \
  if (! channel) \
    return

#define return_if_no_channels(image,channels,data) \
  return_if_no_image (image,data); \
  channels = ligma_image_get_selected_channels (image); \
  if (! channels) \
    return

#define return_if_no_vectors(image,vectors,data) \
  return_if_no_image (image,data); \
  vectors = ligma_image_get_active_vectors (image); \
  if (! vectors) \
    return

#define return_if_no_vectors_list(image,list,data) \
  return_if_no_image (image,data); \
  list = ligma_image_get_selected_vectors (image); \
  if (! list) \
    return


#endif /* __ACTIONS_H__ */
