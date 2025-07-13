/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpprojectable.h
 * Copyright (C) 2008  Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_PROJECTABLE (gimp_projectable_get_type ())
G_DECLARE_INTERFACE (GimpProjectable, gimp_projectable, GIMP, PROJECTABLE, GObject)


struct _GimpProjectableInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void         (* invalidate)         (GimpProjectable *projectable,
                                       gint             x,
                                       gint             y,
                                       gint             width,
                                       gint             height);
  void         (* flush)              (GimpProjectable *projectable,
                                       gboolean         invalidate_preview);
  void         (* structure_changed)  (GimpProjectable *projectable);
  void         (* bounds_changed)     (GimpProjectable *projectable,
                                       gint             old_x,
                                       gint             old_y);

  /*  virtual functions  */
  GimpImage  * (* get_image)          (GimpProjectable *projectable);
  const Babl * (* get_format)         (GimpProjectable *projectable);
  void         (* get_offset)         (GimpProjectable *projectable,
                                       gint            *x,
                                       gint            *y);
  GeglRectangle (* get_bounding_box)  (GimpProjectable *projectable);
  GeglNode   * (* get_graph)          (GimpProjectable *projectable);
  void         (* begin_render)       (GimpProjectable *projectable);
  void         (* end_render)         (GimpProjectable *projectable);
  void         (* invalidate_preview) (GimpProjectable *projectable);
};


void         gimp_projectable_invalidate         (GimpProjectable *projectable,
                                                  gint             x,
                                                  gint             y,
                                                  gint             width,
                                                  gint             height);
void         gimp_projectable_flush              (GimpProjectable *projectable,
                                                  gboolean         preview_invalidated);
void         gimp_projectable_structure_changed  (GimpProjectable *projectable);
void         gimp_projectable_bounds_changed     (GimpProjectable *projectable,
                                                  gint             old_x,
                                                  gint             old_y);

GimpImage  * gimp_projectable_get_image          (GimpProjectable *projectable);
const Babl * gimp_projectable_get_format         (GimpProjectable *projectable);
void         gimp_projectable_get_offset         (GimpProjectable *projectable,
                                                  gint            *x,
                                                  gint            *y);
GeglRectangle gimp_projectable_get_bounding_box  (GimpProjectable *projectable);
GeglNode   * gimp_projectable_get_graph          (GimpProjectable *projectable);
void         gimp_projectable_begin_render       (GimpProjectable *projectable);
void         gimp_projectable_end_render         (GimpProjectable *projectable);
void         gimp_projectable_invalidate_preview (GimpProjectable *projectable);
