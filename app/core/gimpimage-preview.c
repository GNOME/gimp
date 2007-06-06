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

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpdrawable-preview.h"
#include "gimpimage.h"
#include "gimpimage-preview.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplist.h"


void
gimp_image_get_preview_size (GimpViewable *viewable,
                             gint          size,
                             gboolean      is_popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (! image->gimp->config->layer_previews && ! is_popup)
    {
      *width  = size;
      *height = size;
      return;
    }

  gimp_viewable_calc_preview_size (image->width,
                                   image->height,
                                   size,
                                   size,
                                   dot_for_dot,
                                   image->xresolution,
                                   image->yresolution,
                                   width,
                                   height,
                                   NULL);
}

gboolean
gimp_image_get_popup_size (GimpViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (! image->gimp->config->layer_previews)
    return FALSE;

  if (image->width > width || image->height > height)
    {
      gboolean scaling_up;

      gimp_viewable_calc_preview_size (image->width,
                                       image->height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = image->width;
          *popup_height = image->height;
        }

      return TRUE;
    }

  return FALSE;
}

TempBuf *
gimp_image_get_preview (GimpViewable *viewable,
                        GimpContext  *context,
                        gint          width,
                        gint          height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (! image->gimp->config->layer_previews)
    return NULL;

  if (image->comp_preview_valid            &&
      image->comp_preview->width  == width &&
      image->comp_preview->height == height)
    {
      /*  The easy way  */
      return image->comp_preview;
    }
  else
    {
      /*  The hard way  */
      if (image->comp_preview)
        temp_buf_free (image->comp_preview);

      /*  Actually construct the composite preview from the layer previews!
       *  This might seem ridiculous, but it's actually the best way, given
       *  a number of unsavory alternatives.
       */
      image->comp_preview = gimp_image_get_new_preview (viewable, context,
                                                        width, height);

      image->comp_preview_valid = TRUE;

      return image->comp_preview;
    }
}

