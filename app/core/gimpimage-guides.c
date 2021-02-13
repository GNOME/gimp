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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpguide.h"
#include "gimpimage-guides.h"
#include "gimpimage-private.h"
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

  guide = gimp_guide_new (GIMP_ORIENTATION_HORIZONTAL,
                          image->gimp->next_guide_id++);

  if (push_undo)
    gimp_image_undo_push_guide (image,
                                C_("undo-type", "Add Horizontal Guide"), guide);

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

  guide = gimp_guide_new (GIMP_ORIENTATION_VERTICAL,
                          image->gimp->next_guide_id++);

  if (push_undo)
    gimp_image_undo_push_guide (image,
                                C_("undo-type", "Add Vertical Guide"), guide);

  gimp_image_add_guide (image, guide, position);
  g_object_unref (guide);

  return guide;
}

void
gimp_image_add_guide (GimpImage *image,
                      GimpGuide *guide,
                      gint       position)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->guides = g_list_prepend (private->guides, guide);

  gimp_guide_set_position (guide, position);
  g_object_ref (guide);

  gimp_image_guide_added (image, guide);
}

void
gimp_image_remove_guide (GimpImage *image,
                         GimpGuide *guide,
                         gboolean   push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (gimp_guide_is_custom (guide))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_guide (image, C_("undo-type", "Remove Guide"), guide);

  private->guides = g_list_remove (private->guides, guide);
  gimp_aux_item_removed (GIMP_AUX_ITEM (guide));

  gimp_image_guide_removed (image, guide);

  gimp_guide_set_position (guide, GIMP_GUIDE_POSITION_UNDEFINED);
  g_object_unref (guide);
}

void
gimp_image_move_guide (GimpImage *image,
                       GimpGuide *guide,
                       gint       position,
                       gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  if (gimp_guide_is_custom (guide))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_guide (image, C_("undo-type", "Move Guide"), guide);

  gimp_guide_set_position (guide, position);

  gimp_image_guide_moved (image, guide);
}

GList *
gimp_image_get_guides (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->guides;
}

GimpGuide *
gimp_image_get_guide (GimpImage *image,
                      guint32    id)
{
  GList *guides;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (guides = GIMP_IMAGE_GET_PRIVATE (image)->guides;
       guides;
       guides = g_list_next (guides))
    {
      GimpGuide *guide = guides->data;

      if (gimp_aux_item_get_id (GIMP_AUX_ITEM (guide)) == id)
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

  for (guides = GIMP_IMAGE_GET_PRIVATE (image)->guides;
       guides;
       guides = g_list_next (guides))
    {
      GimpGuide *guide = guides->data;

      if (*guide_found) /* this is the first guide after the found one */
        return guide;

      if (gimp_aux_item_get_id (GIMP_AUX_ITEM (guide)) == id) /* found it, next one will be returned */
        *guide_found = TRUE;
    }

  return NULL;
}
