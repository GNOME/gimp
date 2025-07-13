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


void                 floating_sel_attach            (GimpLayer     *layer,
                                                     GimpDrawable  *drawable);
void                 floating_sel_anchor            (GimpLayer     *layer);
gboolean             floating_sel_to_layer          (GimpLayer     *layer,
                                                     GError       **error);
void                 floating_sel_activate_drawable (GimpLayer     *layer);
const GimpBoundSeg * floating_sel_boundary          (GimpLayer     *layer,
                                                     gint          *n_segs);
void                 floating_sel_invalidate        (GimpLayer     *layer);
