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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayermask.h"

#include "gimp-intl.h"


static gboolean        gimp_layer_mask_is_attached        (const GimpItem    *item);
static gboolean        gimp_layer_mask_is_content_locked  (const GimpItem    *item);
static gboolean        gimp_layer_mask_is_position_locked (const GimpItem    *item);
static GimpItemTree  * gimp_layer_mask_get_tree           (GimpItem          *item);
static GimpItem      * gimp_layer_mask_duplicate          (GimpItem          *item,
                                                           GType              new_type);
static gboolean        gimp_layer_mask_rename             (GimpItem          *item,
                                                           const gchar       *new_name,
                                                           const gchar       *undo_desc,
                                                           GError           **error);

static void            gimp_layer_mask_convert_type       (GimpDrawable      *drawable,
                                                           GimpImage         *dest_image,
                                                           const Babl        *new_format,
                                                           GimpImageBaseType  new_base_type,
                                                           GimpPrecision      new_precision,
                                                           gint               layer_dither_type,
                                                           gint               mask_dither_type,
                                                           gboolean           push_undo);


G_DEFINE_TYPE (GimpLayerMask, gimp_layer_mask, GIMP_TYPE_CHANNEL)

#define parent_class gimp_layer_mask_parent_class


static void
gimp_layer_mask_class_init (GimpLayerMaskClass *klass)
{
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class     = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class = GIMP_DRAWABLE_CLASS (klass);

  viewable_class->default_icon_name = "gimp-layer-mask";

  item_class->is_attached        = gimp_layer_mask_is_attached;
  item_class->is_content_locked  = gimp_layer_mask_is_content_locked;
  item_class->is_position_locked = gimp_layer_mask_is_position_locked;
  item_class->get_tree           = gimp_layer_mask_get_tree;
  item_class->duplicate          = gimp_layer_mask_duplicate;
  item_class->rename             = gimp_layer_mask_rename;
  item_class->translate_desc     = C_("undo-type", "Move Layer Mask");
  item_class->to_selection_desc  = C_("undo-type", "Layer Mask to Selection");

  drawable_class->convert_type   = gimp_layer_mask_convert_type;
}

static void
gimp_layer_mask_init (GimpLayerMask *layer_mask)
{
  layer_mask->layer = NULL;
}

static gboolean
gimp_layer_mask_is_content_locked (const GimpItem *item)
{
  GimpLayerMask *mask  = GIMP_LAYER_MASK (item);
  GimpLayer     *layer = gimp_layer_mask_get_layer (mask);

  if (layer)
    return gimp_item_is_content_locked (GIMP_ITEM (layer));

  return FALSE;
}

static gboolean
gimp_layer_mask_is_position_locked (const GimpItem *item)
{
  GimpLayerMask *mask  = GIMP_LAYER_MASK (item);
  GimpLayer     *layer = gimp_layer_mask_get_layer (mask);

  if (layer)
    return gimp_item_is_position_locked (GIMP_ITEM (layer));

  return FALSE;
}

static gboolean
gimp_layer_mask_is_attached (const GimpItem *item)
{
  GimpLayerMask *mask  = GIMP_LAYER_MASK (item);
  GimpLayer     *layer = gimp_layer_mask_get_layer (mask);

  return (GIMP_IS_IMAGE (gimp_item_get_image (item)) &&
          GIMP_IS_LAYER (layer)                      &&
          gimp_layer_get_mask (layer) == mask        &&
          gimp_item_is_attached (GIMP_ITEM (layer)));
}

static GimpItemTree *
gimp_layer_mask_get_tree (GimpItem *item)
{
  return NULL;
}

static GimpItem *
gimp_layer_mask_duplicate (GimpItem *item,
                           GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  return new_item;
}

static gboolean
gimp_layer_mask_rename (GimpItem     *item,
                        const gchar  *new_name,
                        const gchar  *undo_desc,
                        GError      **error)
{
  /* reject renaming, layer masks are always named "<layer name> mask"  */

  g_set_error (error, GIMP_ERROR, GIMP_FAILED,
	       _("Cannot rename layer masks."));

  return FALSE;
}

static void
gimp_layer_mask_convert_type (GimpDrawable      *drawable,
                              GimpImage         *dest_image,
                              const Babl        *new_format,
                              GimpImageBaseType  new_base_type,
                              GimpPrecision      new_precision,
                              gint               layer_dither_type,
                              gint               mask_dither_type,
                              gboolean           push_undo)
{
  new_format = gimp_babl_mask_format (new_precision);

  GIMP_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    new_base_type,
                                                    new_precision,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo);
}

GimpLayerMask *
gimp_layer_mask_new (GimpImage     *image,
                     gint           width,
                     gint           height,
                     const gchar   *name,
                     const GimpRGB *color)
{
  GimpLayerMask *layer_mask;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (color != NULL, NULL);

  layer_mask =
    GIMP_LAYER_MASK (gimp_drawable_new (GIMP_TYPE_LAYER_MASK,
                                        image, name,
                                        0, 0, width, height,
                                        gimp_image_get_mask_format (image)));

  /*  set the layer_mask color and opacity  */
  gimp_channel_set_color (GIMP_CHANNEL (layer_mask), color, FALSE);
  gimp_channel_set_show_masked (GIMP_CHANNEL (layer_mask), TRUE);

  /*  selection mask variables  */
  GIMP_CHANNEL (layer_mask)->x2 = width;
  GIMP_CHANNEL (layer_mask)->y2 = height;

  return layer_mask;
}

GimpLayerMask *
gimp_layer_mask_new_from_buffer (GeglBuffer    *buffer,
                                 GimpImage     *image,
                                 const gchar   *name,
                                 const GimpRGB *color)
{
  GimpLayerMask *layer_mask;
  GeglBuffer    *dest;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  layer_mask = gimp_layer_mask_new (image,
                                    gegl_buffer_get_width  (buffer),
                                    gegl_buffer_get_height (buffer),
                                    name, color);

  dest = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer_mask));
  gegl_buffer_copy (buffer, NULL, dest, NULL);

  return layer_mask;
}

void
gimp_layer_mask_set_layer (GimpLayerMask *layer_mask,
                           GimpLayer     *layer)
{
  g_return_if_fail (GIMP_IS_LAYER_MASK (layer_mask));
  g_return_if_fail (layer == NULL || GIMP_IS_LAYER (layer));

  layer_mask->layer = layer;

  if (layer)
    {
      gchar *mask_name;
      gint   offset_x;
      gint   offset_y;

      gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);
      gimp_item_set_offset (GIMP_ITEM (layer_mask), offset_x, offset_y);

      mask_name = g_strdup_printf (_("%s mask"), gimp_object_get_name (layer));

      gimp_object_take_name (GIMP_OBJECT (layer_mask), mask_name);
    }
}

GimpLayer *
gimp_layer_mask_get_layer (const GimpLayerMask *layer_mask)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (layer_mask), NULL);

  return layer_mask->layer;
}