TempBuf *
gimp_image_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpImage   *image          = GIMP_IMAGE (viewable);
  GimpLayer   *floating_sel   = NULL;
  TempBuf     *comp;
  GList       *list;
  GSList      *reverse_list   = NULL;
  gdouble      ratio;
  gint         bytes;
  gboolean     construct_flag = FALSE;
  gboolean     visible_components[MAX_CHANNELS] = { TRUE, TRUE, TRUE, TRUE };

  if (! image->gimp->config->layer_previews)
    return NULL;

  ratio = (gdouble) width / (gdouble) image->width;

  switch (gimp_image_base_type (image))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      bytes = 4;
      break;
    case GIMP_GRAY:
      bytes = 2;
      break;
    default:
      bytes = 0;
      g_assert_not_reached ();
      break;
    }

  /*  The construction buffer  */
  comp = temp_buf_new (width, height, bytes, 0, 0, NULL);
  temp_buf_data_clear (comp);

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      /*  only add layers that are visible to the list  */
      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        {
          /*  floating selections are added right above the layer
           *  they are attached to
           */
          if (gimp_layer_is_floating_sel (layer))
            {
              floating_sel = layer;
            }
          else
            {
              if (floating_sel &&
                  floating_sel->fs.drawable == GIMP_DRAWABLE (layer))
                {
                  reverse_list = g_slist_prepend (reverse_list, floating_sel);
                }

              reverse_list = g_slist_prepend (reverse_list, layer);
            }
        }
    }

  for (; reverse_list; reverse_list = g_slist_next (reverse_list))
    {
      GimpLayer   *layer = reverse_list->data;
      PixelRegion  src1PR, src2PR, maskPR;
      PixelRegion *mask;
      TempBuf     *layer_buf;
      TempBuf     *mask_buf;
      gint         x, y, w, h;
      gint         x1, y1, x2, y2;
      gint         src_x, src_y;
      gint         src_width, src_height;
      gint         off_x, off_y;
      gboolean     use_sub_preview = FALSE;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      if (! gimp_rectangle_intersect (0, 0,
                                      gimp_item_width  (GIMP_ITEM (layer)),
                                      gimp_item_height (GIMP_ITEM (layer)),
                                      -off_x, -off_y,
                                      image->width, image->height,
                                      &src_x, &src_y,
                                      &src_width, &src_height))
        {
          continue;
        }

      x = (gint) RINT (ratio * off_x);
      y = (gint) RINT (ratio * off_y);
      w = (gint) RINT (ratio * gimp_item_width  (GIMP_ITEM (layer)));
      h = (gint) RINT (ratio * gimp_item_height (GIMP_ITEM (layer)));

      if (w < 1 || h < 1)
        continue;

      if ((w * h) > (width * height * 4))
        use_sub_preview = TRUE;

      x1 = CLAMP (x, 0, width);
      y1 = CLAMP (y, 0, height);
      x2 = CLAMP (x + w, 0, width);
      y2 = CLAMP (y + h, 0, height);

      if (x2 == x1 || y2 == y1)
        continue;

      pixel_region_init_temp_buf (&src1PR, comp,
                                  x1, y1, x2 - x1, y2 - y1);

      if (use_sub_preview)
        {
          layer_buf = gimp_drawable_get_sub_preview (GIMP_DRAWABLE (layer),
                                                     src_x, src_y,
                                                     src_width, src_height,
                                                     (x2 - x1),
                                                     (y2 - y1));

          g_assert (layer_buf);
          g_assert (layer_buf->bytes <= comp->bytes);

          pixel_region_init_temp_buf (&src2PR, layer_buf,
                                      0, 0, src1PR.w, src1PR.h);
        }
      else
        {
          layer_buf = gimp_viewable_get_preview (GIMP_VIEWABLE (layer),
                                                 context, w, h);

          g_assert (layer_buf);
          g_assert (layer_buf->bytes <= comp->bytes);

          pixel_region_init_temp_buf (&src2PR, layer_buf,
                                      x1 - x, y1 - y, src1PR.w, src1PR.h);
        }

      if (layer->mask && layer->mask->apply_mask)
        {
          if (use_sub_preview)
            {
              mask_buf =
                gimp_drawable_get_sub_preview (GIMP_DRAWABLE (layer->mask),
                                               src_x, src_y,
                                               src_width, src_height,
                                               x2 - x1,
                                               y2 - y1);

              pixel_region_init_temp_buf (&maskPR, mask_buf,
                                          0, 0, src1PR.w, maskPR.h);
            }
          else
            {
              mask_buf = gimp_viewable_get_preview (GIMP_VIEWABLE (layer->mask),
                                                    context, w, h);

              pixel_region_init_temp_buf (&maskPR, mask_buf,
                                          x1 - x, y1 - y, src1PR.w, maskPR.h);
            }

          mask = &maskPR;
        }
      else
        {
          mask_buf = NULL;
          mask     = NULL;
        }

      /*  Based on the type of the layer, project the layer onto the
       *   composite preview...
       *  Indexed images are actually already converted to RGB and RGBA,
       *   so just project them as if they were type "intensity"
       *  Send in all TRUE for visible since that info doesn't matter
       *   for previews
       */
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          if (! construct_flag)
            initial_region (&src2PR, &src1PR,
                            mask, NULL,
                            layer->opacity * 255.999,
                            layer->mode,
                            visible_components,
                            INITIAL_INTENSITY_ALPHA);
          else
            combine_regions (&src1PR, &src2PR, &src1PR,
                             mask, NULL,
                             layer->opacity * 255.999,
                             layer->mode,
                             visible_components,
                             COMBINE_INTEN_A_INTEN_A);
        }
      else
        {
          if (! construct_flag)
            initial_region (&src2PR, &src1PR,
                            mask, NULL,
                            layer->opacity * 255.999,
                            layer->mode,
                            visible_components,
                            INITIAL_INTENSITY);
          else
            combine_regions (&src1PR, &src2PR, &src1PR,
                             mask, NULL,
                             layer->opacity * 255.999,
                             layer->mode,
                             visible_components,
                             COMBINE_INTEN_A_INTEN);
        }

      if (use_sub_preview)
        {
          temp_buf_free (layer_buf);

          if (mask_buf)
            temp_buf_free (mask_buf);
        }

      construct_flag = TRUE;
    }

  g_slist_free (reverse_list);

  return comp;
}
