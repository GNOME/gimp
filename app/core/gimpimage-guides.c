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

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


/*  public functions  */

GimpGuide *
gimp_image_add_hguide (GimpImage *gimage,
                       gint       position,
                       gboolean   push_undo)
{
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (position >= 0 && position <= gimage->height, NULL);

  guide = g_new0 (GimpGuide, 1);

  guide->ref_count   = 1;
  guide->position    = -1;
  guide->orientation = GIMP_ORIENTATION_HORIZONTAL;
  guide->guide_ID    = gimage->gimp->next_guide_ID++;

  if (push_undo)
    gimp_image_undo_push_image_guide (gimage, _("Add Horizontal Guide"),
                                      guide);

  gimp_image_add_guide (gimage, guide, position);
  gimp_image_guide_unref (guide);

  return guide;
}

GimpGuide *
gimp_image_add_vguide (GimpImage *gimage,
                       gint       position,
                       gboolean   push_undo)
{
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (position >= 0 && position <= gimage->width, NULL);

  guide = g_new0 (GimpGuide, 1);

  guide->ref_count   = 1;
  guide->position    = -1;
  guide->orientation = GIMP_ORIENTATION_VERTICAL;
  guide->guide_ID    = gimage->gimp->next_guide_ID++;

  if (push_undo)
    gimp_image_undo_push_image_guide (gimage, _("Add Vertical Guide"),
                                      guide);

  gimp_image_add_guide (gimage, guide, position);
  gimp_image_guide_unref (guide);

  return guide;
}

GimpGuide *
gimp_image_guide_ref (GimpGuide *guide)
{
  g_return_val_if_fail (guide != NULL, NULL);

  guide->ref_count++;

  return guide;
}

void
gimp_image_guide_unref (GimpGuide *guide)
{
  g_return_if_fail (guide != NULL);

  guide->ref_count--;

  if (guide->ref_count < 1)
    g_free (guide);
}

void
gimp_image_add_guide (GimpImage *gimage,
		      GimpGuide *guide,
                      gint       position)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (guide != NULL);
  g_return_if_fail (position >= 0);

  if (guide->orientation == GIMP_ORIENTATION_HORIZONTAL)
    g_return_if_fail (position <= gimage->height);
  else
    g_return_if_fail (position <= gimage->width);

  gimage->guides = g_list_prepend (gimage->guides, guide);

  guide->position = position;
  gimp_image_guide_ref (guide);

  gimp_image_update_guide (gimage, guide);
}

void
gimp_image_remove_guide (GimpImage *gimage,
			 GimpGuide *guide,
                         gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (guide != NULL);

  gimp_image_update_guide (gimage, guide);

  if (push_undo)
    gimp_image_undo_push_image_guide (gimage, _("Remove Guide"), guide);

  gimage->guides = g_list_remove (gimage->guides, guide);

  guide->position = -1;
  gimp_image_guide_unref (guide);
}

void
gimp_image_move_guide (GimpImage *gimage,
                       GimpGuide *guide,
                       gint       position,
                       gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (guide != NULL);
  g_return_if_fail (position >= 0);

  if (guide->orientation == GIMP_ORIENTATION_HORIZONTAL)
    g_return_if_fail (position <= gimage->height);
  else
    g_return_if_fail (position <= gimage->width);

  if (push_undo)
    gimp_image_undo_push_image_guide (gimage, _("Move Guide"), guide);

  gimp_image_update_guide (gimage, guide);
  guide->position = position;
  gimp_image_update_guide (gimage, guide);
}

GimpGuide *
gimp_image_find_guide (GimpImage *gimage,
                       gdouble    x,
                       gdouble    y,
                       gdouble    epsilon_x,
                       gdouble    epsilon_y)
{
  GList     *list;
  GimpGuide *guide;
  GimpGuide *ret = NULL;
  gdouble    dist;
  gdouble    mindist = G_MAXDOUBLE;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= gimage->width ||
      y < 0 || y >= gimage->height)
    {
      return NULL;
    }

  for (list = gimage->guides; list; list = g_list_next (list))
    {
      guide = (GimpGuide *) list->data;

      if (guide->position < 0)
        continue;

      switch (guide->orientation)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          dist = ABS (guide->position - y);
          if (dist < MIN (epsilon_y, mindist))
            {
              mindist = dist;
              ret = guide;
            }
          break;

        /* mindist always is in vertical resolution to make it comparable */
        case GIMP_ORIENTATION_VERTICAL:
          dist = ABS (guide->position - x);
          if (dist < MIN (epsilon_x, mindist / epsilon_y * epsilon_x))
            {
              mindist = dist * epsilon_y / epsilon_x;
              ret = guide;
            }
          break;

        default:
          continue;
        }

    }

  return ret;
}
