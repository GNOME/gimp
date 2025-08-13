/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball, Peter Mattis, and others
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

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimp-transform-resize.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-transform.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-selection.h"
#include "gimplayer-new.h"
#include "gimppickable.h"
#include "gimpprogress.h"
#include "gimpselection.h"

#include "gimp-intl.h"


/*  public functions  */

GeglBuffer *
gimp_drawable_transform_buffer_affine (GimpDrawable            *drawable,
                                       GimpContext             *context,
                                       GeglBuffer              *orig_buffer,
                                       gint                     orig_offset_x,
                                       gint                     orig_offset_y,
                                       const GimpMatrix3       *matrix,
                                       GimpTransformDirection   direction,
                                       GimpInterpolationType    interpolation_type,
                                       GimpTransformResize      clip_result,
                                       GimpColorProfile       **buffer_profile,
                                       gint                    *new_offset_x,
                                       gint                    *new_offset_y,
                                       GimpProgress            *progress)
{
  GeglBuffer  *new_buffer;
  GimpMatrix3  m;
  gint         u1, v1, u2, v2;  /* source bounding box */
  gint         x1, y1, x2, y2;  /* target bounding box */
  GimpMatrix3  gegl_matrix;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (orig_buffer), NULL);
  g_return_val_if_fail (matrix != NULL, NULL);
  g_return_val_if_fail (buffer_profile != NULL, NULL);
  g_return_val_if_fail (new_offset_x != NULL, NULL);
  g_return_val_if_fail (new_offset_y != NULL, NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  *buffer_profile =
    gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (drawable));

  m = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    {
      /*  Find the inverse of the transformation matrix  */
      gimp_matrix3_invert (&m);
    }

  u1 = orig_offset_x;
  v1 = orig_offset_y;
  u2 = u1 + gegl_buffer_get_width  (orig_buffer);
  v2 = v1 + gegl_buffer_get_height (orig_buffer);

  /*  Find the bounding coordinates of target */
  gimp_transform_resize_boundary (&m, clip_result,
                                  u1, v1, u2, v2,
                                  &x1, &y1, &x2, &y2);

  /*  Get the new temporary buffer for the transformed result  */
  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, x2 - x1, y2 - y1),
                                gegl_buffer_get_format (orig_buffer));

  gimp_matrix3_identity (&gegl_matrix);
  gimp_matrix3_translate (&gegl_matrix, u1, v1);
  gimp_matrix3_mult (&m, &gegl_matrix);
  gimp_matrix3_translate (&gegl_matrix, -x1, -y1);

  gimp_gegl_apply_transform (orig_buffer, progress, NULL,
                             new_buffer,
                             interpolation_type,
                             &gegl_matrix);

  *new_offset_x = x1;
  *new_offset_y = y1;

  return new_buffer;
}

