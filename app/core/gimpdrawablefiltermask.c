/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablefiltermask.c
 * Copyright (C) 2024 Jehan
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimpdrawablefilter.h"
#include "gimpdrawablefiltermask.h"
#include "gimperror.h"
#include "gimpimage.h"

#include "gimp-intl.h"


static gboolean       gimp_drawable_filter_mask_is_attached   (GimpItem            *item);
static GimpItemTree * gimp_drawable_filter_mask_get_tree      (GimpItem            *item);
static gboolean       gimp_drawable_filter_mask_rename        (GimpItem            *item,
                                                               const gchar         *new_name,
                                                               const gchar         *undo_desc,
                                                               GError             **error);

static void           gimp_drawable_filter_mask_convert_type  (GimpDrawable        *drawable,
                                                               GimpImage           *dest_image,
                                                               const Babl          *new_format,
                                                               GimpColorProfile    *src_profile,
                                                               GimpColorProfile    *dest_profile,
                                                               GeglDitherMethod     layer_dither_type,
                                                               GeglDitherMethod     mask_dither_type,
                                                               gboolean             push_undo,
                                                               GimpProgress        *progress);


G_DEFINE_TYPE (GimpDrawableFilterMask, gimp_drawable_filter_mask, GIMP_TYPE_CHANNEL)

#define parent_class gimp_drawable_filter_mask_parent_class


static void
gimp_drawable_filter_mask_class_init (GimpDrawableFilterMaskClass *klass)
{
  GimpItemClass     *item_class     = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class = GIMP_DRAWABLE_CLASS (klass);

  item_class->is_attached           = gimp_drawable_filter_mask_is_attached;
  item_class->get_tree              = gimp_drawable_filter_mask_get_tree;
  item_class->rename                = gimp_drawable_filter_mask_rename;

  drawable_class->convert_type      = gimp_drawable_filter_mask_convert_type;
}

static void
gimp_drawable_filter_mask_init (GimpDrawableFilterMask *drawable_filter_mask)
{
}

static gboolean
gimp_drawable_filter_mask_is_attached (GimpItem *item)
{
  GimpDrawableFilterMask *mask = GIMP_DRAWABLE_FILTER_MASK (item);
  GimpDrawable           *drawable;

  if (! mask->filter)
    return FALSE;

  drawable = gimp_drawable_filter_get_drawable (mask->filter);

  return (drawable                                     &&
          gimp_item_is_attached (GIMP_ITEM (drawable)) &&
          gimp_drawable_filter_get_mask (mask->filter) == mask);
}

static GimpItemTree *
gimp_drawable_filter_mask_get_tree (GimpItem *item)
{
  return NULL;
}

static gboolean
gimp_drawable_filter_mask_rename (GimpItem     *item,
                                  const gchar  *new_name,
                                  const gchar  *undo_desc,
                                  GError      **error)
{
  g_set_error (error, GIMP_ERROR, GIMP_FAILED,
               _("Cannot rename effect masks."));

  return FALSE;
}

static void
gimp_drawable_filter_mask_convert_type (GimpDrawable      *drawable,
                                        GimpImage         *dest_image,
                                        const Babl        *new_format,
                                        GimpColorProfile  *src_profile,
                                        GimpColorProfile  *dest_profile,
                                        GeglDitherMethod   layer_dither_type,
                                        GeglDitherMethod   mask_dither_type,
                                        gboolean           push_undo,
                                        GimpProgress      *progress)
{
  new_format =
    gimp_babl_mask_format (gimp_babl_format_get_precision (new_format));

  GIMP_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);
}


/*  public functions  */

GimpDrawableFilterMask *
gimp_drawable_filter_mask_new (GimpImage *image,
                               gint       width,
                               gint       height)
{
  GeglColor   *black = gegl_color_new ("black");
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  gimp_color_set_alpha (black, 0.5);
  channel = GIMP_CHANNEL (gimp_drawable_new (GIMP_TYPE_DRAWABLE_FILTER_MASK,
                                             image, NULL,
                                             0, 0, width, height,
                                             gimp_image_get_mask_format (image)));

  gimp_channel_set_color (channel, black, FALSE);
  gimp_channel_set_show_masked (channel, TRUE);

  channel->x2 = width;
  channel->y2 = height;

  g_object_unref (black);

  return GIMP_DRAWABLE_FILTER_MASK (channel);
}

void
gimp_drawable_filter_mask_set_filter (GimpDrawableFilterMask *mask,
                                      GimpDrawableFilter     *filter)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER_MASK (mask));
  g_return_if_fail (filter == NULL || GIMP_IS_DRAWABLE_FILTER (filter));

  mask->filter = filter;

  if (filter)
    {
      GimpDrawable *drawable;
      gchar        *mask_name;
      gint          offset_x;
      gint          offset_y;

      drawable = gimp_drawable_filter_get_drawable (mask->filter);

      if (drawable)
        {
          gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);
          gimp_item_set_offset (GIMP_ITEM (mask), offset_x, offset_y);
        }

      mask_name = g_strdup_printf (_("%s mask"), gimp_object_get_name (filter));

      gimp_object_take_name (GIMP_OBJECT (mask), mask_name);
    }
}

GimpDrawableFilter *
gimp_drawable_filter_mask_get_filter (GimpDrawableFilterMask *mask)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER_MASK (mask), NULL);

  return mask->filter;
}
