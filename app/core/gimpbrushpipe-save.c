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

#include "core-types.h"

#include "gimpbrushpipe.h"
#include "gimpbrushpipe-save.h"


gboolean
gimp_brush_pipe_save (GimpData       *data,
                      GOutputStream  *output,
                      GError        **error)
{
  GimpBrushPipe *pipe = GIMP_BRUSH_PIPE (data);
  const gchar   *name;
  gint           i;

  name = gimp_object_get_name (pipe);

  if (! g_output_stream_printf (output, NULL, NULL, error,
                                "%s\n%d %s\n",
                                name, pipe->n_brushes, pipe->params))
    {
      return FALSE;
    }

  for (i = 0; i < pipe->n_brushes; i++)
    {
      GimpBrush *brush = pipe->brushes[i];

      if (brush &&
          ! GIMP_DATA_GET_CLASS (brush)->save (GIMP_DATA (brush),
                                               output, error))
        {
          return FALSE;
        }
    }

  return TRUE;
}
