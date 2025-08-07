/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview-private.h
 * Copyright (C) 2001-2025 Michael Natterer <mitch@gimp.org>
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


#define GIMP_CONTAINER_VIEW_GET_PRIVATE(obj) \
  (_gimp_container_view_get_private ((GimpContainerView *) (obj)))


typedef struct _GimpContainerViewPrivate GimpContainerViewPrivate;

struct _GimpContainerViewPrivate
{
  GimpContainer   *container;
  GimpContext     *context;

  gint             view_size;
  gint             view_border_width;
  gboolean         reorderable;
  GtkSelectionMode selection_mode;

  /*  initialized by subclass  */
  GtkWidget       *dnd_widget;

  /*  cruft  */
  GHashTable      *item_hash;
  GimpTreeHandler *name_changed_handler;
  GimpTreeHandler *expanded_changed_handler;
};


GimpContainerViewPrivate *
_gimp_container_view_get_private (GimpContainerView *view);