GeglBuffer *
gimp_drawable_transform_buffer_flip (GimpDrawable         *drawable,
                                     GimpContext          *context,
                                     GeglBuffer           *orig_buffer,
                                     gint                  orig_offset_x,
                                     gint                  orig_offset_y,
                                     GimpOrientationType   flip_type,
                                     gdouble               axis,
                                     gboolean              clip_result,
                                     GimpColorProfile    **buffer_profile,
                                     gint                 *new_offset_x,
                                     gint                 *new_offset_y)
{
  const Babl         *format;
  GeglBuffer         *new_buffer;
  GeglBufferIterator *iter;
  GeglRectangle       src_rect;
  GeglRectangle       dest_rect;
  gint                bpp;
  gint                orig_x, orig_y;
  gint                orig_width, orig_height;
  gint                new_x, new_y;
  gint                new_width, new_height;
  gint                x, y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (orig_buffer), NULL);
  g_return_val_if_fail (buffer_profile != NULL, NULL);
  g_return_val_if_fail (new_offset_x != NULL, NULL);
  g_return_val_if_fail (new_offset_y != NULL, NULL);

  *buffer_profile =
    gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (drawable));

  orig_x      = orig_offset_x;
  orig_y      = orig_offset_y;
  orig_width  = gegl_buffer_get_width (orig_buffer);
  orig_height = gegl_buffer_get_height (orig_buffer);

  new_x      = orig_x;
  new_y      = orig_y;
  new_width  = orig_width;
  new_height = orig_height;

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      new_x = RINT (-((gdouble) orig_x +
                      (gdouble) orig_width - axis) + axis);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      new_y = RINT (-((gdouble) orig_y +
                      (gdouble) orig_height - axis) + axis);
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      g_return_val_if_reached (NULL);
      break;
    }

  format = gegl_buffer_get_format (orig_buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                new_width, new_height),
                                format);

  if (clip_result && (new_x != orig_x || new_y != orig_y))
    {
      GeglColor *color = NULL;
      gint       clip_x, clip_y;
      gint       clip_width, clip_height;

      *new_offset_x = orig_x;
      *new_offset_y = orig_y;

      /*  Use transparency, rather than the bg color, as the "outside" color of
       *  channels, and drawables with an alpha channel.
       */
      if (GIMP_IS_CHANNEL (drawable) || babl_format_has_alpha (format))
        {
          color = gegl_color_new ("black");
          gegl_color_set_rgba_with_space (color, 0.0, 0.0, 0.0, 0.0,
                                          gimp_drawable_get_space (drawable));
        }
      else
        {
          color = gegl_color_duplicate (gimp_context_get_background (context));
        }

      gegl_buffer_set_color (new_buffer, NULL, color);
      g_object_unref (color);

      if (gimp_rectangle_intersect (orig_x, orig_y, orig_width, orig_height,
                                    new_x, new_y, new_width, new_height,
                                    &clip_x, &clip_y,
                                    &clip_width, &clip_height))
        {
          orig_x = new_x = clip_x - orig_x;
          orig_y = new_y = clip_y - orig_y;
        }

      orig_width  = new_width  = clip_width;
      orig_height = new_height = clip_height;
    }
  else
    {
      *new_offset_x = new_x;
      *new_offset_y = new_y;

      orig_x = 0;
      orig_y = 0;
      new_x  = 0;
      new_y  = 0;
    }

  if (new_width == 0 && new_height == 0)
    return new_buffer;

  dest_rect.x      = new_x;
  dest_rect.y      = new_y;
  dest_rect.width  = new_width;
  dest_rect.height = new_height;

  iter = gegl_buffer_iterator_new (new_buffer, &dest_rect, 0, NULL,
                                   GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE, 1);

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      while (gegl_buffer_iterator_next (iter))
        {
          gint stride = iter->items[0].roi.width * bpp;

          src_rect = iter->items[0].roi;

          src_rect.x = (orig_x + orig_width)          -
                       (iter->items[0].roi.x - dest_rect.x) -
                       iter->items[0].roi.width;

          gegl_buffer_get (orig_buffer, &src_rect, 1.0, NULL, iter->items[0].data,
                           stride, GEGL_ABYSS_NONE);

          for (y = 0; y < iter->items[0].roi.height; y++)
            {
              guint8 *left  = iter->items[0].data;
              guint8 *right = iter->items[0].data;

              left  += y * stride;
              right += y * stride + (iter->items[0].roi.width - 1) * bpp;

              for (x = 0; x < iter->items[0].roi.width / 2; x++)
                {
                  guint8 temp[bpp];

                  memcpy (temp,  left,  bpp);
                  memcpy (left,  right, bpp);
                  memcpy (right, temp,  bpp);

                  left  += bpp;
                  right -= bpp;
                }
            }
        }
      break;

    case GIMP_ORIENTATION_VERTICAL:
      while (gegl_buffer_iterator_next (iter))
        {
          gint stride = iter->items[0].roi.width * bpp;

          src_rect = iter->items[0].roi;

          src_rect.y = (orig_y + orig_height)         -
                       (iter->items[0].roi.y - dest_rect.y) -
                       iter->items[0].roi.height;

          gegl_buffer_get (orig_buffer, &src_rect, 1.0, NULL, iter->items[0].data,
                           stride, GEGL_ABYSS_NONE);

          for (x = 0; x < iter->items[0].roi.width; x++)
            {
              guint8 *top    = iter->items[0].data;
              guint8 *bottom = iter->items[0].data;

              top    += x * bpp;
              bottom += x * bpp + (iter->items[0].roi.height - 1) * stride;

              for (y = 0; y < iter->items[0].roi.height / 2; y++)
                {
                  guint8 temp[bpp];

                  memcpy (temp,   top,    bpp);
                  memcpy (top,    bottom, bpp);
                  memcpy (bottom, temp,   bpp);

                  top    += stride;
                  bottom -= stride;
                }
            }
        }
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      gegl_buffer_iterator_stop (iter);
      break;
    }

  return new_buffer;
}

