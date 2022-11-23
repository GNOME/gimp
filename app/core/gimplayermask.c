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

#include "core-types.h"

#include "gegl/ligma-babl.h"

#include "ligmaerror.h"
#include "ligmaimage.h"
#include "ligmalayer.h"
#include "ligmalayermask.h"

#include "ligma-intl.h"


static void            ligma_layer_mask_preview_freeze       (LigmaViewable      *viewable);
static void            ligma_layer_mask_preview_thaw         (LigmaViewable      *viewable);

static gboolean        ligma_layer_mask_is_attached          (LigmaItem          *item);
static gboolean        ligma_layer_mask_is_content_locked    (LigmaItem          *item,
                                                             LigmaItem         **locked_item);
static gboolean        ligma_layer_mask_is_position_locked   (LigmaItem          *item,
                                                             LigmaItem         **locked_item,
                                                             gboolean           check_children);
static LigmaItemTree  * ligma_layer_mask_get_tree             (LigmaItem          *item);
static LigmaItem      * ligma_layer_mask_duplicate            (LigmaItem          *item,
                                                             GType              new_type);
static gboolean        ligma_layer_mask_rename               (LigmaItem          *item,
                                                             const gchar       *new_name,
                                                             const gchar       *undo_desc,
                                                             GError           **error);

static void            ligma_layer_mask_bounding_box_changed (LigmaDrawable      *drawable);
static void            ligma_layer_mask_convert_type         (LigmaDrawable      *drawable,
                                                             LigmaImage         *dest_image,
                                                             const Babl        *new_format,
                                                             LigmaColorProfile  *src_profile,
                                                             LigmaColorProfile  *dest_profile,
                                                             GeglDitherMethod   layer_dither_type,
                                                             GeglDitherMethod   mask_dither_type,
                                                             gboolean           push_undo,
                                                             LigmaProgress      *progress);


G_DEFINE_TYPE (LigmaLayerMask, ligma_layer_mask, LIGMA_TYPE_CHANNEL)

#define parent_class ligma_layer_mask_parent_class


static void
ligma_layer_mask_class_init (LigmaLayerMaskClass *klass)
{
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);
  LigmaItemClass     *item_class     = LIGMA_ITEM_CLASS (klass);
  LigmaDrawableClass *drawable_class = LIGMA_DRAWABLE_CLASS (klass);

  viewable_class->default_icon_name = "ligma-layer-mask";

  viewable_class->preview_freeze       = ligma_layer_mask_preview_freeze;
  viewable_class->preview_thaw         = ligma_layer_mask_preview_thaw;

  item_class->is_attached              = ligma_layer_mask_is_attached;
  item_class->is_content_locked        = ligma_layer_mask_is_content_locked;
  item_class->is_position_locked       = ligma_layer_mask_is_position_locked;
  item_class->get_tree                 = ligma_layer_mask_get_tree;
  item_class->duplicate                = ligma_layer_mask_duplicate;
  item_class->rename                   = ligma_layer_mask_rename;
  item_class->translate_desc           = C_("undo-type", "Move Layer Mask");
  item_class->to_selection_desc        = C_("undo-type", "Layer Mask to Selection");

  drawable_class->bounding_box_changed = ligma_layer_mask_bounding_box_changed;
  drawable_class->convert_type         = ligma_layer_mask_convert_type;
}

static void
ligma_layer_mask_init (LigmaLayerMask *layer_mask)
{
  layer_mask->layer = NULL;
}

static void
ligma_layer_mask_preview_freeze (LigmaViewable *viewable)
{
  LigmaLayerMask *mask  = LIGMA_LAYER_MASK (viewable);
  LigmaLayer     *layer = ligma_layer_mask_get_layer (mask);

  if (layer)
    {
      LigmaViewable *parent = ligma_viewable_get_parent (LIGMA_VIEWABLE (layer));

      if (! parent && ligma_item_is_attached (LIGMA_ITEM (layer)))
        parent = LIGMA_VIEWABLE (ligma_item_get_image (LIGMA_ITEM (layer)));

      if (parent)
        ligma_viewable_preview_freeze (parent);
    }
}

static void
ligma_layer_mask_preview_thaw (LigmaViewable *viewable)
{
  LigmaLayerMask *mask  = LIGMA_LAYER_MASK (viewable);
  LigmaLayer     *layer = ligma_layer_mask_get_layer (mask);

  if (layer)
    {
      LigmaViewable *parent = ligma_viewable_get_parent (LIGMA_VIEWABLE (layer));

      if (! parent && ligma_item_is_attached (LIGMA_ITEM (layer)))
        parent = LIGMA_VIEWABLE (ligma_item_get_image (LIGMA_ITEM (layer)));

      if (parent)
        ligma_viewable_preview_thaw (parent);
    }
}

