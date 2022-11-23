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

#ifndef __LIGMA_DODGE_BURN_OPTIONS_H__
#define __LIGMA_DODGE_BURN_OPTIONS_H__


#include "ligmapaintoptions.h"


#define LIGMA_TYPE_DODGE_BURN_OPTIONS            (ligma_dodge_burn_options_get_type ())
#define LIGMA_DODGE_BURN_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DODGE_BURN_OPTIONS, LigmaDodgeBurnOptions))
#define LIGMA_DODGE_BURN_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DODGE_BURN_OPTIONS, LigmaDodgeBurnOptionsClass))
#define LIGMA_IS_DODGE_BURN_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DODGE_BURN_OPTIONS))
#define LIGMA_IS_DODGE_BURN_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DODGE_BURN_OPTIONS))
#define LIGMA_DODGE_BURN_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DODGE_BURN_OPTIONS, LigmaDodgeBurnOptionsClass))


typedef struct _LigmaDodgeBurnOptionsClass LigmaDodgeBurnOptionsClass;

struct _LigmaDodgeBurnOptions
{
  LigmaPaintOptions   parent_instance;

  LigmaDodgeBurnType  type;
  LigmaTransferMode   mode;     /*highlights, midtones, shadows*/
  gdouble            exposure;
};

struct _LigmaDodgeBurnOptionsClass
{
  LigmaPaintOptionsClass  parent_class;
};


GType   ligma_dodge_burn_options_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_DODGE_BURN_OPTIONS_H__  */
