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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DRAWABLE_PRIVATE_H__
#define __GIMP_DRAWABLE_PRIVATE_H__

struct _GimpDrawablePrivate
{
  GimpImageType  type;   /* type of drawable        */

  TileManager   *tiles;  /* tiles for drawable data */
  TileManager   *shadow; /* shadow buffer tiles     */

  GeglNode      *source_node;
  GeglNode      *tile_source_node;

  GimpLayer     *floating_selection;
  GeglNode      *fs_crop_node;
  GeglNode      *fs_opacity_node;
  GeglNode      *fs_offset_node;
  GeglNode      *fs_mode_node;

  GeglNode      *mode_node;

  GSList        *preview_cache; /* preview caches of the channel */
  gboolean       preview_valid; /* is the preview valid?         */
};

#endif /* __GIMP_DRAWABLE_PRIVATE_H__ */
