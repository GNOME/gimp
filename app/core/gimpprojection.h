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

#ifndef __LIGMA_PROJECTION_H__
#define __LIGMA_PROJECTION_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_PROJECTION            (ligma_projection_get_type ())
#define LIGMA_PROJECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROJECTION, LigmaProjection))
#define LIGMA_PROJECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROJECTION, LigmaProjectionClass))
#define LIGMA_IS_PROJECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROJECTION))
#define LIGMA_IS_PROJECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROJECTION))
#define LIGMA_PROJECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROJECTION, LigmaProjectionClass))


typedef struct _LigmaProjectionPrivate LigmaProjectionPrivate;
typedef struct _LigmaProjectionClass   LigmaProjectionClass;

struct _LigmaProjection
{
  LigmaObject             parent_instance;

  LigmaProjectionPrivate *priv;
};

struct _LigmaProjectionClass
{
  LigmaObjectClass  parent_class;

  void (* update) (LigmaProjection *proj,
                   gboolean        now,
                   gint            x,
                   gint            y,
                   gint            width,
                   gint            height);
};


GType            ligma_projection_get_type          (void) G_GNUC_CONST;

LigmaProjection * ligma_projection_new               (LigmaProjectable   *projectable);

void             ligma_projection_set_priority      (LigmaProjection    *projection,
                                                    gint               priority);
gint             ligma_projection_get_priority      (LigmaProjection    *projection);

void             ligma_projection_set_priority_rect (LigmaProjection    *proj,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

void             ligma_projection_stop_rendering    (LigmaProjection    *proj);

void             ligma_projection_flush             (LigmaProjection    *proj);
void             ligma_projection_flush_now         (LigmaProjection    *proj,
                                                    gboolean           direct);
void             ligma_projection_finish_draw       (LigmaProjection    *proj);

gint64           ligma_projection_estimate_memsize  (LigmaImageBaseType  type,
                                                    LigmaComponentType  component_type,
                                                    gint               width,
                                                    gint               height);


#endif /*  __LIGMA_PROJECTION_H__  */
