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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmachannelpropundo.h"
#include "ligmachannelundo.h"
#include "ligmadrawablemodundo.h"
#include "ligmadrawablepropundo.h"
#include "ligmadrawableundo.h"
#include "ligmafloatingselectionundo.h"
#include "ligmagrid.h"
#include "ligmagrouplayer.h"
#include "ligmagrouplayerundo.h"
#include "ligmaguide.h"
#include "ligmaguideundo.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaimageundo.h"
#include "ligmaitempropundo.h"
#include "ligmalayermask.h"
#include "ligmalayermaskpropundo.h"
#include "ligmalayermaskundo.h"
#include "ligmalayerpropundo.h"
#include "ligmalayerundo.h"
#include "ligmamaskundo.h"
#include "ligmasamplepoint.h"
#include "ligmasamplepointundo.h"
#include "ligmaselection.h"

#include "text/ligmatextlayer.h"
#include "text/ligmatextundo.h"

#include "vectors/ligmavectors.h"
#include "vectors/ligmavectorsmodundo.h"
#include "vectors/ligmavectorspropundo.h"
#include "vectors/ligmavectorsundo.h"

#include "ligma-intl.h"


/**************************/
/*  Image Property Undos  */
/**************************/

LigmaUndo *
ligma_image_undo_push_image_type (LigmaImage   *image,
                                 const gchar *undo_desc)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_TYPE, undo_desc,
                               LIGMA_DIRTY_IMAGE,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_precision (LigmaImage   *image,
                                      const gchar *undo_desc)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_PRECISION, undo_desc,
                               LIGMA_DIRTY_IMAGE,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_size (LigmaImage   *image,
                                 const gchar *undo_desc,
                                 gint         previous_origin_x,
                                 gint         previous_origin_y,
                                 gint         previous_width,
                                 gint         previous_height)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_SIZE, undo_desc,
                               LIGMA_DIRTY_IMAGE | LIGMA_DIRTY_IMAGE_SIZE,
                               "previous-origin-x", previous_origin_x,
                               "previous-origin-y", previous_origin_y,
                               "previous-width",    previous_width,
                               "previous-height",   previous_height,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_resolution (LigmaImage   *image,
                                       const gchar *undo_desc)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_RESOLUTION, undo_desc,
                               LIGMA_DIRTY_IMAGE,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_grid (LigmaImage   *image,
                                 const gchar *undo_desc,
                                 LigmaGrid    *grid)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GRID (grid), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_GRID, undo_desc,
                               LIGMA_DIRTY_IMAGE_META,
                               "grid", grid,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_colormap (LigmaImage   *image,
                                     const gchar *undo_desc)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_COLORMAP, undo_desc,
                               LIGMA_DIRTY_IMAGE,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_hidden_profile (LigmaImage   *image,
                                           const gchar *undo_desc)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_HIDDEN_PROFILE, undo_desc,
                               LIGMA_DIRTY_IMAGE,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_metadata (LigmaImage   *image,
                                     const gchar *undo_desc)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_IMAGE_METADATA, undo_desc,
                               LIGMA_DIRTY_IMAGE_META,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_parasite (LigmaImage          *image,
                                     const gchar        *undo_desc,
                                     const LigmaParasite *parasite)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_PARASITE_ATTACH, undo_desc,
                               LIGMA_DIRTY_IMAGE_META,
                               "parasite-name", ligma_parasite_get_name (parasite),
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_image_parasite_remove (LigmaImage   *image,
                                            const gchar *undo_desc,
                                            const gchar *name)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_IMAGE_UNDO,
                               LIGMA_UNDO_PARASITE_REMOVE, undo_desc,
                               LIGMA_DIRTY_IMAGE_META,
                               "parasite-name", name,
                               NULL);
}


/********************************/
/*  Guide & Sample Point Undos  */
/********************************/

LigmaUndo *
ligma_image_undo_push_guide (LigmaImage   *image,
                            const gchar *undo_desc,
                            LigmaGuide   *guide)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GUIDE (guide), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GUIDE_UNDO,
                               LIGMA_UNDO_GUIDE, undo_desc,
                               LIGMA_DIRTY_IMAGE_META,
                               "aux-item", guide,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_sample_point (LigmaImage       *image,
                                   const gchar     *undo_desc,
                                   LigmaSamplePoint *sample_point)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_SAMPLE_POINT_UNDO,
                               LIGMA_UNDO_SAMPLE_POINT, undo_desc,
                               LIGMA_DIRTY_IMAGE_META,
                               "aux-item", sample_point,
                               NULL);
}


/********************/
/*  Drawable Undos  */
/********************/

