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

#include "paint-types.h"

#include "core/gimpdrawable.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimppaintcore.h"
#include "gimppaintcore-stroke.h"
#include "gimppaintoptions.h"


gboolean
gimp_paint_core_stroke (GimpPaintCore    *core,
                        GimpDrawable     *drawable,
                        GimpPaintOptions *paint_options,
                        GimpCoords       *strokes,
                        gint              n_strokes)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (strokes != NULL, FALSE);
  g_return_val_if_fail (n_strokes > 0, FALSE);

  if (gimp_paint_core_start (core, drawable, paint_options, &strokes[0]))
    {
      GimpBrush *current_brush;
      gint i;

      core->start_coords = strokes[0];
      core->last_coords  = strokes[0];

      gimp_paint_core_paint (core, drawable, paint_options, INIT_PAINT);

      current_brush = core->brush;
      gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);
      core->brush = current_brush;

      for (i = 1; i < n_strokes; i++)
        {
          core->cur_coords = strokes[i];

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

gboolean
gimp_paint_core_stroke_vectors (GimpPaintCore    *core,
                                GimpDrawable     *drawable,
                                GimpPaintOptions *paint_options,
                                GimpVectors      *vectors)
{
  GList      *stroke;
  GArray     *coords      = NULL;
  gboolean    initialized = FALSE;
  gboolean    closed;
  gint        off_x, off_y;
  gint        vectors_off_x, vectors_off_y;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  gimp_item_offsets (GIMP_ITEM (vectors),  &vectors_off_x, &vectors_off_y);
  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  off_x -= vectors_off_x;
  off_y -= vectors_off_y;

  for (stroke = vectors->strokes; stroke; stroke = stroke->next)
    {
      gint i;

      coords = gimp_stroke_interpolate (GIMP_STROKE (stroke->data),
                                        1.0, &closed);

      /*  gimp_stroke_interpolate() may return NULL, so iterate over the
       *  list of strokes until one returns coords
       */
      if (! coords)
        continue;

      for (i = 0; i < coords->len; i++)
        {
          g_array_index (coords, GimpCoords, i).x -= off_x;
          g_array_index (coords, GimpCoords, i).y -= off_y;
        }

      if (! initialized)
        {
          if (! gimp_paint_core_start (core, drawable, paint_options,
                                       &(g_array_index (coords, GimpCoords, 0))))
            {
              g_array_free (coords, TRUE);
              return FALSE;
            }

          initialized = TRUE;
        }

      gimp_paint_core_paint (core, drawable, paint_options, INIT_PAINT);

      do
        {
          GimpBrush *current_brush;

          core->start_coords = g_array_index (coords, GimpCoords, 0);
          core->last_coords  = g_array_index (coords, GimpCoords, 0);

          current_brush = core->brush;
          gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);
          core->brush = current_brush;

          for (i = 1; i < coords->len; i++)
            {
              core->cur_coords = g_array_index (coords, GimpCoords, i);

              gimp_paint_core_interpolate (core, drawable, paint_options);

              core->last_coords = core->cur_coords;
            }

          if (closed)
            {
              core->cur_coords = g_array_index (coords, GimpCoords, 0);

              gimp_paint_core_interpolate (core, drawable, paint_options);

              core->last_coords = core->cur_coords;
            }

          g_array_free (coords, TRUE);
          coords = NULL;
        }
      while (coords);

      gimp_paint_core_paint (core, drawable, paint_options, FINISH_PAINT);
    }

  if (initialized)
    {
      gimp_paint_core_finish (core, drawable);

      gimp_paint_core_cleanup (core);
    }

  return TRUE;
}
