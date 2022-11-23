/* LIGMA - The GNU Image Manipulation Program
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

#include "ligma.h"
#include "ligmaimage.h"
#include "ligmaguide.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo-push.h"

#include "ligma-intl.h"


/*  public functions  */

LigmaGuide *
ligma_image_add_hguide (LigmaImage *image,
                       gint       position,
                       gboolean   push_undo)
{
  LigmaGuide *guide;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  guide = ligma_guide_new (LIGMA_ORIENTATION_HORIZONTAL,
                          image->ligma->next_guide_id++);

  if (push_undo)
    ligma_image_undo_push_guide (image,
                                C_("undo-type", "Add Horizontal Guide"), guide);

  ligma_image_add_guide (image, guide, position);
  g_object_unref (G_OBJECT (guide));

  return guide;
}

LigmaGuide *
ligma_image_add_vguide (LigmaImage *image,
                       gint       position,
                       gboolean   push_undo)
{
  LigmaGuide *guide;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  guide = ligma_guide_new (LIGMA_ORIENTATION_VERTICAL,
                          image->ligma->next_guide_id++);

  if (push_undo)
    ligma_image_undo_push_guide (image,
                                C_("undo-type", "Add Vertical Guide"), guide);

  ligma_image_add_guide (image, guide, position);
  g_object_unref (guide);

  return guide;
}

void
ligma_image_add_guide (LigmaImage *image,
                      LigmaGuide *guide,
                      gint       position)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->guides = g_list_prepend (private->guides, guide);

  ligma_guide_set_position (guide, position);
  g_object_ref (guide);

  ligma_image_guide_added (image, guide);
}

void
ligma_image_remove_guide (LigmaImage *image,
                         LigmaGuide *guide,
                         gboolean   push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (ligma_guide_is_custom (guide))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_guide (image, C_("undo-type", "Remove Guide"), guide);

  private->guides = g_list_remove (private->guides, guide);
  ligma_aux_item_removed (LIGMA_AUX_ITEM (guide));

  ligma_image_guide_removed (image, guide);

  ligma_guide_set_position (guide, LIGMA_GUIDE_POSITION_UNDEFINED);
  g_object_unref (guide);
}

void
ligma_image_move_guide (LigmaImage *image,
                       LigmaGuide *guide,
                       gint       position,
                       gboolean   push_undo)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  if (ligma_guide_is_custom (guide))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_guide (image, C_("undo-type", "Move Guide"), guide);

  ligma_guide_set_position (guide, position);

  ligma_image_guide_moved (image, guide);
}

GList *
ligma_image_get_guides (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->guides;
}

LigmaGuide *
ligma_image_get_guide (LigmaImage *image,
                      guint32    id)
{
  GList *guides;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  for (guides = LIGMA_IMAGE_GET_PRIVATE (image)->guides;
       guides;
       guides = g_list_next (guides))
    {
      LigmaGuide *guide = guides->data;

      if (ligma_aux_item_get_id (LIGMA_AUX_ITEM (guide)) == id)
        return guide;
    }

  return NULL;
}

LigmaGuide *
ligma_image_get_next_guide (LigmaImage *image,
                           guint32    id,
                           gboolean  *guide_found)
{
  GList *guides;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (guide_found != NULL, NULL);

  if (id == 0)
    *guide_found = TRUE;
  else
    *guide_found = FALSE;

  for (guides = LIGMA_IMAGE_GET_PRIVATE (image)->guides;
       guides;
       guides = g_list_next (guides))
    {
      LigmaGuide *guide = guides->data;

      if (*guide_found) /* this is the first guide after the found one */
        return guide;

      if (ligma_aux_item_get_id (LIGMA_AUX_ITEM (guide)) == id) /* found it, next one will be returned */
        *guide_found = TRUE;
    }

  return NULL;
}
