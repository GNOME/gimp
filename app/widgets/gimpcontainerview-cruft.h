/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview-cruft.h
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


void      _gimp_container_view_connect_cruft    (GimpContainerView *view);
void      _gimp_container_view_disconnect_cruft (GimpContainerView *view);

void      _gimp_container_view_real_clear_items (GimpContainerView *view);

gpointer  _gimp_container_view_lookup           (GimpContainerView *view,
                                                 GimpViewable      *viewable);
gboolean  _gimp_container_view_contains         (GimpContainerView *view,
                                                 GList             *viewables);
