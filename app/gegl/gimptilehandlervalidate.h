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

#include <gegl-buffer-backend.h>


/***
 * GimpTileHandlerValidate is a GeglTileHandler that renders the
 * projection.
 */

#define GIMP_TYPE_TILE_HANDLER_VALIDATE            (gimp_tile_handler_validate_get_type ())
#define GIMP_TILE_HANDLER_VALIDATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TILE_HANDLER_VALIDATE, GimpTileHandlerValidate))
#define GIMP_TILE_HANDLER_VALIDATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_TILE_HANDLER_VALIDATE, GimpTileHandlerValidateClass))
#define GIMP_IS_TILE_HANDLER_VALIDATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TILE_HANDLER_VALIDATE))
#define GIMP_IS_TILE_HANDLER_VALIDATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_TILE_HANDLER_VALIDATE))
#define GIMP_TILE_HANDLER_VALIDATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_TILE_HANDLER_VALIDATE, GimpTileHandlerValidateClass))


typedef struct _GimpTileHandlerValidate      GimpTileHandlerValidate;
typedef struct _GimpTileHandlerValidateClass GimpTileHandlerValidateClass;

struct _GimpTileHandlerValidate
{
  GeglTileHandler  parent_instance;

  GeglNode        *graph;
  cairo_region_t  *dirty_region;
  const Babl      *format;
  gint             tile_width;
  gint             tile_height;
  gboolean         whole_tile;
  gint             validating;
  gint             suspend_validate;
};

struct _GimpTileHandlerValidateClass
{
  GeglTileHandlerClass  parent_class;

  /*  signals  */
  void (* invalidated)     (GimpTileHandlerValidate *validate,
                            const GeglRectangle     *rect);

  /*  virtual functions  */
  void (* begin_validate)  (GimpTileHandlerValidate *validate);
  void (* end_validate)    (GimpTileHandlerValidate *validate);
  void (* validate)        (GimpTileHandlerValidate *validate,
                            const GeglRectangle     *rect,
                            const Babl              *format,
                            gpointer                 dest_buf,
                            gint                     dest_stride);
  void (* validate_buffer) (GimpTileHandlerValidate *validate,
                            const GeglRectangle     *rect,
                            GeglBuffer              *buffer);
};


GType                     gimp_tile_handler_validate_get_type          (void) G_GNUC_CONST;

GeglTileHandler         * gimp_tile_handler_validate_new               (GeglNode                *graph);

void                      gimp_tile_handler_validate_assign            (GimpTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer);
void                      gimp_tile_handler_validate_unassign          (GimpTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer);
GimpTileHandlerValidate * gimp_tile_handler_validate_get_assigned      (GeglBuffer              *buffer);

void                      gimp_tile_handler_validate_invalidate        (GimpTileHandlerValidate *validate,
                                                                        const GeglRectangle     *rect);
void                      gimp_tile_handler_validate_undo_invalidate   (GimpTileHandlerValidate *validate,
                                                                        const GeglRectangle     *rect);

void                      gimp_tile_handler_validate_begin_validate    (GimpTileHandlerValidate *validate);
void                      gimp_tile_handler_validate_end_validate      (GimpTileHandlerValidate *validate);

void                      gimp_tile_handler_validate_validate          (GimpTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer,
                                                                        const GeglRectangle     *rect,
                                                                        gboolean                 intersect,
                                                                        gboolean                 chunked);

gboolean                  gimp_tile_handler_validate_buffer_set_extent (GeglBuffer              *buffer,
                                                                        const GeglRectangle     *extent);

void                      gimp_tile_handler_validate_buffer_copy       (GeglBuffer              *src_buffer,
                                                                        const GeglRectangle     *src_rect,
                                                                        GeglBuffer              *dst_buffer,
                                                                        const GeglRectangle     *dst_rect);
