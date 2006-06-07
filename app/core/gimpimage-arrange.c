/* The GIMP -- an image manipulation program
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

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-arrange.h"
#include "gimpimage-guides.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimpguide.h"

#include "gimp-intl.h"

/**
 * gimp_image_arrange_objects:
 * @image:                The #GimpImage to which the objects belong.
 * @list:                 A #GList of objects to be aligned.
 * @alignment:            The point on each target object to bring into alignment.
 * @reference:            The #GObject to align the targets with.
 * @reference_alignment:  The point on the reference object to align the target item with..
 * @offset:               How much to shift the target from perfect alignment..
 *
 * This function shifts the positions of a set of target objects, which can be
 * "items" or guides, to bring them into a specified type of alignment with a
 * reference object, which can be an item, guide, or image.  If the requested
 * alignment does not make sense (i.e., trying to align a vertical guide vertically),
 * nothing happens and no error message is generated.
 *
 * When there are multiple target objects, they are arranged so that the spacing
 * between consecutive ones is given by the argument @offset.
 */
void
gimp_image_arrange_objects (GimpImage         *image,
                            GList             *list,
                            GimpAlignmentType  alignment,
                            GObject           *reference,
                            GimpAlignmentType  reference_alignment,
                            gint               offset)
{
  gboolean do_x               = FALSE;
  gboolean do_y               = FALSE;
  gint     reference_offset_x = 0;
  gint     reference_offset_y = 0;
  gint     reference_height   = 0;
  gint     reference_width    = 0;
  gint     x0                 = 0;
  gint     y0                 = 0;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (G_IS_OBJECT (reference));

  /* figure out whether we are aligning horizontally or vertically */
  switch (reference_alignment)
    {
    case GIMP_ALIGN_LEFT:
    case GIMP_ALIGN_CENTER:
    case GIMP_ALIGN_RIGHT:
      do_x = TRUE;
      break;
    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_MIDDLE:
    case GIMP_ALIGN_BOTTOM:
      do_y = TRUE;
      break;
    default:
      g_assert_not_reached ();
    }

  /* get dimensions of the reference object */
  if (GIMP_IS_IMAGE (reference))
    {
      reference_offset_x = 0;
      reference_offset_y = 0;
      reference_width = gimp_image_get_width (image);
      reference_height = gimp_image_get_height (image);
    }
  else if (GIMP_IS_ITEM (reference))
    {
      GimpItem *item = GIMP_ITEM (reference);

      reference_offset_x = item->offset_x;
      reference_offset_y = item->offset_y;
      reference_height   = item->height;
      reference_width    = item->width;
    }
  else if (GIMP_IS_GUIDE (reference))
    {
      GimpGuide *guide = GIMP_GUIDE (reference);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_VERTICAL:
          if (do_y)
            return;
          reference_offset_x = gimp_guide_get_position (guide);
          reference_width = 0;
          break;

        case GIMP_ORIENTATION_HORIZONTAL:
          if (do_x)
            return;
          reference_offset_y = gimp_guide_get_position (guide);
          reference_height = 0;
          break;

        default:
          break;
        }
    }
  else
    {
      g_printerr ("Reference object is not an image, item, or guide.\n");
      return;
    }

  /* compute alignment pos for reference object */
  switch (reference_alignment)
    {
    case GIMP_ALIGN_LEFT:
      x0 = reference_offset_x;
      break;
    case GIMP_ALIGN_CENTER:
      x0 = reference_offset_x + reference_width/2;
      break;
    case GIMP_ALIGN_RIGHT:
      x0 = reference_offset_x + reference_width;
      break;
    case GIMP_ALIGN_TOP:
      y0 = reference_offset_y;
      break;
    case GIMP_ALIGN_MIDDLE:
      y0 = reference_offset_y + reference_height/2;
      break;
    case GIMP_ALIGN_BOTTOM:
      y0 = reference_offset_y + reference_height;
      break;
    default:
      g_assert_not_reached ();
    }


  if (list)
    {
      GList *l;
      gint   n;

      /* FIXME: undo group type is wrong */
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   _("Arrange Objects"));

      for (l = list, n = 1; l; l = g_list_next (l), n++)
        {
          GObject *target          = G_OBJECT (l->data);
          gint     xtranslate      = 0;
          gint     ytranslate      = 0;
          gint     target_offset_x = 0;
          gint     target_offset_y = 0;
          gint     target_height   = 0;
          gint     target_width    = 0;
          gint     x1              = 0;
          gint     y1              = 0;

          /* compute dimensions of target object */
          if (GIMP_IS_ITEM (target))
            {
              GimpItem *item = GIMP_ITEM (target);

              target_offset_x = item->offset_x;
              target_offset_y = item->offset_y;
              target_height   = item->height;
              target_width    = item->width;
            }
          else if (GIMP_IS_GUIDE (target))
            {
              GimpGuide *guide = GIMP_GUIDE (target);

              switch (gimp_guide_get_orientation (guide))
                {
                case GIMP_ORIENTATION_VERTICAL:
                  if (do_y)
                    return;
                  target_offset_x = gimp_guide_get_position (guide);
                  target_width = 0;
                  break;

                case GIMP_ORIENTATION_HORIZONTAL:
                  if (do_x)
                    return;
                  target_offset_y = gimp_guide_get_position (guide);
                  target_height = 0;
                  break;

                default:
                  break;
                }
            }
          else
            {
              g_printerr ("Alignment target is not an item or guide.\n");
            }

          if (do_x)
            {
              switch (alignment)
                {
                case GIMP_ALIGN_LEFT:
                  x1 = target_offset_x;
                  break;
                case GIMP_ALIGN_CENTER:
                  x1 = target_offset_x + target_width/2;
                  break;
                case GIMP_ALIGN_RIGHT:
                  x1 = target_offset_x + target_width;
                  break;
                default:
                  g_assert_not_reached ();
                }

              xtranslate = x0 - x1 + n * offset;
            }

          if (do_y)
            {
              switch (alignment)
                {
                case GIMP_ALIGN_TOP:
                  y1 = target_offset_y;
                  break;
                case GIMP_ALIGN_MIDDLE:
                  y1 = target_offset_y + target_height/2;
                  break;
                case GIMP_ALIGN_BOTTOM:
                  y1 = target_offset_y + target_height;
                  break;
                default:
                  g_assert_not_reached ();
                }

              ytranslate = y0 - y1 + n * offset;
            }

          /* now actually align the target object */
          if (GIMP_IS_ITEM (target))
            {
              gimp_item_translate (GIMP_ITEM (target),
                                   xtranslate, ytranslate, TRUE);
            }
          else if (GIMP_IS_GUIDE (target))
            {
              GimpGuide *guide = GIMP_GUIDE (target);

              switch (gimp_guide_get_orientation (guide))
                {
                case GIMP_ORIENTATION_VERTICAL:
                  gimp_image_move_guide (image, guide, x1 + xtranslate, TRUE);
                  gimp_image_update_guide (image, guide);
                  break;

                case GIMP_ORIENTATION_HORIZONTAL:
                  gimp_image_move_guide (image, guide, y1 + ytranslate, TRUE);
                  gimp_image_update_guide (image, guide);
                  break;

                default:
                  break;
                }
            }
        }

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }
}
