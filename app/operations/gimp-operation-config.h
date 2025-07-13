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


void            gimp_operation_config_init_start    (Gimp          *gimp);
void            gimp_operation_config_init_end      (Gimp          *gimp);
void            gimp_operation_config_exit          (Gimp          *gimp);

gboolean        gimp_operation_config_is_custom     (Gimp          *gimp,
                                                     const gchar   *operation);

void            gimp_operation_config_register      (Gimp          *gimp,
                                                     const gchar   *operation,
                                                     GType          config_type);

GType           gimp_operation_config_get_type      (Gimp          *gimp,
                                                     const gchar   *operation,
                                                     const gchar   *icon_name,
                                                     GType          parent_type);

GimpContainer * gimp_operation_config_get_container (Gimp          *gimp,
                                                     GType          config_type,
                                                     GCompareFunc   sort_func);

void            gimp_operation_config_serialize     (Gimp          *gimp,
                                                     GimpContainer *container,
                                                     GFile         *file);
void            gimp_operation_config_deserialize   (Gimp          *gimp,
                                                     GimpContainer *container,
                                                     GFile         *file);

void            gimp_operation_config_sync_node     (GObject       *config,
                                                     GeglNode      *node);
void            gimp_operation_config_connect_node  (GObject       *config,
                                                     GeglNode      *node);

GParamSpec ** gimp_operation_config_list_properties (GObject      *config,
                                                     GType         owner_type,
                                                     GParamFlags   flags,
                                                     guint        *n_pspecs);
