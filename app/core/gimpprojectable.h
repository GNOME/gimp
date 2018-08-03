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

#ifndef __GIMP_PROJECTABLE_H__
#define __GIMP_PROJECTABLE_H__


#define GIMP_TYPE_PROJECTABLE               (gimp_projectable_get_type ())
#define GIMP_IS_PROJECTABLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROJECTABLE))
#define GIMP_PROJECTABLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROJECTABLE, GimpProjectable))
#define GIMP_PROJECTABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_PROJECTABLE, GimpProjectableInterface))


typedef struct _GimpProjectableInterface GimpProjectableInterface;

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
                                       gint             old_y,
                                       gint             old_width,
                                       gint             old_height);

  /*  virtual functions  */
  GimpImage  * (* get_image)          (GimpProjectable *projectable);
  const Babl * (* get_format)         (GimpProjectable *projectable);
  void         (* get_offset)         (GimpProjectable *projectable,
                                       gint            *x,
                                       gint            *y);
  void         (* get_size)           (GimpProjectable *projectable,
                                       gint            *width,
                                       gint            *height);
  GeglNode   * (* get_graph)          (GimpProjectable *projectable);
  void         (* begin_render)       (GimpProjectable *projectable);
  void         (* end_render)         (GimpProjectable *projectable);
  void         (* invalidate_preview) (GimpProjectable *projectable);
};


GType        gimp_projectable_get_type           (void) G_GNUC_CONST;

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
                                                  gint             old_y,
                                                  gint             old_width,
                                                  gint             old_height);

GimpImage  * gimp_projectable_get_image          (GimpProjectable *projectable);
const Babl * gimp_projectable_get_format         (GimpProjectable *projectable);
void         gimp_projectable_get_offset         (GimpProjectable *projectable,
                                                  gint            *x,
                                                  gint            *y);
void         gimp_projectable_get_size           (GimpProjectable *projectable,
                                                  gint            *width,
                                                  gint            *height);
GeglNode   * gimp_projectable_get_graph          (GimpProjectable *projectable);
void         gimp_projectable_begin_render       (GimpProjectable *projectable);
void         gimp_projectable_end_render         (GimpProjectable *projectable);
void         gimp_projectable_invalidate_preview (GimpProjectable *projectable);


#endif  /* __GIMP_PROJECTABLE_H__ */
