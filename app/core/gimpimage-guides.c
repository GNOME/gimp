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

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpguide.h"
#include "gimpimage-guides.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


/*  public functions  */

GimpGuide *
gimp_image_add_hguide (GimpImage *image,
                       gint       position,
                       gboolean   push_undo)
{
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (position >= 0 && position <= image->height, NULL);

  guide = gimp_guide_new (GIMP_ORIENTATION_HORIZONTAL,
                          image->gimp->next_guide_ID++);

  if (push_undo)
    gimp_image_undo_push_image_guide (image, _("Add Horizontal Guide"), guide);

  gimp_image_add_guide (image, guide, position);
  g_object_unref (G_OBJECT (guide));

  return guide;
}

GimpGuide *
gimp_image_add_vguide (GimpImage *image,
                       gint       position,
                       gboolean   push_undo)
{
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (position >= 0 && position <= image->width, NULL);

  guide = gimp_guide_new (GIMP_ORIENTATION_VERTICAL,
                          image->gimp->next_guide_ID++);

  if (push_undo)
    gimp_image_undo_push_image_guide (image, _("Add Vertical Guide"), guide);

  gimp_image_add_guide (image, guide, position);
  g_object_unref (G_OBJECT (guide));

  return guide;
}

void
gimp_image_add_guide (GimpImage *image,
                      GimpGuide *guide,
                      gint       position)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));
  g_return_if_fail (position >= 0);

  if (gimp_guide_get_orientation (guide) == GIMP_ORIENTATION_HORIZONTAL)
    g_return_if_fail (position <= image->height);
  else
    g_return_if_fail (position <= image->width);

  image->guides = g_list_prepend (image->guides, guide);

  gimp_guide_set_position (guide, position);
  g_object_ref (G_OBJECT (guide));

  gimp_image_update_guide (image, guide);
}

void
gimp_image_remove_guide (GimpImage *image,
                         GimpGuide *guide,
                         gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  gimp_image_update_guide (image, guide);

  if (push_undo)
    gimp_image_undo_push_image_guide (image, _("Remove Guide"), guide);

  image->guides = g_list_remove (image->guides, guide);
  gimp_guide_removed (guide);

  gimp_guide_set_position (guide, -1);
  g_object_unref (G_OBJECT (guide));
}

void
gimp_image_move_guide (GimpImage *image,
                       GimpGuide *guide,
                       gint       position,
                       gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));
  g_return_if_fail (position >= 0);

  if (gimp_guide_get_orientation (guide) == GIMP_ORIENTATION_HORIZONTAL)
    g_return_if_fail (position <= image->height);
  else
    g_return_if_fail (position <= image->width);

  if (push_undo)
    gimp_image_undo_push_image_guide (image, _("Move Guide"), guide);

  gimp_image_update_guide (image, guide);
  gimp_guide_set_position (guide, position);
  gimp_image_update_guide (image, guide);
}

GimpGuide *
gimp_image_get_guide (GimpImage *image,
                      guint32    id)
{
  GList *guides;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (guides = image->guides; guides; guides = g_list_next (guides))
    {
      GimpGuide *guide = guides->data;

      if (gimp_guide_get_ID (guide) == id &&
          gimp_guide_get_position (guide) >= 0)
        return guide;
    }

  return NULL;
}

GimpGuide *
gimp_image_get_next_guide (GimpImage *image,
                           guint32    id,
                           gboolean  *guide_found)
{
  GList *guides;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (guide_found != NULL, NULL);

  if (id == 0)
    *guide_found = TRUE;
  else
    *guide_found = FALSE;

  for (guides = image->guides; guides; guides = g_list_next (guides))
    {
      GimpGuide *guide = guides->data;

      if (gimp_guide_get_position (guide) < 0)
        continue;

      if (*guide_found) /* this is the first guide after the found one */
        return guide;

      if (gimp_guide_get_ID (guide) == id) /* found it, next one will be returned */
        *guide_found = TRUE;
    }

  return NULL;
}

GimpGuide *
gimp_image_find_guide (GimpImage *image,
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

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= image->width ||
      y < 0 || y >= image->height)
    {
      return NULL;
    }

  for (list = image->guides; list; list = g_list_next (list))
    {
      gint position;

      guide = list->data;
      position = gimp_guide_get_position (guide);

      if (position < 0)
        continue;

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          dist = ABS (position - y);
          if (dist < MIN (epsilon_y, mindist))
            {
              mindist = dist;
              ret = guide;
            }
          break;

        /* mindist always is in vertical resolution to make it comparable */
        case GIMP_ORIENTATION_VERTICAL:
          dist = ABS (position - x);
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
