/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemtreeview-search.h
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


void   _gimp_item_tree_view_search_create       (GimpItemTreeView *view,
                                                 GtkWidget        *parent);
void   _gimp_item_tree_view_search_destroy      (GimpItemTreeView *view);

void   _gimp_item_tree_view_search_show         (GimpItemTreeView *view);
void   _gimp_item_tree_view_search_hide         (GimpItemTreeView *view);

void   _gimp_item_tree_view_search_update_links (GimpItemTreeView *view,
                                                 GimpImage        *image,
                                                 GType             item_type);
