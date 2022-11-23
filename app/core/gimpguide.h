/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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

#ifndef __LIGMA_GUIDE_H__
#define __LIGMA_GUIDE_H__


#include "ligmaauxitem.h"


#define LIGMA_GUIDE_POSITION_UNDEFINED G_MININT


#define LIGMA_TYPE_GUIDE            (ligma_guide_get_type ())
#define LIGMA_GUIDE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GUIDE, LigmaGuide))
#define LIGMA_GUIDE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GUIDE, LigmaGuideClass))
#define LIGMA_IS_GUIDE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GUIDE))
#define LIGMA_IS_GUIDE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GUIDE))
#define LIGMA_GUIDE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GUIDE, LigmaGuideClass))


typedef struct _LigmaGuidePrivate LigmaGuidePrivate;
typedef struct _LigmaGuideClass   LigmaGuideClass;

struct _LigmaGuide
{
  LigmaAuxItem       parent_instance;

  LigmaGuidePrivate *priv;
};

struct _LigmaGuideClass
{
  LigmaAuxItemClass  parent_class;
};


GType               ligma_guide_get_type         (void) G_GNUC_CONST;

LigmaGuide *         ligma_guide_new              (LigmaOrientationType  orientation,
                                                 guint32              guide_ID);
LigmaGuide *         ligma_guide_custom_new       (LigmaOrientationType  orientation,
                                                 guint32              guide_ID,
                                                 LigmaGuideStyle       guide_style);

LigmaOrientationType ligma_guide_get_orientation  (LigmaGuide           *guide);
void                ligma_guide_set_orientation  (LigmaGuide           *guide,
                                                 LigmaOrientationType  orientation);

gint                ligma_guide_get_position     (LigmaGuide           *guide);
void                ligma_guide_set_position     (LigmaGuide           *guide,
                                                 gint                 position);

LigmaGuideStyle      ligma_guide_get_style        (LigmaGuide           *guide);
gboolean            ligma_guide_is_custom        (LigmaGuide           *guide);


#endif /* __LIGMA_GUIDE_H__ */