LigmaUndo *
ligma_image_undo_push_drawable (LigmaImage    *image,
                               const gchar  *undo_desc,
                               LigmaDrawable *drawable,
                               GeglBuffer   *buffer,
                               gint          x,
                               gint          y)
{
  LigmaItem *item;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  item = LIGMA_ITEM (drawable);

  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_DRAWABLE_UNDO,
                               LIGMA_UNDO_DRAWABLE, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",   item,
                               "buffer", buffer,
                               "x",      x,
                               "y",      y,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_drawable_mod (LigmaImage    *image,
                                   const gchar  *undo_desc,
                                   LigmaDrawable *drawable,
                                   gboolean      copy_buffer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_DRAWABLE_MOD_UNDO,
                               LIGMA_UNDO_DRAWABLE_MOD, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",        drawable,
                               "copy-buffer", copy_buffer,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_drawable_format (LigmaImage    *image,
                                      const gchar  *undo_desc,
                                      LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_DRAWABLE_PROP_UNDO,
                               LIGMA_UNDO_DRAWABLE_FORMAT, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item", drawable,
                               NULL);
}


/****************/
/*  Mask Undos  */
/****************/

LigmaUndo *
ligma_image_undo_push_mask (LigmaImage   *image,
                           const gchar *undo_desc,
                           LigmaChannel *mask)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CHANNEL (mask), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (mask)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_MASK_UNDO,
                               LIGMA_UNDO_MASK, undo_desc,
                               LIGMA_IS_SELECTION (mask) ?
                               LIGMA_DIRTY_SELECTION :
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item", mask,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_mask_precision (LigmaImage   *image,
                                     const gchar *undo_desc,
                                     LigmaChannel *mask)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CHANNEL (mask), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (mask)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_MASK_UNDO,
                               LIGMA_UNDO_MASK, undo_desc,
                               LIGMA_IS_SELECTION (mask) ?
                               LIGMA_DIRTY_SELECTION :
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",           mask,
                               "convert-format", TRUE,
                               NULL);
}


/****************/
/*  Item Undos  */
/****************/

