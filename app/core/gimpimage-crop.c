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

#include <string.h>

#include <glib-object.h>

#include "core-types.h"

#include "base/pixel-region.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-crop.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "gimppickable.h"
#include "gimpsamplepoint.h"

#include "gimp-intl.h"


typedef enum
{
  AUTO_CROP_NOTHING = 0,
  AUTO_CROP_ALPHA   = 1,
  AUTO_CROP_COLOR   = 2
} AutoCropType;


typedef AutoCropType (* ColorsEqualFunc) (guchar *col1,
                                          guchar *col2,
                                          gint    bytes);


/*  local function prototypes  */

static AutoCropType gimp_image_crop_guess_bgcolor (GimpPickable *pickable,
                                                   gint          bytes,
                                                   gboolean      has_alpha,
                                                   guchar       *color,
                                                   gint          x1,
                                                   gint          x2,
                                                   gint          y1,
                                                   gint          y2);
static gint         gimp_image_crop_colors_equal  (guchar       *col1,
                                                   guchar       *col2,
                                                   gint          bytes);
static gint         gimp_image_crop_colors_alpha  (guchar       *col1,
                                                   guchar       *col2,
                                                   gint          bytes);


/*  public functions  */

void
gimp_image_crop (GimpImage   *image,
                 GimpContext *context,
                 gint         x1,
                 gint         y1,
                 gint         x2,
                 gint         y2,
                 gboolean     active_layer_only,
                 gboolean     crop_layers)
{
  gint width, height;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  width  = x2 - x1;
  height = y2 - y1;

  /*  Make sure new width and height are non-zero  */
  if (width < 1 || height < 1)
    return;

  gimp_set_busy (image->gimp);

  if (active_layer_only)
    {
      GimpLayer *layer;
      gint       off_x, off_y;

      layer = gimp_image_get_active_layer (image);

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      off_x -= x1;
      off_y -= y1;

      gimp_item_resize (GIMP_ITEM (layer), context, width, height, off_x, off_y);
    }
  else
    {
      GimpItem *item;
      GList    *list;

      g_object_freeze_notify (G_OBJECT (image));

      if (crop_layers)
        gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CROP,
                                     Q_("command|Crop Image"));
      else
        gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_RESIZE,
                                     _("Resize Image"));

      /*  Push the image size to the stack  */
      gimp_image_undo_push_image_size (image, NULL);

      /*  Set the new width and height  */
      g_object_set (image,
                    "width",  width,
                    "height", height,
                    NULL);

      /*  Resize all channels  */
      for (list = GIMP_LIST (image->channels)->list;
           list;
           list = g_list_next (list))
        {
          item = (GimpItem *) list->data;

          gimp_item_resize (item, context, width, height, -x1, -y1);
        }

      /*  Resize all vectors  */
      for (list = GIMP_LIST (image->vectors)->list;
           list;
           list = g_list_next (list))
        {
          item = (GimpItem *) list->data;

          gimp_item_resize (item, context, width, height, -x1, -y1);
        }

      /*  Don't forget the selection mask!  */
      gimp_item_resize (GIMP_ITEM (gimp_image_get_mask (image)), context,
                        width, height, -x1, -y1);

      /*  crop all layers  */
      list = GIMP_LIST (image->layers)->list;

      while (list)
        {
          item = (GimpItem *) list->data;

          list = g_list_next (list);

          gimp_item_translate (item, -x1, -y1, TRUE);

          if (crop_layers)
            {
              gint off_x, off_y;
              gint lx1, ly1, lx2, ly2;

              gimp_item_offsets (item, &off_x, &off_y);

              lx1 = CLAMP (off_x, 0, image->width);
              ly1 = CLAMP (off_y, 0, image->height);
              lx2 = CLAMP (gimp_item_width  (item) + off_x,
                           0, image->width);
              ly2 = CLAMP (gimp_item_height (item) + off_y,
                           0, image->height);

              width  = lx2 - lx1;
              height = ly2 - ly1;

              if (width > 0 && height > 0)
                gimp_item_resize (item, context, width, height,
                                  -(lx1 - off_x),
                                  -(ly1 - off_y));
              else
                gimp_image_remove_layer (image, GIMP_LAYER (item));
            }
        }

      /*  Reposition or remove all guides  */
      list = image->guides;
      while (list)
        {
          GimpGuide *guide        = list->data;
          gboolean   remove_guide = FALSE;
          gint       position     = gimp_guide_get_position (guide);

          list = g_list_next (list);

          switch (gimp_guide_get_orientation (guide))
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              if ((position < y1) || (position > y2))
                remove_guide = TRUE;
              else
                position -= y1;
              break;

            case GIMP_ORIENTATION_VERTICAL:
              if ((position < x1) || (position > x2))
                remove_guide = TRUE;
              else
                position -= x1;
              break;

            default:
              break;
            }

          if (remove_guide)
            gimp_image_remove_guide (image, guide, TRUE);
          else if (position != gimp_guide_get_position (guide))
            gimp_image_move_guide (image, guide, position, TRUE);
        }

      /*  Reposition or remove sample points  */
      list = image->sample_points;
      while (list)
        {
          GimpSamplePoint *sample_point        = list->data;
          gboolean         remove_sample_point = FALSE;
          gint             new_x               = sample_point->x;
          gint             new_y               = sample_point->y;

          list = g_list_next (list);

          new_y -= y1;
          if ((sample_point->y < y1) || (sample_point->y > y2))
            remove_sample_point = TRUE;

          new_x -= x1;
          if ((sample_point->x < x1) || (sample_point->x > x2))
            remove_sample_point = TRUE;

          if (remove_sample_point)
            gimp_image_remove_sample_point (image, sample_point, TRUE);
          else if (new_x != sample_point->x || new_y != sample_point->y)
            gimp_image_move_sample_point (image, sample_point,
                                          new_x, new_y, TRUE);
        }

      gimp_image_undo_group_end (image);

      gimp_image_update (image, 0, 0, image->width, image->height);

      gimp_viewable_size_changed (GIMP_VIEWABLE (image));
      g_object_thaw_notify (G_OBJECT (image));
    }

  gimp_unset_busy (image->gimp);
}

