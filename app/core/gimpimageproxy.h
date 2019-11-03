/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageviewable.h
 * Copyright (C) 2019 Ell
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

#ifndef __GIMP_IMAGE_VIEWABLE_H__
#define __GIMP_IMAGE_VIEWABLE_H__


#include "gimpviewable.h"


#define GIMP_TYPE_IMAGE_VIEWABLE            (gimp_image_viewable_get_type ())
#define GIMP_IMAGE_VIEWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_VIEWABLE, GimpImageViewable))
#define GIMP_IMAGE_VIEWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_VIEWABLE, GimpImageViewableClass))
#define GIMP_IS_IMAGE_VIEWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_VIEWABLE))
#define GIMP_IS_IMAGE_VIEWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_VIEWABLE))
#define GIMP_IMAGE_VIEWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_VIEWABLE, GimpImageViewableClass))


typedef struct _GimpImageViewablePrivate GimpImageViewablePrivate;
typedef struct _GimpImageViewableClass   GimpImageViewableClass;

struct _GimpImageViewable
{
  GimpViewable              parent_instance;

  GimpImageViewablePrivate *priv;
};

struct _GimpImageViewableClass
{
  GimpViewableClass  parent_class;
};


GType               gimp_image_viewable_get_type         (void) G_GNUC_CONST;

GimpImageViewable * gimp_image_viewable_new              (GimpImage *image);

GimpImage         * gimp_image_viewable_get_image        (GimpImageViewable *image_viewable);

void                gimp_image_viewable_set_show_all     (GimpImageViewable *image_viewable,
                                                          gboolean           show_all);
gboolean            gimp_image_viewable_get_show_all     (GimpImageViewable *image_viewable);

GeglRectangle       gimp_image_viewable_get_bounding_box (GimpImageViewable *image_viewable);


#endif /* __GIMP_IMAGE_VIEWABLE_H__ */
