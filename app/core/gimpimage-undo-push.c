/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpchannelpropundo.h"
#include "gimpchannelundo.h"
#include "gimpdrawablemodundo.h"
#include "gimpdrawableundo.h"
#include "gimpfloatingselectionundo.h"
#include "gimpgrid.h"
#include "gimpgrouplayer.h"
#include "gimpgrouplayerundo.h"
#include "gimpguide.h"
#include "gimpguideundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpimageundo.h"
#include "gimpitempropundo.h"
#include "gimplayermask.h"
#include "gimplayermaskpropundo.h"
#include "gimplayermaskundo.h"
#include "gimplayerpropundo.h"
#include "gimplayerundo.h"
#include "gimpmaskundo.h"
#include "gimpsamplepoint.h"
#include "gimpsamplepointundo.h"
#include "gimpselection.h"

#include "text/gimptextlayer.h"
#include "text/gimptextundo.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectorsmodundo.h"
#include "vectors/gimpvectorspropundo.h"
#include "vectors/gimpvectorsundo.h"

#include "gimp-intl.h"


/**************************/
/*  Image Property Undos  */
/**************************/

GimpUndo *
gimp_image_undo_push_image_type (GimpImage   *image,
                                 const gchar *undo_desc)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_TYPE, undo_desc,
                               GIMP_DIRTY_IMAGE,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_precision (GimpImage   *image,
                                      const gchar *undo_desc)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_PRECISION, undo_desc,
                               GIMP_DIRTY_IMAGE,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_size (GimpImage   *image,
                                 const gchar *undo_desc,
                                 gint         previous_origin_x,
                                 gint         previous_origin_y,
                                 gint         previous_width,
                                 gint         previous_height)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_SIZE, undo_desc,
                               GIMP_DIRTY_IMAGE | GIMP_DIRTY_IMAGE_SIZE,
                               "previous-origin-x", previous_origin_x,
                               "previous-origin-y", previous_origin_y,
                               "previous-width",    previous_width,
                               "previous-height",   previous_height,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_resolution (GimpImage   *image,
                                       const gchar *undo_desc)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_RESOLUTION, undo_desc,
                               GIMP_DIRTY_IMAGE,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_grid (GimpImage   *image,
                                 const gchar *undo_desc,
                                 GimpGrid    *grid)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GRID (grid), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_GRID, undo_desc,
                               GIMP_DIRTY_IMAGE_META,
                               "grid", grid,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_colormap (GimpImage   *image,
                                     const gchar *undo_desc)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_COLORMAP, undo_desc,
                               GIMP_DIRTY_IMAGE,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_color_managed (GimpImage   *image,
                                          const gchar *undo_desc)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_COLOR_MANAGED, undo_desc,
                               GIMP_DIRTY_IMAGE,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_metadata (GimpImage   *image,
                                     const gchar *undo_desc)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_IMAGE_METADATA, undo_desc,
                               GIMP_DIRTY_IMAGE_META,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_parasite (GimpImage          *image,
                                     const gchar        *undo_desc,
                                     const GimpParasite *parasite)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_PARASITE_ATTACH, undo_desc,
                               GIMP_DIRTY_IMAGE_META,
                               "parasite-name", gimp_parasite_name (parasite),
                               NULL);
}

GimpUndo *
gimp_image_undo_push_image_parasite_remove (GimpImage   *image,
                                            const gchar *undo_desc,
                                            const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                               GIMP_UNDO_PARASITE_REMOVE, undo_desc,
                               GIMP_DIRTY_IMAGE_META,
                               "parasite-name", name,
                               NULL);
}


/********************************/
/*  Guide & Sample Point Undos  */
/********************************/

GimpUndo *
gimp_image_undo_push_guide (GimpImage   *image,
                            const gchar *undo_desc,
                            GimpGuide   *guide)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GUIDE_UNDO,
                               GIMP_UNDO_GUIDE, undo_desc,
                               GIMP_DIRTY_IMAGE_META,
                               "aux-item", guide,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_sample_point (GimpImage       *image,
                                   const gchar     *undo_desc,
                                   GimpSamplePoint *sample_point)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_SAMPLE_POINT (sample_point), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_SAMPLE_POINT_UNDO,
                               GIMP_UNDO_SAMPLE_POINT, undo_desc,
                               GIMP_DIRTY_IMAGE_META,
                               "aux-item", sample_point,
                               NULL);
}


