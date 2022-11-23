/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaprojectable.h
 * Copyright (C) 2008  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PROJECTABLE_H__
#define __LIGMA_PROJECTABLE_H__


#define LIGMA_TYPE_PROJECTABLE (ligma_projectable_get_type ())
G_DECLARE_INTERFACE (LigmaProjectable, ligma_projectable, LIGMA, PROJECTABLE, GObject)


struct _LigmaProjectableInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void         (* invalidate)         (LigmaProjectable *projectable,
                                       gint             x,
                                       gint             y,
                                       gint             width,
                                       gint             height);
  void         (* flush)              (LigmaProjectable *projectable,
                                       gboolean         invalidate_preview);
  void         (* structure_changed)  (LigmaProjectable *projectable);
  void         (* bounds_changed)     (LigmaProjectable *projectable,
                                       gint             old_x,
                                       gint             old_y);

  /*  virtual functions  */
  LigmaImage  * (* get_image)          (LigmaProjectable *projectable);
  const Babl * (* get_format)         (LigmaProjectable *projectable);
  void         (* get_offset)         (LigmaProjectable *projectable,
                                       gint            *x,
                                       gint            *y);
  GeglRectangle (* get_bounding_box)  (LigmaProjectable *projectable);
  GeglNode   * (* get_graph)          (LigmaProjectable *projectable);
  void         (* begin_render)       (LigmaProjectable *projectable);
  void         (* end_render)         (LigmaProjectable *projectable);
  void         (* invalidate_preview) (LigmaProjectable *projectable);
};


void         ligma_projectable_invalidate         (LigmaProjectable *projectable,
                                                  gint             x,
                                                  gint             y,
                                                  gint             width,
                                                  gint             height);
void         ligma_projectable_flush              (LigmaProjectable *projectable,
                                                  gboolean         preview_invalidated);
void         ligma_projectable_structure_changed  (LigmaProjectable *projectable);
void         ligma_projectable_bounds_changed     (LigmaProjectable *projectable,
                                                  gint             old_x,
                                                  gint             old_y);

LigmaImage  * ligma_projectable_get_image          (LigmaProjectable *projectable);
const Babl * ligma_projectable_get_format         (LigmaProjectable *projectable);
void         ligma_projectable_get_offset         (LigmaProjectable *projectable,
                                                  gint            *x,
                                                  gint            *y);
GeglRectangle ligma_projectable_get_bounding_box  (LigmaProjectable *projectable);
GeglNode   * ligma_projectable_get_graph          (LigmaProjectable *projectable);
void         ligma_projectable_begin_render       (LigmaProjectable *projectable);
void         ligma_projectable_end_render         (LigmaProjectable *projectable);
void         ligma_projectable_invalidate_preview (LigmaProjectable *projectable);


#endif  /* __LIGMA_PROJECTABLE_H__ */
