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

#ifndef __LIGMA_PAINT_INFO_H__
#define __LIGMA_PAINT_INFO_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_PAINT_INFO            (ligma_paint_info_get_type ())
#define LIGMA_PAINT_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAINT_INFO, LigmaPaintInfo))
#define LIGMA_PAINT_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAINT_INFO, LigmaPaintInfoClass))
#define LIGMA_IS_PAINT_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAINT_INFO))
#define LIGMA_IS_PAINT_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAINT_INFO))
#define LIGMA_PAINT_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAINT_INFO, LigmaPaintInfoClass))


typedef struct _LigmaPaintInfoClass LigmaPaintInfoClass;

struct _LigmaPaintInfo
{
  LigmaViewable      parent_instance;

  Ligma             *ligma;

  GType             paint_type;
  GType             paint_options_type;

  gchar            *blurb;

  LigmaPaintOptions *paint_options;
};

struct _LigmaPaintInfoClass
{
  LigmaViewableClass  parent_class;
};


GType           ligma_paint_info_get_type     (void) G_GNUC_CONST;

LigmaPaintInfo * ligma_paint_info_new          (Ligma          *ligma,
                                              GType          paint_type,
                                              GType          paint_options_type,
                                              const gchar   *identifier,
                                              const gchar   *blurb,
                                              const gchar   *icon_name);

void            ligma_paint_info_set_standard (Ligma          *ligma,
                                              LigmaPaintInfo *paint_info);
LigmaPaintInfo * ligma_paint_info_get_standard (Ligma          *ligma);


#endif  /*  __LIGMA_PAINT_INFO_H__  */
