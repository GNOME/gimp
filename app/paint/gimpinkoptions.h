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

#ifndef  __LIGMA_INK_OPTIONS_H__
#define  __LIGMA_INK_OPTIONS_H__


#include "ligmapaintoptions.h"


#define LIGMA_TYPE_INK_OPTIONS            (ligma_ink_options_get_type ())
#define LIGMA_INK_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INK_OPTIONS, LigmaInkOptions))
#define LIGMA_INK_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_INK_OPTIONS, LigmaInkOptionsClass))
#define LIGMA_IS_INK_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INK_OPTIONS))
#define LIGMA_IS_INK_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_INK_OPTIONS))
#define LIGMA_INK_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_INK_OPTIONS, LigmaInkOptionsClass))


typedef struct _LigmaInkOptionsClass LigmaInkOptionsClass;

struct _LigmaInkOptions
{
  LigmaPaintOptions  parent_instance;

  gdouble           size;
  gdouble           tilt_angle;

  gdouble           size_sensitivity;
  gdouble           vel_sensitivity;
  gdouble           tilt_sensitivity;

  LigmaInkBlobType   blob_type;
  gdouble           blob_aspect;
  gdouble           blob_angle;
};

struct _LigmaInkOptionsClass
{
  LigmaPaintOptionsClass  parent_instance;
};


GType   ligma_ink_options_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_INK_OPTIONS_H__  */
