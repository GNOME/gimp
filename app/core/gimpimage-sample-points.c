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
#include "ligmaimage-private.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-undo-push.h"
#include "ligmasamplepoint.h"

#include "ligma-intl.h"


/*  public functions  */

LigmaSamplePoint *
ligma_image_add_sample_point_at_pos (LigmaImage *image,
                                    gint       x,
                                    gint       y,
                                    gboolean   push_undo)
{
  LigmaSamplePoint *sample_point;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (x >= 0 && x < ligma_image_get_width  (image), NULL);
  g_return_val_if_fail (y >= 0 && y < ligma_image_get_height (image), NULL);

  sample_point = ligma_sample_point_new (image->ligma->next_sample_point_id++);

  if (push_undo)
    ligma_image_undo_push_sample_point (image, C_("undo-type", "Add Sample Point"),
                                       sample_point);

  ligma_image_add_sample_point (image, sample_point, x, y);
  g_object_unref (sample_point);

  return sample_point;
}

void
ligma_image_add_sample_point (LigmaImage       *image,
                             LigmaSamplePoint *sample_point,
                             gint             x,
                             gint             y)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->sample_points = g_list_append (private->sample_points, sample_point);

  ligma_sample_point_set_position (sample_point, x, y);
  g_object_ref (sample_point);

  ligma_image_sample_point_added (image, sample_point);
}

void
ligma_image_remove_sample_point (LigmaImage       *image,
                                LigmaSamplePoint *sample_point,
                                gboolean         push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    ligma_image_undo_push_sample_point (image,
                                       C_("undo-type", "Remove Sample Point"),
                                       sample_point);

  private->sample_points = g_list_remove (private->sample_points, sample_point);
  ligma_aux_item_removed (LIGMA_AUX_ITEM (sample_point));

  ligma_image_sample_point_removed (image, sample_point);

  ligma_sample_point_set_position (sample_point,
                                  LIGMA_SAMPLE_POINT_POSITION_UNDEFINED,
                                  LIGMA_SAMPLE_POINT_POSITION_UNDEFINED);
  g_object_unref (sample_point);
}

void
ligma_image_move_sample_point (LigmaImage       *image,
                              LigmaSamplePoint *sample_point,
                              gint             x,
                              gint             y,
                              gboolean         push_undo)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x < ligma_image_get_width  (image));
  g_return_if_fail (y < ligma_image_get_height (image));

  if (push_undo)
    ligma_image_undo_push_sample_point (image,
                                       C_("undo-type", "Move Sample Point"),
                                       sample_point);

  ligma_sample_point_set_position (sample_point, x, y);

  ligma_image_sample_point_moved (image, sample_point);
}

void
ligma_image_set_sample_point_pick_mode (LigmaImage         *image,
                                       LigmaSamplePoint   *sample_point,
                                       LigmaColorPickMode  pick_mode,
                                       gboolean           push_undo)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  if (push_undo)
    ligma_image_undo_push_sample_point (image,
                                       C_("undo-type",
                                          "Set Sample Point Pick Mode"),
                                       sample_point);

  ligma_sample_point_set_pick_mode (sample_point, pick_mode);

  /* well... */
  ligma_image_sample_point_moved (image, sample_point);
}

GList *
ligma_image_get_sample_points (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->sample_points;
}

LigmaSamplePoint *
ligma_image_get_sample_point (LigmaImage *image,
                             guint32    id)
{
  GList *sample_points;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  for (sample_points = LIGMA_IMAGE_GET_PRIVATE (image)->sample_points;
       sample_points;
       sample_points = g_list_next (sample_points))
    {
      LigmaSamplePoint *sample_point = sample_points->data;

      if (ligma_aux_item_get_id (LIGMA_AUX_ITEM (sample_point)) == id)
        return sample_point;
    }

  return NULL;
}

LigmaSamplePoint *
ligma_image_get_next_sample_point (LigmaImage *image,
                                  guint32    id,
                                  gboolean  *sample_point_found)
{
  GList *sample_points;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (sample_point_found != NULL, NULL);

  if (id == 0)
    *sample_point_found = TRUE;
  else
    *sample_point_found = FALSE;

  for (sample_points = LIGMA_IMAGE_GET_PRIVATE (image)->sample_points;
       sample_points;
       sample_points = g_list_next (sample_points))
    {
      LigmaSamplePoint *sample_point = sample_points->data;

      if (*sample_point_found) /* this is the first guide after the found one */
        return sample_point;

      if (ligma_aux_item_get_id (LIGMA_AUX_ITEM (sample_point)) == id) /* found it, next one will be returned */
        *sample_point_found = TRUE;
    }

  return NULL;
}