static void
gimp_drawable_transform_rotate_point (gint              x,
                                      gint              y,
                                      GimpRotationType  rotate_type,
                                      gdouble           center_x,
                                      gdouble           center_y,
                                      gint             *new_x,
                                      gint             *new_y)
{
  g_return_if_fail (new_x != NULL);
  g_return_if_fail (new_y != NULL);

  switch (rotate_type)
    {
    case GIMP_ROTATE_DEGREES90:
      *new_x = RINT (center_x - (gdouble) y + center_y);
      *new_y = RINT (center_y + (gdouble) x - center_x);
      break;

    case GIMP_ROTATE_DEGREES180:
      *new_x = RINT (center_x - ((gdouble) x - center_x));
      *new_y = RINT (center_y - ((gdouble) y - center_y));
      break;

    case GIMP_ROTATE_DEGREES270:
      *new_x = RINT (center_x + (gdouble) y - center_y);
      *new_y = RINT (center_y - (gdouble) x + center_x);
      break;

    default:
      *new_x = x;
      *new_y = y;
      g_return_if_reached ();
    }
}

GeglBuffer *
gimp_drawable_transform_buffer_rotate (GimpDrawable      *drawable,
                                       GimpContext       *context,
                                       GeglBuffer        *orig_buffer,
                                       gint               orig_offset_x,
                                       gint               orig_offset_y,
                                       GimpRotationType   rotate_type,
                                       gdouble            center_x,
                                       gdouble            center_y,
                                       gboolean           clip_result,
                                       GimpColorProfile **buffer_profile,
                                       gint              *new_offset_x,
                                       gint              *new_offset_y)
{
  const Babl    *format;
  GeglBuffer    *new_buffer;
  GeglRectangle  src_rect;
  GeglRectangle  dest_rect;
  gint           orig_x, orig_y;
  gint           orig_width, orig_height;
  gint           orig_bpp;
  gint           new_x, new_y;
  gint           new_width, new_height;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (orig_buffer), NULL);
  g_return_val_if_fail (buffer_profile != NULL, NULL);
  g_return_val_if_fail (new_offset_x != NULL, NULL);
  g_return_val_if_fail (new_offset_y != NULL, NULL);

  *buffer_profile =
    gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (drawable));

  orig_x      = orig_offset_x;
  orig_y      = orig_offset_y;
  orig_width  = gegl_buffer_get_width (orig_buffer);
  orig_height = gegl_buffer_get_height (orig_buffer);
  orig_bpp    = babl_format_get_bytes_per_pixel (gegl_buffer_get_format (orig_buffer));

  switch (rotate_type)
    {
    case GIMP_ROTATE_DEGREES90:
      gimp_drawable_transform_rotate_point (orig_x,
                                            orig_y + orig_height,
                                            rotate_type, center_x, center_y,
                                            &new_x, &new_y);
      new_width  = orig_height;
      new_height = orig_width;
      break;

    case GIMP_ROTATE_DEGREES180:
      gimp_drawable_transform_rotate_point (orig_x + orig_width,
                                            orig_y + orig_height,
                                            rotate_type, center_x, center_y,
                                            &new_x, &new_y);
      new_width  = orig_width;
      new_height = orig_height;
      break;

    case GIMP_ROTATE_DEGREES270:
      gimp_drawable_transform_rotate_point (orig_x + orig_width,
                                            orig_y,
                                            rotate_type, center_x, center_y,
                                            &new_x, &new_y);
      new_width  = orig_height;
      new_height = orig_width;
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }

  format = gegl_buffer_get_format (orig_buffer);

  if (clip_result && (new_x != orig_x || new_y != orig_y ||
                      new_width != orig_width || new_height != orig_height))

    {
      GeglColor *color = NULL;
      gint       clip_x, clip_y;
      gint       clip_width, clip_height;

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    orig_width, orig_height),
                                    format);

      *new_offset_x = orig_x;
      *new_offset_y = orig_y;

      /*  Use transparency, rather than the bg color, as the "outside" color of
       *  channels, and drawables with an alpha channel.
       */
      if (GIMP_IS_CHANNEL (drawable) || babl_format_has_alpha (format))
        {
          color = gegl_color_new ("black");
          gegl_color_set_rgba_with_space (color, 0.0, 0.0, 0.0, 0.0,
                                          gimp_drawable_get_space (drawable));
        }
      else
        {
          color = gegl_color_duplicate (gimp_context_get_background (context));
        }

      gegl_buffer_set_color (new_buffer, NULL, color);
      g_object_unref (color);

      if (gimp_rectangle_intersect (orig_x, orig_y, orig_width, orig_height,
                                    new_x, new_y, new_width, new_height,
                                    &clip_x, &clip_y,
                                    &clip_width, &clip_height))
        {
          gint saved_orig_x = orig_x;
          gint saved_orig_y = orig_y;

          new_x = clip_x - orig_x;
          new_y = clip_y - orig_y;

          switch (rotate_type)
            {
            case GIMP_ROTATE_DEGREES90:
              gimp_drawable_transform_rotate_point (clip_x + clip_width,
                                                    clip_y,
                                                    GIMP_ROTATE_DEGREES270,
                                                    center_x,
                                                    center_y,
                                                    &orig_x,
                                                    &orig_y);
              orig_x      -= saved_orig_x;
              orig_y      -= saved_orig_y;
              orig_width   = clip_height;
              orig_height  = clip_width;
              break;

            case GIMP_ROTATE_DEGREES180:
              orig_x      = clip_x - orig_x;
              orig_y      = clip_y - orig_y;
              orig_width  = clip_width;
              orig_height = clip_height;
              break;

            case GIMP_ROTATE_DEGREES270:
              gimp_drawable_transform_rotate_point (clip_x,
                                                    clip_y + clip_height,
                                                    GIMP_ROTATE_DEGREES90,
                                                    center_x,
                                                    center_y,
                                                    &orig_x,
                                                    &orig_y);
              orig_x      -= saved_orig_x;
              orig_y      -= saved_orig_y;
              orig_width   = clip_height;
              orig_height  = clip_width;
              break;
            }

          new_width  = clip_width;
          new_height = clip_height;
        }
      else
        {
          new_width  = 0;
          new_height = 0;
        }
    }
  else
    {
      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    new_width, new_height),
                                    format);

      *new_offset_x = new_x;
      *new_offset_y = new_y;

      orig_x = 0;
      orig_y = 0;
      new_x  = 0;
      new_y  = 0;
    }

  if (new_width < 1 || new_height < 1)
    return new_buffer;

  src_rect.x      = orig_x;
  src_rect.y      = orig_y;
  src_rect.width  = orig_width;
  src_rect.height = orig_height;

  dest_rect.x      = new_x;
  dest_rect.y      = new_y;
  dest_rect.width  = new_width;
  dest_rect.height = new_height;

  switch (rotate_type)
    {
    case GIMP_ROTATE_DEGREES90:
      {
        guchar *buf = g_new (guchar, new_height * orig_bpp);
        gint    i;

        /* Not cool, we leak memory if we return, but anyway that is
         * never supposed to happen. If we see this warning, a bug has
         * to be fixed!
         */
        g_return_val_if_fail (new_height == orig_width, NULL);

        src_rect.y      = orig_y + orig_height - 1;
        src_rect.height = 1;

        dest_rect.x     = new_x;
        dest_rect.width = 1;

        for (i = 0; i < orig_height; i++)
          {
            src_rect.y  = orig_y + orig_height - 1 - i;
            dest_rect.x = new_x + i;

            gegl_buffer_get (orig_buffer, &src_rect, 1.0, NULL, buf,
                             GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            gegl_buffer_set (new_buffer, &dest_rect, 0, NULL, buf,
                             GEGL_AUTO_ROWSTRIDE);
          }

        g_free (buf);
      }
      break;

    case GIMP_ROTATE_DEGREES180:
      {
        guchar *buf = g_new (guchar, new_width * orig_bpp);
        gint    i, j, k;

        /* Not cool, we leak memory if we return, but anyway that is
         * never supposed to happen. If we see this warning, a bug has
         * to be fixed!
         */
        g_return_val_if_fail (new_width == orig_width, NULL);

        src_rect.y      = orig_y + orig_height - 1;
        src_rect.height = 1;

        dest_rect.y      = new_y;
        dest_rect.height = 1;

        for (i = 0; i < orig_height; i++)
          {
            src_rect.y  = orig_y + orig_height - 1 - i;
            dest_rect.y = new_y + i;

            gegl_buffer_get (orig_buffer, &src_rect, 1.0, NULL, buf,
                             GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

            for (j = 0; j < orig_width / 2; j++)
              {
                guchar *left  = buf + j * orig_bpp;
                guchar *right = buf + (orig_width - 1 - j) * orig_bpp;

                for (k = 0; k < orig_bpp; k++)
                  {
                    guchar tmp = left[k];
                    left[k]    = right[k];
                    right[k]   = tmp;
                  }
              }

            gegl_buffer_set (new_buffer, &dest_rect, 0, NULL, buf,
                             GEGL_AUTO_ROWSTRIDE);
          }

        g_free (buf);
      }
      break;

    case GIMP_ROTATE_DEGREES270:
      {
        guchar *buf = g_new (guchar, new_width * orig_bpp);
        gint    i;

        /* Not cool, we leak memory if we return, but anyway that is
         * never supposed to happen. If we see this warning, a bug has
         * to be fixed!
         */
        g_return_val_if_fail (new_width == orig_height, NULL);

        src_rect.x     = orig_x + orig_width - 1;
        src_rect.width = 1;

        dest_rect.y      = new_y;
        dest_rect.height = 1;

        for (i = 0; i < orig_width; i++)
          {
            src_rect.x  = orig_x + orig_width - 1 - i;
            dest_rect.y = new_y + i;

            gegl_buffer_get (orig_buffer, &src_rect, 1.0, NULL, buf,
                             GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            gegl_buffer_set (new_buffer, &dest_rect, 0, NULL, buf,
                             GEGL_AUTO_ROWSTRIDE);
          }

        g_free (buf);
      }
      break;
    }

  return new_buffer;
}

GimpDrawable *
gimp_drawable_transform_affine (GimpDrawable           *drawable,
                                GimpContext            *context,
                                const GimpMatrix3      *matrix,
                                GimpTransformDirection  direction,
                                GimpInterpolationType   interpolation_type,
                                GimpTransformResize     clip_result,
                                GimpProgress           *progress)
{
  GimpImage    *image;
  GList        *drawables;
  GeglBuffer   *orig_buffer;
  gint          orig_offset_x;
  gint          orig_offset_y;
  gboolean      new_layer;
  GimpDrawable *result = NULL;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (matrix != NULL, NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* Start a transform undo group */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_TRANSFORM,
                               C_("undo-type", "Transform"));

  /* Cut/Copy from the specified drawable */
  drawables   = g_list_prepend (NULL, drawable);
  orig_buffer = gimp_drawable_transform_cut (drawables, context,
                                             &orig_offset_x, &orig_offset_y,
                                             &new_layer);
  g_free (drawables);

  if (orig_buffer)
    {
      GeglBuffer       *new_buffer;
      gint              new_offset_x;
      gint              new_offset_y;
      GimpColorProfile *profile;

      /*  also transform the mask if we are transforming an entire layer  */
      if (GIMP_IS_LAYER (drawable) &&
          gimp_layer_get_mask (GIMP_LAYER (drawable)) &&
          gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (drawable));

          gimp_item_transform (GIMP_ITEM (mask), context,
                               matrix,
                               direction,
                               interpolation_type,
                               clip_result,
                               progress);
        }

      /* transform the buffer */
      new_buffer = gimp_drawable_transform_buffer_affine (drawable, context,
                                                          orig_buffer,
                                                          orig_offset_x,
                                                          orig_offset_y,
                                                          matrix,
                                                          direction,
                                                          interpolation_type,
                                                          clip_result,
                                                          &profile,
                                                          &new_offset_x,
                                                          &new_offset_y,
                                                          progress);

      /* Free the cut/copied buffer */
      g_object_unref (orig_buffer);

      if (new_buffer)
        {
          result = gimp_drawable_transform_paste (drawable, new_buffer, profile,
                                                  new_offset_x, new_offset_y,
                                                  new_layer, TRUE);
          g_object_unref (new_buffer);
        }
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (image);

  return result;
}

GimpDrawable *
gimp_drawable_transform_flip (GimpDrawable        *drawable,
                              GimpContext         *context,
                              GimpOrientationType  flip_type,
                              gdouble              axis,
                              gboolean             clip_result)
{
  GimpImage    *image;
  GList        *drawables;
  GeglBuffer   *orig_buffer;
  gint          orig_offset_x;
  gint          orig_offset_y;
  gboolean      new_layer;
  GimpDrawable *result = NULL;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* Start a transform undo group */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_TRANSFORM,
                               C_("undo-type", "Flip"));

  /* Cut/Copy from the specified drawable */
  drawables   = g_list_prepend (NULL, drawable);
  orig_buffer = gimp_drawable_transform_cut (drawables, context,
                                             &orig_offset_x, &orig_offset_y,
                                             &new_layer);
  g_free (drawables);

  if (orig_buffer)
    {
      GeglBuffer       *new_buffer;
      gint              new_offset_x;
      gint              new_offset_y;
      GimpColorProfile *profile;

      /*  also transform the mask if we are transforming an entire layer  */
      if (GIMP_IS_LAYER (drawable) &&
          gimp_layer_get_mask (GIMP_LAYER (drawable)) &&
          gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (drawable));

          gimp_item_flip (GIMP_ITEM (mask), context,
                          flip_type,
                          axis,
                          clip_result);
        }

      /* transform the buffer */
      new_buffer = gimp_drawable_transform_buffer_flip (drawable, context,
                                                        orig_buffer,
                                                        orig_offset_x,
                                                        orig_offset_y,
                                                        flip_type, axis,
                                                        clip_result,
                                                        &profile,
                                                        &new_offset_x,
                                                        &new_offset_y);

      /* Free the cut/copied buffer */
      g_object_unref (orig_buffer);

      if (new_buffer)
        {
          result = gimp_drawable_transform_paste (drawable, new_buffer, profile,
                                                  new_offset_x, new_offset_y,
                                                  new_layer, TRUE);
          g_object_unref (new_buffer);
        }
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (image);

  return result;
}

GimpDrawable *
gimp_drawable_transform_rotate (GimpDrawable     *drawable,
                                GimpContext      *context,
                                GimpRotationType  rotate_type,
                                gdouble           center_x,
                                gdouble           center_y,
                                gboolean          clip_result)
{
  GimpImage    *image;
  GList        *drawables;
  GeglBuffer   *orig_buffer;
  gint          orig_offset_x;
  gint          orig_offset_y;
  gboolean      new_layer;
  GimpDrawable *result = NULL;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* Start a transform undo group */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_TRANSFORM,
                               C_("undo-type", "Rotate"));

  /* Cut/Copy from the specified drawable */
  drawables   = g_list_prepend (NULL, drawable);
  orig_buffer = gimp_drawable_transform_cut (drawables, context,
                                             &orig_offset_x, &orig_offset_y,
                                             &new_layer);
  g_free (drawables);

  if (orig_buffer)
    {
      GeglBuffer       *new_buffer;
      gint              new_offset_x;
      gint              new_offset_y;
      GimpColorProfile *profile;

      /*  also transform the mask if we are transforming an entire layer  */
      if (GIMP_IS_LAYER (drawable) &&
          gimp_layer_get_mask (GIMP_LAYER (drawable)) &&
          gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (drawable));

          gimp_item_rotate (GIMP_ITEM (mask), context,
                            rotate_type,
                            center_x,
                            center_y,
                            clip_result);
        }

      /* transform the buffer */
      new_buffer = gimp_drawable_transform_buffer_rotate (drawable, context,
                                                          orig_buffer,
                                                          orig_offset_x,
                                                          orig_offset_y,
                                                          rotate_type,
                                                          center_x, center_y,
                                                          clip_result,
                                                          &profile,
                                                          &new_offset_x,
                                                          &new_offset_y);

      /* Free the cut/copied buffer */
      g_object_unref (orig_buffer);

      if (new_buffer)
        {
          result = gimp_drawable_transform_paste (drawable, new_buffer, profile,
                                                  new_offset_x, new_offset_y,
                                                  new_layer, TRUE);
          g_object_unref (new_buffer);
        }
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (image);

  return result;
}

