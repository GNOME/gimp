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


#define GUIDE_EPSILON 5


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
                       gdouble    y)
{
  GList     *list;
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

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
          if (ABS (guide->position - y) < GUIDE_EPSILON)
            return guide;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          if (ABS (guide->position - x) < GUIDE_EPSILON)
            return guide;
          break;

        default:
          break;
        }
    }

  return NULL;
}

gboolean
gimp_image_snap_point (GimpImage *gimage,
                       gdouble    x,
                       gdouble    y,
                       gint      *tx,
                       gint      *ty)
{
  GList     *list;
  GimpGuide *guide;
  gint       minxdist, minydist;
  gint       dist;
  gboolean   snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  *tx = x;
  *ty = y;

  if (x < 0 || x >= gimage->width ||
      y < 0 || y >= gimage->height)
    {
      return FALSE;
    }

  minxdist = G_MAXINT;
  minydist = G_MAXINT;

  for (list = gimage->guides; list; list = g_list_next (list))
    {
      guide = (GimpGuide *) list->data;

      if (guide->position < 0)
        continue;

      switch (guide->orientation)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          dist = ABS (guide->position - y);

          if (dist < MIN (GUIDE_EPSILON, minydist))
            {
              minydist = dist;
              *ty = guide->position;
              snapped = TRUE;
            }
          break;

        case GIMP_ORIENTATION_VERTICAL:
          dist = ABS (guide->position - x);

          if (dist < MIN (GUIDE_EPSILON, minxdist))
            {
              minxdist = dist;
              *tx = guide->position;
              snapped = TRUE;
            }
          break;

        default:
          break;
        }
    }

  return snapped;
}

gboolean
gimp_image_snap_rectangle (GimpImage *gimage,
                           gdouble    x1,
                           gdouble    y1,
                           gdouble    x2,
                           gdouble    y2,
                           gint      *tx1,
                           gint      *ty1)
{
  gint     nx1, ny1;
  gint     nx2, ny2;
  gboolean snap1, snap2;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (tx1 != NULL, FALSE);
  g_return_val_if_fail (ty1 != NULL, FALSE);

  *tx1 = x1;
  *ty1 = y1;

  snap1 = gimp_image_snap_point (gimage, x1, y1, &nx1, &ny1);
  snap2 = gimp_image_snap_point (gimage, x2, y2, &nx2, &ny2);

  if (snap1 || snap2)
    {
      if (x1 != nx1)
	*tx1 = nx1;
      else if (x2 != nx2)
	*tx1 = x1 + (nx2 - x2);
  
      if (y1 != ny1)
	*ty1 = ny1;
      else if (y2 != ny2)
	*ty1 = y1 + (ny2 - y2);
    }

  return snap1 || snap2;
}
