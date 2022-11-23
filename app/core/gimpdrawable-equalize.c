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

#include "ligmadrawable.h"
#include "ligmadrawable-equalize.h"
#include "ligmadrawable-histogram.h"
#include "ligmadrawable-operation.h"
#include "ligmahistogram.h"
#include "ligmaimage.h"
#include "ligmaselection.h"

#include "ligma-intl.h"


void
ligma_drawable_equalize (LigmaDrawable *drawable,
                        gboolean      mask_only)
{
  LigmaImage     *image;
  LigmaChannel   *selection;
  LigmaHistogram *histogram;
  GeglNode      *equalize;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));

  image = ligma_item_get_image (LIGMA_ITEM (drawable));
  selection = ligma_image_get_mask (image);

  histogram = ligma_histogram_new (FALSE);
  ligma_drawable_calculate_histogram (drawable, histogram, FALSE);

  equalize = gegl_node_new_child (NULL,
                                  "operation", "ligma:equalize",
                                  "histogram", histogram,
                                  NULL);

  if (! mask_only)
    ligma_selection_suspend (LIGMA_SELECTION (selection));

  ligma_drawable_apply_operation (drawable, NULL,
                                 C_("undo-type", "Equalize"),
                                 equalize);

  if (! mask_only)
    ligma_selection_resume (LIGMA_SELECTION (selection));

  g_object_unref (equalize);
  g_object_unref (histogram);
}
