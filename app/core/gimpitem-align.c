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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitem-align.h"

#include "gimp-intl.h"

/**
 * gimp_item_align:
 * @target:               The #GimpItem to move.
 * @target_alignment:     The part of the target to align..
 * @reference:            The #GimpItem to align the target with.
 * @reference_alignment:  The part of the reference item to align the target item with..
 * @offset:               How much to shift the target from perfect alignment..
 *
 * Aligns the target item with the reference item in the specified way.  If
 * the reference item is #NULL, then the target item is aligned with the
 * image it belongs to.
 */
void
gimp_item_align (GimpItem          *target,
                 GimpAlignmentType  target_alignment,
                 GimpItem          *reference,
                 GimpAlignmentType  reference_alignment,
                 gint               offset)
{
  gboolean do_x       = FALSE;
  gboolean do_y       = FALSE;
  gint     xtranslate = 0;
  gint     ytranslate = 0;
  gint     reference_offset_x;
  gint     reference_offset_y;
  gint     reference_height;
  gint     reference_width;
  gint     x0         = 0;
  gint     y0         = 0;
  gint     x1         = 0;
  gint     y1         = 0;

  if (!target)
    return;

  if (! (reference || target->gimage))
    return;

  if (reference)
    {
      reference_offset_x = reference->offset_x;
      reference_offset_y = reference->offset_y;
      reference_height   = reference->height;
      reference_width    = reference->width;
    }
  else
    {
      reference_offset_x = 0;
      reference_offset_y = 0;
      reference_height   = gimp_image_get_height (target->gimage);
      reference_width    = gimp_image_get_width (target->gimage);
    }

  switch (reference_alignment)
    {
    case GIMP_ALIGN_LEFT:
      do_x = TRUE;
      x0 = reference_offset_x;
      break;
    case GIMP_ALIGN_CENTER:
      do_x = TRUE;
      x0 = reference_offset_x + reference_width/2;
      break;
    case GIMP_ALIGN_RIGHT:
      do_x = TRUE;
      x0 = reference_offset_x + reference_width;
      break;
    case GIMP_ALIGN_TOP:
      do_y = TRUE;
      y0 = reference_offset_y;
      break;
    case GIMP_ALIGN_MIDDLE:
      do_y = TRUE;
      y0 = reference_offset_y + reference_height/2;
      break;
    case GIMP_ALIGN_BOTTOM:
      do_y = TRUE;
      y0 = reference_offset_y + reference_height;
      break;
    default:
      g_assert_not_reached ();
    }

  if (do_x)
    {
      switch (target_alignment)
        {
        case GIMP_ALIGN_LEFT:
          x1 = target->offset_x;
          break;
        case GIMP_ALIGN_CENTER:
          x1 = target->offset_x + target->width/2;
          break;
        case GIMP_ALIGN_RIGHT:
          x1 = target->offset_x + target->width;
          break;
        default:
          g_assert_not_reached ();
        }

      xtranslate = x0 - x1 + offset;
    }

  if (do_y)
    {
      switch (target_alignment)
        {
        case GIMP_ALIGN_TOP:
          y1 = target->offset_y;
          break;
        case GIMP_ALIGN_MIDDLE:
          y1 = target->offset_y + target->height/2;
          break;
        case GIMP_ALIGN_BOTTOM:
          y1 = target->offset_y + target->height;
          break;
        default:
          g_assert_not_reached ();
        }

      ytranslate = y0 - y1 + offset;
    }

  gimp_item_translate (target, xtranslate, ytranslate, TRUE);
}
