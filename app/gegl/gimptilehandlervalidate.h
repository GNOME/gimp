/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TILE_HANDLER_VALIDATE_H__
#define __LIGMA_TILE_HANDLER_VALIDATE_H__

#include <gegl-buffer-backend.h>

/***
 * LigmaTileHandlerValidate is a GeglTileHandler that renders the
 * projection.
 */

G_BEGIN_DECLS

#define LIGMA_TYPE_TILE_HANDLER_VALIDATE            (ligma_tile_handler_validate_get_type ())
#define LIGMA_TILE_HANDLER_VALIDATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TILE_HANDLER_VALIDATE, LigmaTileHandlerValidate))
#define LIGMA_TILE_HANDLER_VALIDATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_TILE_HANDLER_VALIDATE, LigmaTileHandlerValidateClass))
#define LIGMA_IS_TILE_HANDLER_VALIDATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TILE_HANDLER_VALIDATE))
#define LIGMA_IS_TILE_HANDLER_VALIDATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_TILE_HANDLER_VALIDATE))
#define LIGMA_TILE_HANDLER_VALIDATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_TILE_HANDLER_VALIDATE, LigmaTileHandlerValidateClass))


typedef struct _LigmaTileHandlerValidate      LigmaTileHandlerValidate;
typedef struct _LigmaTileHandlerValidateClass LigmaTileHandlerValidateClass;

struct _LigmaTileHandlerValidate
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

struct _LigmaTileHandlerValidateClass
{
  GeglTileHandlerClass  parent_class;

  /*  signals  */
  void (* invalidated)     (LigmaTileHandlerValidate *validate,
                            const GeglRectangle     *rect);

  /*  virtual functions  */
  void (* begin_validate)  (LigmaTileHandlerValidate *validate);
  void (* end_validate)    (LigmaTileHandlerValidate *validate);
  void (* validate)        (LigmaTileHandlerValidate *validate,
                            const GeglRectangle     *rect,
                            const Babl              *format,
                            gpointer                 dest_buf,
                            gint                     dest_stride);
  void (* validate_buffer) (LigmaTileHandlerValidate *validate,
                            const GeglRectangle     *rect,
                            GeglBuffer              *buffer);
};


GType                     ligma_tile_handler_validate_get_type          (void) G_GNUC_CONST;

GeglTileHandler         * ligma_tile_handler_validate_new               (GeglNode                *graph);

void                      ligma_tile_handler_validate_assign            (LigmaTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer);
void                      ligma_tile_handler_validate_unassign          (LigmaTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer);
LigmaTileHandlerValidate * ligma_tile_handler_validate_get_assigned      (GeglBuffer              *buffer);

void                      ligma_tile_handler_validate_invalidate        (LigmaTileHandlerValidate *validate,
                                                                        const GeglRectangle     *rect);
void                      ligma_tile_handler_validate_undo_invalidate   (LigmaTileHandlerValidate *validate,
                                                                        const GeglRectangle     *rect);

void                      ligma_tile_handler_validate_begin_validate    (LigmaTileHandlerValidate *validate);
void                      ligma_tile_handler_validate_end_validate      (LigmaTileHandlerValidate *validate);

void                      ligma_tile_handler_validate_validate          (LigmaTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer,
                                                                        const GeglRectangle     *rect,
                                                                        gboolean                 intersect,
                                                                        gboolean                 chunked);

gboolean                  ligma_tile_handler_validate_buffer_set_extent (GeglBuffer              *buffer,
                                                                        const GeglRectangle     *extent);

void                      ligma_tile_handler_validate_buffer_copy       (GeglBuffer              *src_buffer,
                                                                        const GeglRectangle     *src_rect,
                                                                        GeglBuffer              *dst_buffer,
                                                                        const GeglRectangle     *dst_rect);


G_END_DECLS

#endif /* __LIGMA_TILE_HANDLER_VALIDATE_H__ */
