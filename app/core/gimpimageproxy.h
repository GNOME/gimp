/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimageproxy.h
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

#ifndef __LIGMA_IMAGE_PROXY_H__
#define __LIGMA_IMAGE_PROXY_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_IMAGE_PROXY            (ligma_image_proxy_get_type ())
#define LIGMA_IMAGE_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_PROXY, LigmaImageProxy))
#define LIGMA_IMAGE_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE_PROXY, LigmaImageProxyClass))
#define LIGMA_IS_IMAGE_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_PROXY))
#define LIGMA_IS_IMAGE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE_PROXY))
#define LIGMA_IMAGE_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE_PROXY, LigmaImageProxyClass))


typedef struct _LigmaImageProxyPrivate LigmaImageProxyPrivate;
typedef struct _LigmaImageProxyClass   LigmaImageProxyClass;

struct _LigmaImageProxy
{
  LigmaViewable           parent_instance;

  LigmaImageProxyPrivate *priv;
};

struct _LigmaImageProxyClass
{
  LigmaViewableClass  parent_class;
};


GType            ligma_image_proxy_get_type         (void) G_GNUC_CONST;

LigmaImageProxy * ligma_image_proxy_new              (LigmaImage      *image);

LigmaImage      * ligma_image_proxy_get_image        (LigmaImageProxy *image_proxy);
              
void             ligma_image_proxy_set_show_all     (LigmaImageProxy *image_proxy,
                                                    gboolean        show_all);
gboolean         ligma_image_proxy_get_show_all     (LigmaImageProxy *image_proxy);
              
GeglRectangle    ligma_image_proxy_get_bounding_box (LigmaImageProxy *image_proxy);


#endif /* __LIGMA_IMAGE_PROXY_H__ */
