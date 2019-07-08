/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#ifndef __GIMP_GUIDE_H__
#define __GIMP_GUIDE_H__


#include "gimpauxitem.h"


#define GIMP_GUIDE_POSITION_UNDEFINED G_MININT


#define GIMP_TYPE_GUIDE            (gimp_guide_get_type ())
#define GIMP_GUIDE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GUIDE, GimpGuide))
#define GIMP_GUIDE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GUIDE, GimpGuideClass))
#define GIMP_IS_GUIDE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GUIDE))
#define GIMP_IS_GUIDE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GUIDE))
#define GIMP_GUIDE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GUIDE, GimpGuideClass))


typedef struct _GimpGuidePrivate GimpGuidePrivate;
typedef struct _GimpGuideClass   GimpGuideClass;

struct _GimpGuide
{
  GimpAuxItem       parent_instance;

  GimpGuidePrivate *priv;
};

struct _GimpGuideClass
{
  GimpAuxItemClass  parent_class;
};


GType               gimp_guide_get_type         (void) G_GNUC_CONST;

GimpGuide *         gimp_guide_new              (GimpOrientationType  orientation,
                                                 guint32              guide_ID);
GimpGuide *         gimp_guide_custom_new       (GimpOrientationType  orientation,
                                                 guint32              guide_ID,
                                                 GimpGuideStyle       guide_style);

GimpOrientationType gimp_guide_get_orientation  (GimpGuide           *guide);
void                gimp_guide_set_orientation  (GimpGuide           *guide,
                                                 GimpOrientationType  orientation);

gint                gimp_guide_get_position     (GimpGuide           *guide);
void                gimp_guide_set_position     (GimpGuide           *guide,
                                                 gint                 position);

GimpGuideStyle      gimp_guide_get_style        (GimpGuide           *guide);
gboolean            gimp_guide_is_custom        (GimpGuide           *guide);


#endif /* __GIMP_GUIDE_H__ */
