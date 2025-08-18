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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "path-types.h"

#include "libgimpmath/gimpmath.h"

#include "core/gimpimage.h"
#include "core/gimptempbuf.h"

#include "gimpstroke.h"
#include "gimppath.h"
#include "gimppath-preview.h"


/*  public functions  */

GimpTempBuf *
gimp_path_get_new_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height,
                           GeglColor    *fg_color G_GNUC_UNUSED)
{
  GimpPath    *path;
  GimpItem    *item;
  GimpStroke  *cur_stroke;
  gdouble      xscale, yscale;
  guchar      *data;
  GimpTempBuf *temp_buf;

  path = GIMP_PATH (viewable);
  item = GIMP_ITEM (viewable);

  xscale = ((gdouble) width)  / gimp_image_get_width  (gimp_item_get_image (item));
  yscale = ((gdouble) height) / gimp_image_get_height (gimp_item_get_image (item));

  temp_buf = gimp_temp_buf_new (width, height, babl_format ("Y' u8"));
  data = gimp_temp_buf_get_data (temp_buf);
  memset (data, 255, width * height);

  for (cur_stroke = gimp_path_stroke_get_next (path, NULL);
       cur_stroke;
       cur_stroke = gimp_path_stroke_get_next (path, cur_stroke))
    {
      GArray   *coords;
      gboolean  closed;
      gint      i;

      coords = gimp_stroke_interpolate (cur_stroke, 0.5, &closed);

      if (coords)
        {
          for (i = 0; i < coords->len; i++)
            {
              GimpCoords point;
              gint       x, y;

              point = g_array_index (coords, GimpCoords, i);

              x = ROUND (point.x * xscale);
              y = ROUND (point.y * yscale);

              if (x >= 0 && y >= 0 && x < width && y < height)
                data[y * width + x] = 0;
            }

          g_array_free (coords, TRUE);
        }
    }

  return temp_buf;
}