gboolean
gimp_image_crop_auto_shrink (GimpImage *image,
                             gint       x1,
                             gint       y1,
                             gint       x2,
                             gint       y2,
                             gboolean   active_drawable_only,
                             gint      *shrunk_x1,
                             gint      *shrunk_y1,
                             gint      *shrunk_x2,
                             gint      *shrunk_y2)
{
  GimpDrawable    *active_drawable = NULL;
  GimpPickable    *pickable;
  ColorsEqualFunc  colors_equal_func;
  guchar           bgcolor[MAX_CHANNELS] = { 0, 0, 0, 0 };
  gboolean         has_alpha;
  PixelRegion      PR;
  guchar          *buffer = NULL;
  gint             width, height;
  GimpImageType    type;
  gint             bytes;
  gint             x, y, abort;
  gboolean         retval = FALSE;

  g_return_val_if_fail (image != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (shrunk_x1 != NULL, FALSE);
  g_return_val_if_fail (shrunk_y1 != NULL, FALSE);
  g_return_val_if_fail (shrunk_x2 != NULL, FALSE);
  g_return_val_if_fail (shrunk_y2 != NULL, FALSE);

  gimp_set_busy (image->gimp);

  /* You should always keep in mind that crop->tx2 and crop->ty2 are
     the NOT the coordinates of the bottomright corner of the area to
     be cropped. They point at the pixel located one to the right and
     one to the bottom.
   */

  if (active_drawable_only)
    {
      active_drawable = gimp_image_active_drawable (image);

      if (! active_drawable)
        goto FINISH;

      pickable = GIMP_PICKABLE (active_drawable);
    }
  else
    {
      pickable = GIMP_PICKABLE (image->projection);
   }

  gimp_pickable_flush (pickable);

  type      = gimp_pickable_get_image_type (pickable);
  bytes     = GIMP_IMAGE_TYPE_BYTES (type);
  has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (type);

  switch (gimp_image_crop_guess_bgcolor (pickable,
                                         bytes, has_alpha, bgcolor,
                                         x1, x2-1, y1, y2-1))
    {
    case AUTO_CROP_ALPHA:
      colors_equal_func = (ColorsEqualFunc) gimp_image_crop_colors_alpha;
      break;
    case AUTO_CROP_COLOR:
      colors_equal_func = (ColorsEqualFunc) gimp_image_crop_colors_equal;
      break;
    default:
      goto FINISH;
      break;
    }

  width  = x2 - x1;
  height = y2 - y1;

  pixel_region_init (&PR, gimp_pickable_get_tiles (pickable),
                     x1, y1, width, height, FALSE);

  /* The following could be optimized further by processing
   * the smaller side first instead of defaulting to width    --Sven
   */

  buffer = g_malloc ((width > height ? width : height) * bytes);

  /* Check how many of the top lines are uniform/transparent. */
  abort = FALSE;
  for (y = y1; y < y2 && !abort; y++)
    {
      pixel_region_get_row (&PR, x1, y, width, buffer, 1);
      for (x = 0; x < width && !abort; x++)
        abort = !(colors_equal_func) (bgcolor, buffer + x * bytes, bytes);
    }
  if (y == y2 && !abort)
    goto FINISH;
  y1 = y - 1;

  /* Check how many of the bottom lines are uniform/transparent. */
  abort = FALSE;
  for (y = y2; y > y1 && !abort; y--)
    {
      pixel_region_get_row (&PR, x1, y-1 , width, buffer, 1);
      for (x = 0; x < width && !abort; x++)
        abort = !(colors_equal_func) (bgcolor, buffer + x * bytes, bytes);
    }
  y2 = y + 1;

  /* compute a new height for the next operations */
  height = y2 - y1;

  /* Check how many of the left lines are uniform/transparent. */
  abort = FALSE;
  for (x = x1; x < x2 && !abort; x++)
    {
      pixel_region_get_col (&PR, x, y1, height, buffer, 1);
      for (y = 0; y < height && !abort; y++)
        abort = !(colors_equal_func) (bgcolor, buffer + y * bytes, bytes);
    }
  x1 = x - 1;

  /* Check how many of the right lines are uniform/transparent. */
  abort = FALSE;
  for (x = x2; x > x1 && !abort; x--)
    {
      pixel_region_get_col (&PR, x-1, y1, height, buffer, 1);
      for (y = 0; y < height && !abort; y++)
        abort = !(colors_equal_func) (bgcolor, buffer + y * bytes, bytes);
    }
  x2 = x + 1;

  *shrunk_x1 = x1;
  *shrunk_y1 = y1;
  *shrunk_x2 = x2;
  *shrunk_y2 = y2;

  retval = TRUE;

 FINISH:
  g_free (buffer);
  gimp_unset_busy (image->gimp);

  return retval;
}


/*  private functions  */

static AutoCropType
gimp_image_crop_guess_bgcolor (GimpPickable *pickable,
                               gint          bytes,
                               gboolean      has_alpha,
                               guchar       *color,
                               gint          x1,
                               gint          x2,
                               gint          y1,
                               gint          y2)
{
  guchar  tl[4];
  guchar  tr[4];
  guchar  bl[4];
  guchar  br[4];
  gint    i;

  for (i = 0; i < bytes; i++)
    color[i] = 0;

  /* First check if there's transparency to crop. If not, guess the
   * background-color to see if at least 2 corners are equal.
   */

  if (! gimp_pickable_get_pixel_at (pickable, x1, y1, tl) ||
      ! gimp_pickable_get_pixel_at (pickable, x1, y2, tr) ||
      ! gimp_pickable_get_pixel_at (pickable, x2, y1, bl) ||
      ! gimp_pickable_get_pixel_at (pickable, x2, y2, br))
    {
      return AUTO_CROP_NOTHING;
    }

  if (has_alpha)
    {
      gint alpha = bytes - 1;

      if ((tl[alpha] == 0 && tr[alpha] == 0) ||
          (tl[alpha] == 0 && bl[alpha] == 0) ||
          (tr[alpha] == 0 && br[alpha] == 0) ||
          (bl[alpha] == 0 && br[alpha] == 0))
        {
          return AUTO_CROP_ALPHA;
        }
    }

  if (gimp_image_crop_colors_equal (tl, tr, bytes) ||
      gimp_image_crop_colors_equal (tl, bl, bytes))
    {
      memcpy (color, tl, bytes);
      return AUTO_CROP_COLOR;
    }

  if (gimp_image_crop_colors_equal (br, bl, bytes) ||
      gimp_image_crop_colors_equal (br, tr, bytes))
    {
      memcpy (color, br, bytes);
      return AUTO_CROP_COLOR;
    }

  return AUTO_CROP_NOTHING;
}

static int
gimp_image_crop_colors_equal (guchar *col1,
                              guchar *col2,
                              gint    bytes)
{
  gint b;

  for (b = 0; b < bytes; b++)
    {
      if (col1[b] != col2[b])
        return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_image_crop_colors_alpha (guchar *dummy,
                              guchar *col,
                              gint    bytes)
{
  return (col[bytes - 1] == 0);
}