GeglBuffer *
gimp_drawable_transform_cut (GList        *drawables,
                             GimpContext  *context,
                             gint         *offset_x,
                             gint         *offset_y,
                             gboolean     *new_layer)
{
  GimpImage  *image  = NULL;
  GeglBuffer *buffer = NULL;
  GList      *iter;
  gboolean    drawables_are_layers = FALSE;

  g_return_val_if_fail (g_list_length (drawables), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (offset_x != NULL, NULL);
  g_return_val_if_fail (offset_y != NULL, NULL);
  g_return_val_if_fail (new_layer != NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (gimp_item_is_attached (iter->data), NULL);

      if (! image)
        image = gimp_item_get_image (iter->data);
      else
        g_return_val_if_fail (image == gimp_item_get_image (iter->data), NULL);

      if (drawables_are_layers)
        g_return_val_if_fail (GIMP_IS_LAYER (iter->data), NULL);
      else if (GIMP_IS_LAYER (iter->data))
        drawables_are_layers = TRUE;
    }

  /*  extract the selected mask if there is a selection  */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      gint x, y, w, h;

      /* set the keep_indexed flag to FALSE here, since we use
       * gimp_layer_new_from_gegl_buffer() later which assumes that
       * the buffer are either RGB or GRAY.  Eeek!!!  (Sven)
       */
      if (gimp_image_mask_intersect (image, drawables, &x, &y, &w, &h))
        {
          buffer = gimp_selection_extract (GIMP_SELECTION (gimp_image_get_mask (image)),
                                           drawables, context,
                                           TRUE, FALSE, TRUE,
                                           offset_x, offset_y,
                                           NULL);
          /*  clear the selection  */
          gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);

          *new_layer = TRUE;
        }
      else
        {
          buffer = NULL;
          *new_layer = FALSE;
        }
    }
  else  /*  otherwise, just copy the layer  */
    {
      buffer = gimp_selection_extract (GIMP_SELECTION (gimp_image_get_mask (image)),
                                       drawables, context,
                                       FALSE, TRUE,
                                       drawables_are_layers,
                                       offset_x, offset_y,
                                       NULL);

      *new_layer = FALSE;
    }

  return buffer;
}