/********************/
/*  Drawable Undos  */
/********************/

GimpUndo *
gimp_image_undo_push_drawable (GimpImage    *image,
                               const gchar  *undo_desc,
                               GimpDrawable *drawable,
                               GeglBuffer   *buffer,
                               gint          x,
                               gint          y)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_DRAWABLE_UNDO,
                               GIMP_UNDO_DRAWABLE, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",   item,
                               "buffer", buffer,
                               "x",      x,
                               "y",      y,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_drawable_mod (GimpImage    *image,
                                   const gchar  *undo_desc,
                                   GimpDrawable *drawable,
                                   gboolean      copy_buffer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_DRAWABLE_MOD_UNDO,
                               GIMP_UNDO_DRAWABLE_MOD, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",        drawable,
                               "copy-buffer", copy_buffer,
                               NULL);
}


/****************/
/*  Mask Undos  */
/****************/

GimpUndo *
gimp_image_undo_push_mask (GimpImage   *image,
                           const gchar *undo_desc,
                           GimpChannel *mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (mask)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_MASK_UNDO,
                               GIMP_UNDO_MASK, undo_desc,
                               GIMP_IS_SELECTION (mask) ?
                               GIMP_DIRTY_SELECTION :
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item", mask,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_mask_precision (GimpImage   *image,
                                     const gchar *undo_desc,
                                     GimpChannel *mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (mask)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_MASK_UNDO,
                               GIMP_UNDO_MASK, undo_desc,
                               GIMP_IS_SELECTION (mask) ?
                               GIMP_DIRTY_SELECTION :
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",           mask,
                               "convert-format", TRUE,
                               NULL);
}


/****************/
/*  Item Undos  */
/****************/