static gboolean
ligma_layer_mask_is_content_locked (LigmaItem  *item,
                                   LigmaItem **locked_item)
{
  LigmaLayerMask *mask  = LIGMA_LAYER_MASK (item);
  LigmaLayer     *layer = ligma_layer_mask_get_layer (mask);

  if (layer)
    return ligma_item_is_content_locked (LIGMA_ITEM (layer), locked_item);

  return FALSE;
}

static gboolean
ligma_layer_mask_is_position_locked (LigmaItem  *item,
                                    LigmaItem **locked_item,
                                    gboolean   check_children)
{
  LigmaLayerMask *mask  = LIGMA_LAYER_MASK (item);
  LigmaLayer     *layer = ligma_layer_mask_get_layer (mask);

  if (layer)
    return ligma_item_is_position_locked (LIGMA_ITEM (layer), locked_item);

  return FALSE;
}

static gboolean
ligma_layer_mask_is_attached (LigmaItem *item)
{
  LigmaLayerMask *mask  = LIGMA_LAYER_MASK (item);
  LigmaLayer     *layer = ligma_layer_mask_get_layer (mask);

  return (LIGMA_IS_IMAGE (ligma_item_get_image (item)) &&
          LIGMA_IS_LAYER (layer)                      &&
          ligma_layer_get_mask (layer) == mask        &&
          ligma_item_is_attached (LIGMA_ITEM (layer)));
}

static LigmaItemTree *
ligma_layer_mask_get_tree (LigmaItem *item)
{
  return NULL;
}

static LigmaItem *
ligma_layer_mask_duplicate (LigmaItem *item,
                           GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_DRAWABLE), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  return new_item;
}

static gboolean
ligma_layer_mask_rename (LigmaItem     *item,
                        const gchar  *new_name,
                        const gchar  *undo_desc,
                        GError      **error)
{
  /* reject renaming, layer masks are always named "<layer name> mask"  */

  g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
               _("Cannot rename layer masks."));

  return FALSE;
}

static void
ligma_layer_mask_bounding_box_changed (LigmaDrawable *drawable)
{
  LigmaLayerMask *mask  = LIGMA_LAYER_MASK (drawable);
  LigmaLayer     *layer = ligma_layer_mask_get_layer (mask);

  if (LIGMA_DRAWABLE_CLASS (parent_class)->bounding_box_changed)
    LIGMA_DRAWABLE_CLASS (parent_class)->bounding_box_changed (drawable);

  if (layer)
    ligma_drawable_update_bounding_box (LIGMA_DRAWABLE (layer));
}

static void
ligma_layer_mask_convert_type (LigmaDrawable      *drawable,
                              LigmaImage         *dest_image,
                              const Babl        *new_format,
                              LigmaColorProfile  *src_profile,
                              LigmaColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              LigmaProgress      *progress)
{
  new_format =
    ligma_babl_mask_format (ligma_babl_format_get_precision (new_format));

  LIGMA_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);
}

LigmaLayerMask *
ligma_layer_mask_new (LigmaImage     *image,
                     gint           width,
                     gint           height,
                     const gchar   *name,
                     const LigmaRGB *color)
{
  LigmaLayerMask *layer_mask;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (color != NULL, NULL);

  layer_mask =
    LIGMA_LAYER_MASK (ligma_drawable_new (LIGMA_TYPE_LAYER_MASK,
                                        image, name,
                                        0, 0, width, height,
                                        ligma_image_get_mask_format (image)));

  /*  set the layer_mask color and opacity  */
  ligma_channel_set_color (LIGMA_CHANNEL (layer_mask), color, FALSE);
  ligma_channel_set_show_masked (LIGMA_CHANNEL (layer_mask), TRUE);

  /*  selection mask variables  */
  LIGMA_CHANNEL (layer_mask)->x2 = width;
  LIGMA_CHANNEL (layer_mask)->y2 = height;

  return layer_mask;
}

void
ligma_layer_mask_set_layer (LigmaLayerMask *layer_mask,
                           LigmaLayer     *layer)
{
  g_return_if_fail (LIGMA_IS_LAYER_MASK (layer_mask));
  g_return_if_fail (layer == NULL || LIGMA_IS_LAYER (layer));

  layer_mask->layer = layer;

  if (layer)
    {
      gchar *mask_name;
      gint   offset_x;
      gint   offset_y;

      ligma_item_get_offset (LIGMA_ITEM (layer), &offset_x, &offset_y);
      ligma_item_set_offset (LIGMA_ITEM (layer_mask), offset_x, offset_y);

      mask_name = g_strdup_printf (_("%s mask"), ligma_object_get_name (layer));

      ligma_object_take_name (LIGMA_OBJECT (layer_mask), mask_name);
    }
}

LigmaLayer *
ligma_layer_mask_get_layer (LigmaLayerMask *layer_mask)
{
  g_return_val_if_fail (LIGMA_IS_LAYER_MASK (layer_mask), NULL);

  return layer_mask->layer;
}
