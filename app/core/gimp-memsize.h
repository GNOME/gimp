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

#ifndef __APP_LIGMA_MEMSIZE_H__
#define __APP_LIGMA_MEMSIZE_H__


gint64   ligma_g_type_instance_get_memsize      (GTypeInstance   *instance);
gint64   ligma_g_object_get_memsize             (GObject         *object);

gint64   ligma_g_hash_table_get_memsize         (GHashTable      *hash,
                                                gint64           data_size);
gint64   ligma_g_hash_table_get_memsize_foreach (GHashTable      *hash,
                                                LigmaMemsizeFunc  func,
                                                gint64          *gui_size);

gint64   ligma_g_slist_get_memsize              (GSList          *slist,
                                                gint64           data_size);
gint64   ligma_g_slist_get_memsize_foreach      (GSList          *slist,
                                                LigmaMemsizeFunc  func,
                                                gint64          *gui_size);

gint64   ligma_g_list_get_memsize               (GList           *list,
                                                gint64           data_size);
gint64   ligma_g_list_get_memsize_foreach       (GList           *list,
                                                LigmaMemsizeFunc  func,
                                                gint64          *gui_size);

gint64   ligma_g_queue_get_memsize              (GQueue          *queue,
                                                gint64           data_size);
gint64   ligma_g_queue_get_memsize_foreach      (GQueue          *queue,
                                                LigmaMemsizeFunc  func,
                                                gint64          *gui_size);

gint64   ligma_g_value_get_memsize              (GValue          *value);
gint64   ligma_g_param_spec_get_memsize         (GParamSpec      *pspec);

gint64   ligma_gegl_buffer_get_memsize          (GeglBuffer      *buffer);
gint64   ligma_gegl_pyramid_get_memsize         (GeglBuffer      *buffer);

gint64   ligma_string_get_memsize               (const gchar     *string);
gint64   ligma_parasite_get_memsize             (LigmaParasite    *parasite,
                                                gint64          *gui_size);


#endif /* __APP_LIGMA_MEMSIZE_H__ */
