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

#ifndef __LIGMA_AIRBRUSH_H__
#define __LIGMA_AIRBRUSH_H__


#include "ligmapaintbrush.h"


#define LIGMA_TYPE_AIRBRUSH            (ligma_airbrush_get_type ())
#define LIGMA_AIRBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_AIRBRUSH, LigmaAirbrush))
#define LIGMA_AIRBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_AIRBRUSH, LigmaAirbrushClass))
#define LIGMA_IS_AIRBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_AIRBRUSH))
#define LIGMA_IS_AIRBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_AIRBRUSH))
#define LIGMA_AIRBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_AIRBRUSH, LigmaAirbrushClass))


typedef struct _LigmaAirbrushClass LigmaAirbrushClass;

struct _LigmaAirbrush
{
  LigmaPaintbrush    parent_instance;

  guint             timeout_id;

  LigmaSymmetry     *sym;
  LigmaDrawable     *drawable;
  LigmaPaintOptions *paint_options;
  LigmaCoords        coords;
};

struct _LigmaAirbrushClass
{
  LigmaPaintbrushClass  parent_class;

  /*  signals  */
  void (* stamp) (LigmaAirbrush *airbrush);
};


void    ligma_airbrush_register (Ligma                      *ligma,
                                LigmaPaintRegisterCallback  callback);

GType   ligma_airbrush_get_type (void) G_GNUC_CONST;

void    ligma_airbrush_stamp    (LigmaAirbrush              *airbrush);


#endif  /*  __LIGMA_AIRBRUSH_H__  */
