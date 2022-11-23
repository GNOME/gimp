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

#ifndef __LIGMA_OPERATION_CONFIG_H__
#define __LIGMA_OPERATION_CONFIG_H__


void            ligma_operation_config_register      (Ligma          *ligma,
                                                     const gchar   *operation,
                                                     GType          config_type);

GType           ligma_operation_config_get_type      (Ligma          *ligma,
                                                     const gchar   *operation,
                                                     const gchar   *icon_name,
                                                     GType          parent_type);

LigmaContainer * ligma_operation_config_get_container (Ligma          *ligma,
                                                     GType          config_type,
                                                     GCompareFunc   sort_func);

void            ligma_operation_config_serialize     (Ligma          *ligma,
                                                     LigmaContainer *container,
                                                     GFile         *file);
void            ligma_operation_config_deserialize   (Ligma          *ligma,
                                                     LigmaContainer *container,
                                                     GFile         *file);

void            ligma_operation_config_sync_node     (GObject       *config,
                                                     GeglNode      *node);
void            ligma_operation_config_connect_node  (GObject       *config,
                                                     GeglNode      *node);

GParamSpec ** ligma_operation_config_list_properties (GObject      *config,
                                                     GType         owner_type,
                                                     GParamFlags   flags,
                                                     guint        *n_pspecs);


#endif /* __LIGMA_OPERATION_CONFIG_H__ */