GimpUndo *
gimp_image_undo_push_item_reorder (GimpImage   *image,
                                   const gchar *undo_desc,
                                   GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_REORDER, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_rename (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_RENAME, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_displace (GimpImage   *image,
                                    const gchar *undo_desc,
                                    GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_DISPLACE, undo_desc,
                               GIMP_IS_DRAWABLE (item) ?
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE :
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_VECTORS,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_visibility (GimpImage   *image,
                                      const gchar *undo_desc,
                                      GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_VISIBILITY, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_linked (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_LINKED, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_color_tag (GimpImage   *image,
                                     const gchar *undo_desc,
                                     GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_COLOR_TAG, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_lock_content (GimpImage   *image,
                                        const gchar *undo_desc,
                                        GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_LOCK_CONTENT, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_lock_position (GimpImage   *image,
                                         const gchar *undo_desc,
                                         GimpItem    *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_ITEM_LOCK_POSITION, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_parasite (GimpImage          *image,
                                    const gchar        *undo_desc,
                                    GimpItem           *item,
                                    const GimpParasite *parasite)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_PARASITE_ATTACH, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item",          item,
                               "parasite-name", gimp_parasite_name (parasite),
                               NULL);
}

GimpUndo *
gimp_image_undo_push_item_parasite_remove (GimpImage   *image,
                                           const gchar *undo_desc,
                                           GimpItem    *item,
                                           const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                               GIMP_UNDO_PARASITE_REMOVE, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item",          item,
                               "parasite-name", name,
                               NULL);
}


/*****************/
/*  Layer Undos  */
/*****************/

GimpUndo *
gimp_image_undo_push_layer_add (GimpImage   *image,
                                const gchar *undo_desc,
                                GimpLayer   *layer,
                                GimpLayer   *prev_layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (layer)), NULL);
  g_return_val_if_fail (prev_layer == NULL || GIMP_IS_LAYER (prev_layer),
                        NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_UNDO,
                               GIMP_UNDO_LAYER_ADD, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "prev-layer", prev_layer,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_remove (GimpImage   *image,
                                   const gchar *undo_desc,
                                   GimpLayer   *layer,
                                   GimpLayer   *prev_parent,
                                   gint         prev_position,
                                   GimpLayer   *prev_layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);
  g_return_val_if_fail (prev_parent == NULL || GIMP_IS_LAYER (prev_parent),
                        NULL);
  g_return_val_if_fail (prev_layer == NULL || GIMP_IS_LAYER (prev_layer),
                        NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_UNDO,
                               GIMP_UNDO_LAYER_REMOVE, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",          layer,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-layer",    prev_layer,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_mode (GimpImage   *image,
                                 const gchar *undo_desc,
                                 GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                               GIMP_UNDO_LAYER_MODE, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_opacity (GimpImage   *image,
                                    const gchar *undo_desc,
                                    GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                               GIMP_UNDO_LAYER_OPACITY, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_lock_alpha (GimpImage   *image,
                                       const gchar *undo_desc,
                                       GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                               GIMP_UNDO_LAYER_LOCK_ALPHA, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}


/***********************/
/*  Group Layer Undos  */
/***********************/

GimpUndo *
gimp_image_undo_push_group_layer_suspend_resize (GimpImage      *image,
                                                 const gchar    *undo_desc,
                                                 GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_SUSPEND_RESIZE, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_group_layer_resume_resize (GimpImage      *image,
                                                const gchar    *undo_desc,
                                                GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_RESUME_RESIZE, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_group_layer_suspend_mask (GimpImage      *image,
                                               const gchar    *undo_desc,
                                               GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_SUSPEND_MASK, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_group_layer_resume_mask (GimpImage      *image,
                                              const gchar    *undo_desc,
                                              GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_RESUME_MASK, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_group_layer_start_transform (GimpImage      *image,
                                                  const gchar    *undo_desc,
                                                  GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_START_TRANSFORM, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_group_layer_end_transform (GimpImage      *image,
                                                const gchar    *undo_desc,
                                                GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_END_TRANSFORM, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_group_layer_convert (GimpImage      *image,
                                          const gchar    *undo_desc,
                                          GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_GROUP_LAYER_UNDO,
                               GIMP_UNDO_GROUP_LAYER_CONVERT, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item", group,
                               NULL);
}


/**********************/
/*  Text Layer Undos  */
/**********************/

GimpUndo *
gimp_image_undo_push_text_layer (GimpImage        *image,
                                 const gchar      *undo_desc,
                                 GimpTextLayer    *layer,
                                 const GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_TEXT_UNDO,
                               GIMP_UNDO_TEXT_LAYER, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item",  layer,
                               "param", pspec,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_text_layer_modified (GimpImage     *image,
                                          const gchar   *undo_desc,
                                          GimpTextLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_TEXT_UNDO,
                               GIMP_UNDO_TEXT_LAYER_MODIFIED, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_text_layer_convert (GimpImage     *image,
                                         const gchar   *undo_desc,
                                         GimpTextLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_TEXT_UNDO,
                               GIMP_UNDO_TEXT_LAYER_CONVERT, undo_desc,
                               GIMP_DIRTY_ITEM,
                               "item", layer,
                               NULL);
}


/**********************/
/*  Layer Mask Undos  */
/**********************/

GimpUndo *
gimp_image_undo_push_layer_mask_add (GimpImage     *image,
                                     const gchar   *undo_desc,
                                     GimpLayer     *layer,
                                     GimpLayerMask *mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (mask)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_MASK_UNDO,
                               GIMP_UNDO_LAYER_MASK_ADD, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "layer-mask", mask,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_mask_remove (GimpImage     *image,
                                        const gchar   *undo_desc,
                                        GimpLayer     *layer,
                                        GimpLayerMask *mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (mask)), NULL);
  g_return_val_if_fail (gimp_layer_mask_get_layer (mask) == layer, NULL);
  g_return_val_if_fail (gimp_layer_get_mask (layer) == mask, NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_MASK_UNDO,
                               GIMP_UNDO_LAYER_MASK_REMOVE, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "layer-mask", mask,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_mask_apply (GimpImage   *image,
                                       const gchar *undo_desc,
                                       GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_MASK_PROP_UNDO,
                               GIMP_UNDO_LAYER_MASK_APPLY, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_layer_mask_show (GimpImage   *image,
                                      const gchar *undo_desc,
                                      GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_LAYER_MASK_PROP_UNDO,
                               GIMP_UNDO_LAYER_MASK_SHOW, undo_desc,
                               GIMP_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}


/*******************/
/*  Channel Undos  */
/*******************/

GimpUndo *
gimp_image_undo_push_channel_add (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpChannel *channel,
                                  GimpChannel *prev_channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (channel)), NULL);
  g_return_val_if_fail (prev_channel == NULL || GIMP_IS_CHANNEL (prev_channel),
                        NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_CHANNEL_UNDO,
                               GIMP_UNDO_CHANNEL_ADD, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",         channel,
                               "prev-channel", prev_channel,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_channel_remove (GimpImage   *image,
                                     const gchar *undo_desc,
                                     GimpChannel *channel,
                                     GimpChannel *prev_parent,
                                     gint         prev_position,
                                     GimpChannel *prev_channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)), NULL);
  g_return_val_if_fail (prev_parent == NULL || GIMP_IS_CHANNEL (prev_parent),
                        NULL);
  g_return_val_if_fail (prev_channel == NULL || GIMP_IS_CHANNEL (prev_channel),
                        NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_CHANNEL_UNDO,
                               GIMP_UNDO_CHANNEL_REMOVE, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",          channel,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-channel",  prev_channel,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_channel_color (GimpImage   *image,
                                    const gchar *undo_desc,
                                    GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_CHANNEL_PROP_UNDO,
                               GIMP_UNDO_CHANNEL_COLOR, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                               "item", channel,
                               NULL);
}


/*******************/
/*  Vectors Undos  */
/*******************/

GimpUndo *
gimp_image_undo_push_vectors_add (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpVectors *vectors,
                                  GimpVectors *prev_vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (vectors)), NULL);
  g_return_val_if_fail (prev_vectors == NULL || GIMP_IS_VECTORS (prev_vectors),
                        NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_VECTORS_UNDO,
                               GIMP_UNDO_VECTORS_ADD, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",         vectors,
                               "prev-vectors", prev_vectors,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_vectors_remove (GimpImage   *image,
                                     const gchar *undo_desc,
                                     GimpVectors *vectors,
                                     GimpVectors *prev_parent,
                                     gint         prev_position,
                                     GimpVectors *prev_vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (vectors)), NULL);
  g_return_val_if_fail (prev_parent == NULL || GIMP_IS_VECTORS (prev_parent),
                        NULL);
  g_return_val_if_fail (prev_vectors == NULL || GIMP_IS_VECTORS (prev_vectors),
                        NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_VECTORS_UNDO,
                               GIMP_UNDO_VECTORS_REMOVE, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item",          vectors,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-vectors",  prev_vectors,
                               NULL);
}

GimpUndo *
gimp_image_undo_push_vectors_mod (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (vectors)), NULL);

  return gimp_image_undo_push (image, GIMP_TYPE_VECTORS_MOD_UNDO,
                               GIMP_UNDO_VECTORS_MOD, undo_desc,
                               GIMP_DIRTY_ITEM | GIMP_DIRTY_VECTORS,
                               "item", vectors,
                               NULL);
}


/******************************/
/*  Floating Selection Undos  */
/******************************/

GimpUndo *
gimp_image_undo_push_fs_to_layer (GimpImage    *image,
                                  const gchar  *undo_desc,
                                  GimpLayer    *floating_layer)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), NULL);

  undo = gimp_image_undo_push (image, GIMP_TYPE_FLOATING_SELECTION_UNDO,
                               GIMP_UNDO_FS_TO_LAYER, undo_desc,
                               GIMP_DIRTY_IMAGE_STRUCTURE,
                               "item", floating_layer,
                               NULL);

  return undo;
}


/******************************************************************************/
/*  Something for which programmer is too lazy to write an undo function for  */
/******************************************************************************/

static void
undo_pop_cantundo (GimpUndo            *undo,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      gimp_message (undo->image->gimp, NULL, GIMP_MESSAGE_WARNING,
                    _("Can't undo %s"), gimp_object_get_name (undo));
      break;

    case GIMP_UNDO_MODE_REDO:
      break;
    }
}

GimpUndo *
gimp_image_undo_push_cantundo (GimpImage   *image,
                               const gchar *undo_desc)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility.
   */

  undo = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                               GIMP_UNDO_CANT, undo_desc,
                               GIMP_DIRTY_ALL,
                               NULL);

  if (undo)
    g_signal_connect (undo, "pop",
                      G_CALLBACK (undo_pop_cantundo),
                      NULL);

  return undo;
}
