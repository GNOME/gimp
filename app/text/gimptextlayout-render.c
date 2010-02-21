/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <pango/pangocairo.h>

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimptext.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"


void
gimp_text_layout_render (GimpTextLayout    *layout,
                         cairo_t           *cr,
                         GimpTextDirection  base_dir,
                         gboolean           path)
{
  PangoLayout    *pango_layout;
  cairo_matrix_t  trafo;
  gint            x, y;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (cr != NULL);

  gimp_text_layout_get_offsets (layout, &x, &y);

  pango_layout = gimp_text_layout_get_pango_layout (layout);

  /* If the width of the layout is > 0, then the text-box is FIXED and
   * the layout position should be offset if the alignment is centered
   * or right-aligned, also adjust for RTL text direction.
   */
  if (pango_layout_get_width (pango_layout) > 0)
    {
      PangoAlignment align = pango_layout_get_alignment (pango_layout);
      gint           width;

      pango_layout_get_pixel_size (pango_layout, &width, NULL);

      if ((base_dir == GIMP_TEXT_DIRECTION_LTR && align == PANGO_ALIGN_RIGHT) ||
          (base_dir == GIMP_TEXT_DIRECTION_RTL && align == PANGO_ALIGN_LEFT))
        {
          x += PANGO_PIXELS (pango_layout_get_width (pango_layout)) - width;
        }
      else if (align == PANGO_ALIGN_CENTER)
        {
          x += (PANGO_PIXELS (pango_layout_get_width (pango_layout))
                - width) / 2;
       }
    }

  cairo_translate (cr, x, y);

  gimp_text_layout_get_transform (layout, &trafo);
  cairo_transform (cr, &trafo);

  if (path)
    pango_cairo_layout_path (cr, pango_layout);
  else
    pango_cairo_show_layout (cr, pango_layout);
}
