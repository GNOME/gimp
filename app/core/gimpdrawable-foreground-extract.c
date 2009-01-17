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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/siox.h"
#include "base/tile-manager.h"

#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpdrawable-foreground-extract.h"
#include "gimpimage.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_foreground_extract (GimpDrawable              *drawable,
                                  GimpForegroundExtractMode  mode,
                                  GimpDrawable              *mask,
                                  GimpProgress              *progress)
{
  SioxState    *state;
  const gdouble sensitivity[3] = { SIOX_DEFAULT_SENSITIVITY_L,
                                   SIOX_DEFAULT_SENSITIVITY_A,
                                   SIOX_DEFAULT_SENSITIVITY_B };

  g_return_if_fail (GIMP_IS_DRAWABLE (mask));
  g_return_if_fail (mode == GIMP_FOREGROUND_EXTRACT_SIOX);

  state =
    gimp_drawable_foreground_extract_siox_init (drawable,
                                                0, 0,
                                                gimp_item_get_width  (GIMP_ITEM (mask)),
                                                gimp_item_get_height (GIMP_ITEM (mask)));

  if (state)
    {
      gimp_drawable_foreground_extract_siox (mask, state,
                                             SIOX_REFINEMENT_RECALCULATE,
                                             SIOX_DEFAULT_SMOOTHNESS,
                                             sensitivity,
                                             FALSE,
                                             progress);

      gimp_drawable_foreground_extract_siox_done (state);
    }
}

SioxState *
gimp_drawable_foreground_extract_siox_init (GimpDrawable *drawable,
                                            gint          x,
                                            gint          y,
                                            gint          width,
                                            gint          height)
{
  const guchar *colormap = NULL;
  gboolean      intersect;
  gint          offset_x;
  gint          offset_y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);

  if (gimp_drawable_is_indexed (drawable))
    colormap = gimp_drawable_get_colormap (drawable);

  gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

  intersect = gimp_rectangle_intersect (offset_x, offset_y,
                                        gimp_item_get_width  (GIMP_ITEM (drawable)),
                                        gimp_item_get_height (GIMP_ITEM (drawable)),
                                        x, y, width, height,
                                        &x, &y, &width, &height);


  /* FIXME:
   * Clear the mask outside the rectangle that we are working on?
   */

  if (! intersect)
    return NULL;

  return siox_init (gimp_drawable_get_tiles (drawable), colormap,
                    offset_x, offset_y,
                    x, y, width, height);
}

void
gimp_drawable_foreground_extract_siox (GimpDrawable       *mask,
                                       SioxState          *state,
                                       SioxRefinementType  refinement,
                                       gint                smoothness,
                                       const gdouble       sensitivity[3],
                                       gboolean            multiblob,
                                       GimpProgress       *progress)
{
  gint x1, y1;
  gint x2, y2;

  g_return_if_fail (GIMP_IS_DRAWABLE (mask));
  g_return_if_fail (gimp_drawable_bytes (mask) == 1);

  g_return_if_fail (state != NULL);

  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (progress)
    gimp_progress_start (progress, _("Foreground Extraction"), FALSE);

  if (GIMP_IS_CHANNEL (mask))
    {
      gimp_channel_bounds (GIMP_CHANNEL (mask), &x1, &y1, &x2, &y2);
    }
  else
    {
      x1 = 0;
      y1 = 0;
      x2 = gimp_item_get_width  (GIMP_ITEM (mask));
      y2 = gimp_item_get_height (GIMP_ITEM (mask));
    }

  siox_foreground_extract (state, refinement,
                           gimp_drawable_get_tiles (mask), x1, y1, x2, y2,
                           smoothness, sensitivity, multiblob,
                           (SioxProgressFunc) gimp_progress_set_value,
                           progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_drawable_update (mask, x1, y1, x2, y2);
}

void
gimp_drawable_foreground_extract_siox_done (SioxState *state)
{
  g_return_if_fail (state != NULL);

  siox_done (state);
}
