/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmawidgetsutils.h
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_WIDGETS_UTILS_H__
#define __LIGMA_WIDGETS_UTILS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean             ligma_event_triggers_context_menu (const GdkEvent   *event,
                                                       gboolean          on_release);

GtkWidget          * ligma_grid_attach_aligned        (GtkGrid           *grid,
                                                      gint               left,
                                                      gint               top,
                                                      const gchar       *label_text,
                                                      gfloat             xalign,
                                                      gfloat             yalign,
                                                      GtkWidget         *widget,
                                                      gint               columns);

void                 ligma_label_set_attributes       (GtkLabel          *label,
                                                      ...);

GdkMonitor         * ligma_widget_get_monitor         (GtkWidget         *widget);
GdkMonitor         * ligma_get_monitor_at_pointer     (void);

void                 ligma_widget_track_monitor       (GtkWidget         *widget,
                                                      GCallback          monitor_changed_callback,
                                                      gpointer           user_data,
                                                      GDestroyNotify     user_data_destroy);

LigmaColorProfile   * ligma_monitor_get_color_profile  (GdkMonitor        *monitor);
LigmaColorProfile   * ligma_widget_get_color_profile   (GtkWidget         *widget);

LigmaColorTransform * ligma_widget_get_color_transform (GtkWidget         *widget,
                                                      LigmaColorConfig   *config,
                                                      LigmaColorProfile  *src_profile,
                                                      const Babl        *src_format,
                                                      const Babl        *dest_format,
                                                      LigmaColorProfile  *softproof_profile,
                                                      LigmaColorRenderingIntent proof_intent,
                                                      gboolean           proof_bpc);


G_END_DECLS

#endif /* __LIGMA_WIDGETS_UTILS_H__ */
