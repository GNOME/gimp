/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetry-mandala.h
 * Copyright (C) 2015 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_MANDALA_H__
#define __LIGMA_MANDALA_H__


#include "ligmasymmetry.h"


#define LIGMA_TYPE_MANDALA            (ligma_mandala_get_type ())
#define LIGMA_MANDALA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MANDALA, LigmaMandala))
#define LIGMA_MANDALA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MANDALA, LigmaMandalaClass))
#define LIGMA_IS_MANDALA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MANDALA))
#define LIGMA_IS_MANDALA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MANDALA))
#define LIGMA_MANDALA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MANDALA, LigmaMandalaClass))


typedef struct _LigmaMandalaClass LigmaMandalaClass;

struct _LigmaMandala
{
  LigmaSymmetry  parent_instance;

  gdouble       center_x;
  gdouble       center_y;
  gint          size;
  gboolean      disable_transformation;
  gboolean      enable_reflection;

  LigmaGuide    *horizontal_guide;
  LigmaGuide    *vertical_guide;
};

struct _LigmaMandalaClass
{
  LigmaSymmetryClass  parent_class;
};


GType   ligma_mandala_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_MANDALA_H__  */
