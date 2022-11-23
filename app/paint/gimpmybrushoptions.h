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

#ifndef  __LIGMA_MYBRUSH_OPTIONS_H__
#define  __LIGMA_MYBRUSH_OPTIONS_H__


#include "ligmapaintoptions.h"


#define LIGMA_TYPE_MYBRUSH_OPTIONS            (ligma_mybrush_options_get_type ())
#define LIGMA_MYBRUSH_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MYBRUSH_OPTIONS, LigmaMybrushOptions))
#define LIGMA_MYBRUSH_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MYBRUSH_OPTIONS, LigmaMybrushOptionsClass))
#define LIGMA_IS_MYBRUSH_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MYBRUSH_OPTIONS))
#define LIGMA_IS_MYBRUSH_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MYBRUSH_OPTIONS))
#define LIGMA_MYBRUSH_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MYBRUSH_OPTIONS, LigmaMybrushOptionsClass))


typedef struct _LigmaMybrushOptionsClass LigmaMybrushOptionsClass;

struct _LigmaMybrushOptions
{
  LigmaPaintOptions  parent_instance;

  gdouble           radius;
  gdouble           opaque;
  gdouble           hardness;
  gboolean          eraser;
  gboolean          no_erasing;
};

struct _LigmaMybrushOptionsClass
{
  LigmaPaintOptionsClass  parent_instance;
};


GType   ligma_mybrush_options_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_MYBRUSH_OPTIONS_H__  */
