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

#ifndef __LIGMA_LAYER_MASK_H__
#define __LIGMA_LAYER_MASK_H__


#include "ligmachannel.h"


#define LIGMA_TYPE_LAYER_MASK            (ligma_layer_mask_get_type ())
#define LIGMA_LAYER_MASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_MASK, LigmaLayerMask))
#define LIGMA_LAYER_MASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER_MASK, LigmaLayerMaskClass))
#define LIGMA_IS_LAYER_MASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_MASK))
#define LIGMA_IS_LAYER_MASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER_MASK))
#define LIGMA_LAYER_MASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LAYER_MASK, LigmaLayerMaskClass))


typedef struct _LigmaLayerMaskClass  LigmaLayerMaskClass;

struct _LigmaLayerMask
{
  LigmaChannel  parent_instance;

  LigmaLayer   *layer;
};

struct _LigmaLayerMaskClass
{
  LigmaChannelClass  parent_class;
};


GType           ligma_layer_mask_get_type  (void) G_GNUC_CONST;

LigmaLayerMask * ligma_layer_mask_new       (LigmaImage     *image,
                                           gint           width,
                                           gint           height,
                                           const gchar   *name,
                                           const LigmaRGB *color);

void            ligma_layer_mask_set_layer (LigmaLayerMask *layer_mask,
                                           LigmaLayer     *layer);
LigmaLayer     * ligma_layer_mask_get_layer (LigmaLayerMask *layer_mask);


#endif /* __LIGMA_LAYER_MASK_H__ */
