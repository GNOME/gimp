/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_IMAGE_MAP_H__
#define __GIMP_IMAGE_MAP_H__


#include "gimpobject.h"


typedef void (* GimpImageMapApplyFunc) (gpointer     data,
                                        PixelRegion *srcPR,
                                        PixelRegion *destPR);


#define GIMP_TYPE_IMAGE_MAP            (gimp_image_map_get_type ())
#define GIMP_IMAGE_MAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_MAP, GimpImageMap))
#define GIMP_IMAGE_MAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_MAP, GimpImageMapClass))
#define GIMP_IS_IMAGE_MAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_MAP))
#define GIMP_IS_IMAGE_MAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_MAP))
#define GIMP_IMAGE_MAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_MAP, GimpImageMapClass))


typedef struct _GimpImageMapClass  GimpImageMapClass;

struct _GimpImageMapClass
{
  GimpObjectClass  parent_class;

  void (* flush) (GimpImageMap *image_map);
};


/*  Image Map functions  */

/*  Successive image_map_apply functions can be called, but eventually
 *  MUST be followed with an image_map_commit or an image_map_abort call
 *  The image map is no longer valid after a call to commit or abort.
 */

GType          gimp_image_map_get_type     (void) G_GNUC_CONST;

GimpImageMap * gimp_image_map_new          (GimpDrawable          *drawable,
                                            const gchar           *undo_desc);

void           gimp_image_map_apply        (GimpImageMap          *image_map,
                                            GimpImageMapApplyFunc  apply_func,
                                            gpointer               apply_data);
void           gimp_image_map_commit       (GimpImageMap          *image_map);
void           gimp_image_map_clear        (GimpImageMap          *image_map);
void           gimp_image_map_abort        (GimpImageMap          *image_map);


#endif /* __GIMP_IMAGE_MAP_H__ */