LigmaUndo *
ligma_image_undo_push_item_reorder (LigmaImage   *image,
                                   const gchar *undo_desc,
                                   LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_REORDER, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_rename (LigmaImage   *image,
                                  const gchar *undo_desc,
                                  LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_RENAME, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_displace (LigmaImage   *image,
                                    const gchar *undo_desc,
                                    LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_DISPLACE, undo_desc,
                               LIGMA_IS_DRAWABLE (item) ?
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE :
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_VECTORS,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_visibility (LigmaImage   *image,
                                      const gchar *undo_desc,
                                      LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_VISIBILITY, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_color_tag (LigmaImage   *image,
                                     const gchar *undo_desc,
                                     LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_COLOR_TAG, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_lock_content (LigmaImage   *image,
                                        const gchar *undo_desc,
                                        LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_LOCK_CONTENT, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_lock_position (LigmaImage   *image,
                                         const gchar *undo_desc,
                                         LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_LOCK_POSITION, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_lock_visibility (LigmaImage   *image,
                                           const gchar *undo_desc,
                                           LigmaItem    *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_ITEM_LOCK_VISIBILITY, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_parasite (LigmaImage          *image,
                                    const gchar        *undo_desc,
                                    LigmaItem           *item,
                                    const LigmaParasite *parasite)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_PARASITE_ATTACH, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item",          item,
                               "parasite-name", ligma_parasite_get_name (parasite),
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_item_parasite_remove (LigmaImage   *image,
                                           const gchar *undo_desc,
                                           LigmaItem    *item,
                                           const gchar *name)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_ITEM_PROP_UNDO,
                               LIGMA_UNDO_PARASITE_REMOVE, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item",          item,
                               "parasite-name", name,
                               NULL);
}


/*****************/
/*  Layer Undos  */
/*****************/

LigmaUndo *
ligma_image_undo_push_layer_add (LigmaImage   *image,
                                const gchar *undo_desc,
                                LigmaLayer   *layer,
                                GList       *prev_layers)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (! ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  for (iter = prev_layers; iter; iter = iter->next)
    g_return_val_if_fail (LIGMA_IS_LAYER (iter->data), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_UNDO,
                               LIGMA_UNDO_LAYER_ADD, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",        layer,
                               "prev-layers", prev_layers,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_remove (LigmaImage   *image,
                                   const gchar *undo_desc,
                                   LigmaLayer   *layer,
                                   LigmaLayer   *prev_parent,
                                   gint         prev_position,
                                   GList       *prev_layers)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);
  g_return_val_if_fail (prev_parent == NULL || LIGMA_IS_LAYER (prev_parent),
                        NULL);

  for (iter = prev_layers; iter; iter = iter->next)
    g_return_val_if_fail (LIGMA_IS_LAYER (iter->data), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_UNDO,
                               LIGMA_UNDO_LAYER_REMOVE, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",          layer,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-layers",   prev_layers,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_mode (LigmaImage   *image,
                                 const gchar *undo_desc,
                                 LigmaLayer   *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_PROP_UNDO,
                               LIGMA_UNDO_LAYER_MODE, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_opacity (LigmaImage   *image,
                                    const gchar *undo_desc,
                                    LigmaLayer   *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_PROP_UNDO,
                               LIGMA_UNDO_LAYER_OPACITY, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_lock_alpha (LigmaImage   *image,
                                       const gchar *undo_desc,
                                       LigmaLayer   *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_PROP_UNDO,
                               LIGMA_UNDO_LAYER_LOCK_ALPHA, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}


/***********************/
/*  Group Layer Undos  */
/***********************/

LigmaUndo *
ligma_image_undo_push_group_layer_suspend_resize (LigmaImage      *image,
                                                 const gchar    *undo_desc,
                                                 LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_group_layer_resume_resize (LigmaImage      *image,
                                                const gchar    *undo_desc,
                                                LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_group_layer_suspend_mask (LigmaImage      *image,
                                               const gchar    *undo_desc,
                                               LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_group_layer_resume_mask (LigmaImage      *image,
                                              const gchar    *undo_desc,
                                              LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_RESUME_MASK, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_group_layer_start_transform (LigmaImage      *image,
                                                  const gchar    *undo_desc,
                                                  LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_group_layer_end_transform (LigmaImage      *image,
                                                const gchar    *undo_desc,
                                                LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_group_layer_convert (LigmaImage      *image,
                                          const gchar    *undo_desc,
                                          LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_GROUP_LAYER_UNDO,
                               LIGMA_UNDO_GROUP_LAYER_CONVERT, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item", group,
                               NULL);
}


/**********************/
/*  Text Layer Undos  */
/**********************/

LigmaUndo *
ligma_image_undo_push_text_layer (LigmaImage        *image,
                                 const gchar      *undo_desc,
                                 LigmaTextLayer    *layer,
                                 const GParamSpec *pspec)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_TEXT_UNDO,
                               LIGMA_UNDO_TEXT_LAYER, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item",  layer,
                               "param", pspec,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_text_layer_modified (LigmaImage     *image,
                                          const gchar   *undo_desc,
                                          LigmaTextLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_TEXT_UNDO,
                               LIGMA_UNDO_TEXT_LAYER_MODIFIED, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_text_layer_convert (LigmaImage     *image,
                                         const gchar   *undo_desc,
                                         LigmaTextLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_TEXT_UNDO,
                               LIGMA_UNDO_TEXT_LAYER_CONVERT, undo_desc,
                               LIGMA_DIRTY_ITEM,
                               "item", layer,
                               NULL);
}


/**********************/
/*  Layer Mask Undos  */
/**********************/

LigmaUndo *
ligma_image_undo_push_layer_mask_add (LigmaImage     *image,
                                     const gchar   *undo_desc,
                                     LigmaLayer     *layer,
                                     LigmaLayerMask *mask)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (! ligma_item_is_attached (LIGMA_ITEM (mask)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_MASK_UNDO,
                               LIGMA_UNDO_LAYER_MASK_ADD, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "layer-mask", mask,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_mask_remove (LigmaImage     *image,
                                        const gchar   *undo_desc,
                                        LigmaLayer     *layer,
                                        LigmaLayerMask *mask)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (mask)), NULL);
  g_return_val_if_fail (ligma_layer_mask_get_layer (mask) == layer, NULL);
  g_return_val_if_fail (ligma_layer_get_mask (layer) == mask, NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_MASK_UNDO,
                               LIGMA_UNDO_LAYER_MASK_REMOVE, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "layer-mask", mask,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_mask_apply (LigmaImage   *image,
                                       const gchar *undo_desc,
                                       LigmaLayer   *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_MASK_PROP_UNDO,
                               LIGMA_UNDO_LAYER_MASK_APPLY, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_layer_mask_show (LigmaImage   *image,
                                      const gchar *undo_desc,
                                      LigmaLayer   *layer)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_LAYER_MASK_PROP_UNDO,
                               LIGMA_UNDO_LAYER_MASK_SHOW, undo_desc,
                               LIGMA_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}


/*******************/
/*  Channel Undos  */
/*******************/

LigmaUndo *
ligma_image_undo_push_channel_add (LigmaImage   *image,
                                  const gchar *undo_desc,
                                  LigmaChannel *channel,
                                  GList       *prev_channels)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (! ligma_item_is_attached (LIGMA_ITEM (channel)), NULL);

  for (iter = prev_channels; iter; iter = iter->next)
    g_return_val_if_fail (LIGMA_IS_CHANNEL (iter->data), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_CHANNEL_UNDO,
                               LIGMA_UNDO_CHANNEL_ADD, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",          channel,
                               "prev-channels", prev_channels,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_channel_remove (LigmaImage   *image,
                                     const gchar *undo_desc,
                                     LigmaChannel *channel,
                                     LigmaChannel *prev_parent,
                                     gint         prev_position,
                                     GList       *prev_channels)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)), NULL);
  g_return_val_if_fail (prev_parent == NULL || LIGMA_IS_CHANNEL (prev_parent),
                        NULL);

  for (iter = prev_channels; iter; iter = iter->next)
    g_return_val_if_fail (LIGMA_IS_CHANNEL (iter->data), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_CHANNEL_UNDO,
                               LIGMA_UNDO_CHANNEL_REMOVE, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",          channel,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-channels", prev_channels,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_channel_color (LigmaImage   *image,
                                    const gchar *undo_desc,
                                    LigmaChannel *channel)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_CHANNEL_PROP_UNDO,
                               LIGMA_UNDO_CHANNEL_COLOR, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE,
                               "item", channel,
                               NULL);
}


/*******************/
/*  Vectors Undos  */
/*******************/

LigmaUndo *
ligma_image_undo_push_vectors_add (LigmaImage   *image,
                                  const gchar *undo_desc,
                                  LigmaVectors *vectors,
                                  GList       *prev_vectors)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (! ligma_item_is_attached (LIGMA_ITEM (vectors)), NULL);

  for (iter = prev_vectors; iter; iter = iter->next)
    g_return_val_if_fail (LIGMA_IS_VECTORS (iter->data), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_VECTORS_UNDO,
                               LIGMA_UNDO_VECTORS_ADD, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",         vectors,
                               "prev-vectors", prev_vectors,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_vectors_remove (LigmaImage   *image,
                                     const gchar *undo_desc,
                                     LigmaVectors *vectors,
                                     LigmaVectors *prev_parent,
                                     gint         prev_position,
                                     GList       *prev_vectors)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (vectors)), NULL);
  g_return_val_if_fail (prev_parent == NULL || LIGMA_IS_VECTORS (prev_parent),
                        NULL);

  for (iter = prev_vectors; iter; iter = iter->next)
    g_return_val_if_fail (LIGMA_IS_VECTORS (iter->data), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_VECTORS_UNDO,
                               LIGMA_UNDO_VECTORS_REMOVE, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item",          vectors,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-vectors",  prev_vectors,
                               NULL);
}

LigmaUndo *
ligma_image_undo_push_vectors_mod (LigmaImage   *image,
                                  const gchar *undo_desc,
                                  LigmaVectors *vectors)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (vectors)), NULL);

  return ligma_image_undo_push (image, LIGMA_TYPE_VECTORS_MOD_UNDO,
                               LIGMA_UNDO_VECTORS_MOD, undo_desc,
                               LIGMA_DIRTY_ITEM | LIGMA_DIRTY_VECTORS,
                               "item", vectors,
                               NULL);
}


/******************************/
/*  Floating Selection Undos  */
/******************************/

LigmaUndo *
ligma_image_undo_push_fs_to_layer (LigmaImage    *image,
                                  const gchar  *undo_desc,
                                  LigmaLayer    *floating_layer)
{
  LigmaUndo *undo;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER (floating_layer), NULL);

  undo = ligma_image_undo_push (image, LIGMA_TYPE_FLOATING_SELECTION_UNDO,
                               LIGMA_UNDO_FS_TO_LAYER, undo_desc,
                               LIGMA_DIRTY_IMAGE_STRUCTURE,
                               "item", floating_layer,
                               NULL);

  return undo;
}


/******************************************************************************/
/*  Something for which programmer is too lazy to write an undo function for  */
/******************************************************************************/

static void
undo_pop_cantundo (LigmaUndo            *undo,
                   LigmaUndoMode         undo_mode,
                   LigmaUndoAccumulator *accum)
{
  switch (undo_mode)
    {
    case LIGMA_UNDO_MODE_UNDO:
      ligma_message (undo->image->ligma, NULL, LIGMA_MESSAGE_WARNING,
                    _("Can't undo %s"), ligma_object_get_name (undo));
      break;

    case LIGMA_UNDO_MODE_REDO:
      break;
    }
}

LigmaUndo *
ligma_image_undo_push_cantundo (LigmaImage   *image,
                               const gchar *undo_desc)
{
  LigmaUndo *undo;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility.
   */

  undo = ligma_image_undo_push (image, LIGMA_TYPE_UNDO,
                               LIGMA_UNDO_CANT, undo_desc,
                               LIGMA_DIRTY_ALL,
                               NULL);

  if (undo)
    g_signal_connect (undo, "pop",
                      G_CALLBACK (undo_pop_cantundo),
                      NULL);

  return undo;
}
