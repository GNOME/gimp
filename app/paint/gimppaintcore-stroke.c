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

#include <gtk/gtk.h>

#include "paint-types.h"

#include "core/gimpdrawable.h"

#include "gimppaintcore.h"
#include "gimppaintcore-stroke.h"


gboolean
gimp_paint_core_stroke (GimpPaintCore    *core,
                        GimpDrawable     *drawable,
                        GimpPaintOptions *paint_options,
                        gint              n_strokes,
                        gdouble          *strokes)
{
  GimpCoords coords;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (paint_options != NULL, FALSE);
  g_return_val_if_fail (n_strokes > 0, FALSE);
  g_return_val_if_fail (strokes != NULL, FALSE);

  coords.x        = strokes[0];
  coords.y        = strokes[1];
  coords.pressure = 1.0;
  coords.xtilt    = 0.5;
  coords.ytilt    = 0.5;
  coords.wheel    = 0.5;

  if (gimp_paint_core_start (core, drawable, &coords))
    {
      gint i;

      core->start_coords = coords;
      core->last_coords  = coords;

      gimp_paint_core_paint (core, drawable, paint_options, INIT_PAINT);

      gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);

      for (i = 1; i < n_strokes; i++)
        {
          coords.x = strokes[i * 2 + 0];
          coords.y = strokes[i * 2 + 1];

          core->cur_coords = coords;

          gimp_paint_core_interpolate (core, drawable, paint_options);

          core->last_coords = core->cur_coords;
       }

      gimp_paint_core_paint (core, drawable, paint_options, FINISH_PAINT);

      gimp_paint_core_finish (core, drawable);

      gimp_paint_core_cleanup (core);

      return TRUE;
    }

  return FALSE;
}
