/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageproxy.h
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

#ifndef __GIMP_IMAGE_PROXY_H__
#define __GIMP_IMAGE_PROXY_H__


#include "gimpviewable.h"


#define GIMP_TYPE_IMAGE_PROXY            (gimp_image_proxy_get_type ())
#define GIMP_IMAGE_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_PROXY, GimpImageProxy))
#define GIMP_IMAGE_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_PROXY, GimpImageProxyClass))
#define GIMP_IS_IMAGE_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_PROXY))
#define GIMP_IS_IMAGE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_PROXY))
#define GIMP_IMAGE_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_PROXY, GimpImageProxyClass))


typedef struct _GimpImageProxyPrivate GimpImageProxyPrivate;
typedef struct _GimpImageProxyClass   GimpImageProxyClass;

struct _GimpImageProxy
{
  GimpViewable           parent_instance;

  GimpImageProxyPrivate *priv;
};

struct _GimpImageProxyClass
{
  GimpViewableClass  parent_class;
};


GType            gimp_image_proxy_get_type         (void) G_GNUC_CONST;

GimpImageProxy * gimp_image_proxy_new              (GimpImage      *image);

GimpImage      * gimp_image_proxy_get_image        (GimpImageProxy *image_proxy);
              
void             gimp_image_proxy_set_show_all     (GimpImageProxy *image_proxy,
                                                    gboolean        show_all);
gboolean         gimp_image_proxy_get_show_all     (GimpImageProxy *image_proxy);
              
GeglRectangle    gimp_image_proxy_get_bounding_box (GimpImageProxy *image_proxy);


#endif /* __GIMP_IMAGE_PROXY_H__ */
