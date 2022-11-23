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

#ifndef __LIGMA_SAMPLE_POINT_H__
#define __LIGMA_SAMPLE_POINT_H__


#include "ligmaauxitem.h"


#define LIGMA_SAMPLE_POINT_POSITION_UNDEFINED G_MININT


#define LIGMA_TYPE_SAMPLE_POINT            (ligma_sample_point_get_type ())
#define LIGMA_SAMPLE_POINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SAMPLE_POINT, LigmaSamplePoint))
#define LIGMA_SAMPLE_POINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SAMPLE_POINT, LigmaSamplePointClass))
#define LIGMA_IS_SAMPLE_POINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SAMPLE_POINT))
#define LIGMA_IS_SAMPLE_POINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SAMPLE_POINT))
#define LIGMA_SAMPLE_POINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SAMPLE_POINT, LigmaSamplePointClass))


typedef struct _LigmaSamplePointPrivate LigmaSamplePointPrivate;
typedef struct _LigmaSamplePointClass   LigmaSamplePointClass;

struct _LigmaSamplePoint
{
  LigmaAuxItem             parent_instance;

  LigmaSamplePointPrivate *priv;
};

struct _LigmaSamplePointClass
{
  LigmaAuxItemClass  parent_class;
};


GType             ligma_sample_point_get_type      (void) G_GNUC_CONST;

LigmaSamplePoint * ligma_sample_point_new           (guint32            sample_point_ID);

void              ligma_sample_point_get_position  (LigmaSamplePoint   *sample_point,
                                                   gint              *position_x,
                                                   gint              *position_y);
void              ligma_sample_point_set_position  (LigmaSamplePoint   *sample_point,
                                                   gint               position_x,
                                                   gint               position_y);

LigmaColorPickMode ligma_sample_point_get_pick_mode (LigmaSamplePoint   *sample_point);
void              ligma_sample_point_set_pick_mode (LigmaSamplePoint   *sample_point,
                                                   LigmaColorPickMode  pick_mode);


#endif /* __LIGMA_SAMPLE_POINT_H__ */
