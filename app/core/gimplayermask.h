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

#ifndef __GIMP_LAYER_MASK_H__
#define __GIMP_LAYER_MASK_H__


#include "gimpchannel.h"


#define GIMP_TYPE_LAYER_MASK            (gimp_layer_mask_get_type ())
#define GIMP_LAYER_MASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_MASK, GimpLayerMask))
#define GIMP_LAYER_MASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_MASK, GimpLayerMaskClass))
#define GIMP_IS_LAYER_MASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_MASK))
#define GIMP_IS_LAYER_MASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_MASK))
#define GIMP_LAYER_MASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER_MASK, GimpLayerMaskClass))


typedef struct _GimpLayerMaskClass  GimpLayerMaskClass;

struct _GimpLayerMask
{
  GimpChannel  parent_instance;

  GimpLayer   *layer;

  gboolean     apply_mask;    /*  controls mask application  */
  gboolean     edit_mask;     /*  edit mask or layer?        */
  gboolean     show_mask;     /*  show mask or layer?        */
};

struct _GimpLayerMaskClass
{
  GimpChannelClass  parent_class;

  void (* apply_changed) (GimpLayerMask *layer_mask);
  void (* edit_changed)  (GimpLayerMask *layer_mask);
  void (* show_changed)  (GimpLayerMask *layer_mask);
};


/*  function declarations  */

GType           gimp_layer_mask_get_type    (void) G_GNUC_CONST;

GimpLayerMask * gimp_layer_mask_new            (GimpImage           *image,
                                             gint                 width,
                                             gint                 height,
                                             const gchar         *name,
                                             const GimpRGB       *color);

void            gimp_layer_mask_set_layer   (GimpLayerMask       *layer_mask,
                                             GimpLayer           *layer);
GimpLayer     * gimp_layer_mask_get_layer   (const GimpLayerMask *layer_mask);

void            gimp_layer_mask_set_apply   (GimpLayerMask       *layer_mask,
                                             gboolean             apply,
                                             gboolean             push_undo);
gboolean        gimp_layer_mask_get_apply   (const GimpLayerMask *layer_mask);

void            gimp_layer_mask_set_edit    (GimpLayerMask       *layer_mask,
                                             gboolean             edit);
gboolean        gimp_layer_mask_get_edit    (const GimpLayerMask *layer_mask);

void            gimp_layer_mask_set_show    (GimpLayerMask       *layer_mask,
                                             gboolean             show,
                                             gboolean             push_undo);
gboolean        gimp_layer_mask_get_show    (const GimpLayerMask *layer_mask);


#endif /* __GIMP_LAYER_MASK_H__ */
