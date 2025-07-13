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


struct _GimpDrawablePrivate
{
  GeglBuffer       *buffer; /* buffer for drawable data */
  GeglBuffer       *shadow; /* shadow buffer            */

  GimpColorProfile *format_profile;

  GeglNode         *source_node;
  GeglNode         *buffer_source_node;
  GimpContainer    *filter_stack;
  GeglRectangle     bounding_box;

  GimpLayer        *floating_selection;
  GimpFilter       *fs_filter;
  GeglNode         *fs_crop_node;
  GimpApplicator   *fs_applicator;

  GeglNode         *mode_node;

  gint              paint_count;
  GeglBuffer       *paint_buffer;
  cairo_region_t   *paint_copy_region;
  cairo_region_t   *paint_update_region;

  gboolean          push_resize_undo;
};
