/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgetsutils.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_WIDGETS_UTILS_H__
#define __GIMP_WIDGETS_UTILS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean             gimp_event_triggers_context_menu (const GdkEvent   *event,
                                                       gboolean          on_release);

GtkWidget          * gimp_grid_attach_aligned        (GtkGrid           *grid,
                                                      gint               left,
                                                      gint               top,
                                                      const gchar       *label_text,
                                                      gfloat             label_xalign,
                                                      gfloat             label_yalign,
                                                      GtkWidget         *widget,
                                                      gint               widget_columns);

void                 gimp_label_set_attributes       (GtkLabel          *label,
                                                      ...);

GdkMonitor         * gimp_widget_get_monitor         (GtkWidget         *widget);
GdkMonitor         * gimp_get_monitor_at_pointer     (void);

void                 gimp_widget_track_monitor       (GtkWidget         *widget,
                                                      GCallback          monitor_changed_callback,
                                                      gpointer           user_data,
                                                      GDestroyNotify     user_data_destroy);

GimpColorProfile   * gimp_monitor_get_color_profile  (GdkMonitor        *monitor);
GimpColorProfile   * gimp_widget_get_color_profile   (GtkWidget         *widget);

GimpColorTransform * gimp_widget_get_color_transform (GtkWidget         *widget,
                                                      GimpColorConfig   *config,
                                                      GimpColorProfile  *src_profile,
                                                      const Babl        *src_format,
                                                      const Babl        *dest_format,
                                                      GimpColorProfile  *softproof_profile,
                                                      GimpColorRenderingIntent proof_intent,
                                                      gboolean           proof_bpc);
const Babl         * gimp_widget_get_render_space    (GtkWidget       *widget,
                                                      GimpColorConfig *config);

void                 gimp_widget_set_native_handle   (GtkWidget        *widget,
                                                      GBytes          **handle);
void                 gimp_widget_free_native_handle  (GtkWidget        *widget,
                                                      GBytes          **window_handle);

gboolean             gimp_widget_animation_enabled   (void);

/* Internal use */

G_GNUC_INTERNAL void _gimp_widget_get_profiles       (GtkWidget         *widget,
                                                      GimpColorConfig   *config,
                                                      GimpColorProfile **proof_profile,
                                                      GimpColorProfile **dest_profile);


G_END_DECLS

#endif /* __GIMP_WIDGETS_UTILS_H__ */
