/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __APP_GIMP_UTILS_H__
#define __APP_GIMP_UTILS_H__


gboolean      gimp_rectangle_intersect      (gint          x1,
                                             gint          y1,
                                             gint          width1,
                                             gint          height1,
                                             gint          x2,
                                             gint          y2,
                                             gint          width2,
                                             gint          height2,
                                             gint         *dest_x,
                                             gint         *dest_y,
                                             gint         *dest_width,
                                             gint         *dest_height);

gint64        gimp_g_object_get_memsize     (GObject      *object);
gint64        gimp_g_hash_table_get_memsize (GHashTable   *hash);
gint64        gimp_g_slist_get_memsize      (GSList       *slist,
                                             gint64        data_size);
gint64        gimp_g_list_get_memsize       (GList        *list,
                                             gint64        data_size);
gint64        gimp_g_value_get_memsize      (GValue       *value);

gchar       * gimp_get_default_language     (const gchar  *category);

gboolean      gimp_boolean_handled_accum    (GSignalInvocationHint *ihint,
                                             GValue       *return_accu,
                                             const GValue *handler_return,
                                             gpointer      dummy);

GParameter  * gimp_parameters_append        (GType         object_type,
                                             GParameter   *params,
                                             gint         *n_params,
                                            ...);
GParameter  * gimp_parameters_append_valist (GType         object_type,
                                             GParameter   *params,
                                             gint         *n_params,
                                             va_list       args);
void          gimp_parameters_free          (GParameter   *params,
                                             gint          n_params);

const gchar * gimp_check_glib_version       (guint         required_major,
                                             guint         required_minor,
                                             guint         required_micro);


#endif /* __APP_GIMP_UTILS_H__ */