GimpDrawable *
gimp_drawable_transform_paste (GimpDrawable     *drawable,
                               GeglBuffer       *buffer,
                               GimpColorProfile *buffer_profile,
                               gint              offset_x,
                               gint              offset_y,
                               gboolean          new_layer,
                               gboolean          push_undo)
{
  GimpImage *image;
  GimpLayer *layer = NULL;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (buffer_profile), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (push_undo)
    {
      const gchar *undo_desc = NULL;

      if (GIMP_IS_LAYER (drawable))
        undo_desc = C_("undo-type", "Transform Layer");
      else if (GIMP_IS_CHANNEL (drawable))
        undo_desc = C_("undo-type", "Transform Channel");
      else
        return NULL;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE, undo_desc);
    }

  if (new_layer)
    {
      layer =
        gimp_layer_new_from_gegl_buffer (buffer, image,
                                         gimp_drawable_get_format_with_alpha (drawable),
                                         _("Transformation"),
                                         GIMP_OPACITY_OPAQUE,
                                         gimp_image_get_default_new_layer_mode (image),
                                         buffer_profile);

      gimp_item_set_offset (GIMP_ITEM (layer), offset_x, offset_y);

      floating_sel_attach (layer, drawable);

      drawable = GIMP_DRAWABLE (layer);
    }
  else
    {
      gimp_drawable_set_buffer_full (drawable, push_undo, NULL,
                                     buffer,
                                     GEGL_RECTANGLE (offset_x, offset_y, 0, 0),
                                     TRUE);
    }

  if (push_undo)
    gimp_image_undo_group_end (image);

  return drawable;
}
