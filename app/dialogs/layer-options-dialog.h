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

#ifndef __LAYER_OPTIONS_DIALOG_H__
#define __LAYER_OPTIONS_DIALOG_H__


typedef void (* GimpLayerOptionsCallback) (GtkWidget              *dialog,
                                           GimpImage              *image,
                                           GimpLayer              *layer,
                                           GimpContext            *context,
                                           const gchar            *layer_name,
                                           GimpLayerMode           layer_mode,
                                           GimpLayerColorSpace     layer_blend_space,
                                           GimpLayerColorSpace     layer_composite_space,
                                           GimpLayerCompositeMode  layer_composite_mode,
                                           gdouble                 layer_opacity,
                                           GimpFillType            layer_fill_type,
                                           gint                    layer_width,
                                           gint                    layer_height,
                                           gint                    layer_offset_x,
                                           gint                    layer_offset_y,
                                           gboolean                layer_visible,
                                           GimpColorTag            layer_color_tag,
                                           gboolean                layer_lock_content,
                                           gboolean                layer_lock_position,
                                           gboolean                layer_lock_visibility,
                                           gboolean                layer_lock_alpha,
                                           gboolean                rename_text_layer,
                                           gpointer                user_data);


GtkWidget * layer_options_dialog_new (GimpImage                *image,
                                      GimpLayer                *layer,
                                      GimpContext              *context,
                                      GtkWidget                *parent,
                                      const gchar              *title,
                                      const gchar              *role,
                                      const gchar              *icon_name,
                                      const gchar              *desc,
                                      const gchar              *help_id,
                                      const gchar              *layer_name,
                                      GimpLayerMode             layer_mode,
                                      GimpLayerColorSpace       layer_blend_space,
                                      GimpLayerColorSpace       layer_composite_space,
                                      GimpLayerCompositeMode    layer_composite_mode,
                                      gdouble                   layer_opacity,
                                      GimpFillType              layer_fill_type,
                                      gboolean                  layer_visible,
                                      GimpColorTag              layer_color_tag,
                                      gboolean                  layer_lock_content,
                                      gboolean                  layer_lock_position,
                                      gboolean                  layer_lock_visibility,
                                      gboolean                  layer_lock_alpha,
                                      GimpLayerOptionsCallback  callback,
                                      gpointer                  user_data);


#endif /* __LAYER_OPTIONS_DIALOG_H__ */
