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

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/siox.h"
#include "base/tile-manager.h"

#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpdrawable-foreground-extract.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpprogress.h"
#include "gimp-utils.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_foreground_extract (GimpDrawable              *drawable,
                                  GimpForegroundExtractMode  mode,
                                  GimpDrawable              *mask,
                                  GimpProgress              *progress)
{
  const gint    smoothness     = SIOX_DEFAULT_SMOOTHNESS;
  const gdouble sensitivity[3] = { SIOX_DEFAULT_SENSITIVITY_L,
                                   SIOX_DEFAULT_SENSITIVITY_A,
                                   SIOX_DEFAULT_SENSITIVITY_B };

  g_return_if_fail (GIMP_IS_DRAWABLE (mask));
  g_return_if_fail (mode == GIMP_FOREGROUND_EXTRACT_SIOX);

  gimp_drawable_foreground_extract_siox (drawable, mask,
                                         0, 0,
                                         gimp_item_width (GIMP_ITEM (mask)),
                                         gimp_item_height (GIMP_ITEM (mask)),
                                         smoothness, sensitivity,
                                         progress);
}

void
gimp_drawable_foreground_extract_siox (GimpDrawable  *drawable,
                                       GimpDrawable  *mask,
                                       gint           x,
                                       gint           y,
                                       gint           width,
                                       gint           height,
                                       gint           smoothness,
                                       const gdouble  sensitivity[3],
                                       GimpProgress  *progress)
{
  GimpImage    *gimage;
  const guchar *colormap = NULL;
  gboolean      intersect;
  gint          offset_x;
  gint          offset_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  g_return_if_fail (GIMP_IS_DRAWABLE (mask));
  g_return_if_fail (gimp_drawable_bytes (mask) == 1);

  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_image_base_type (gimage) == GIMP_INDEXED)
    colormap = gimp_image_get_colormap (gimage);

  gimp_item_offsets (GIMP_ITEM (drawable), &offset_x, &offset_y);

  intersect = gimp_rectangle_intersect (offset_x, offset_y,
                                        gimp_item_width (GIMP_ITEM (drawable)),
                                        gimp_item_height (GIMP_ITEM (drawable)),
                                        x, y, width, height,
                                        &x, &y, &width, &height);


  /* FIXME:
   * Clear the mask outside the rectangle that we are working on?
   */

  if (! intersect)
    return;

  if (progress)
    gimp_progress_start (progress, _("Foreground Extraction..."), FALSE);

  siox_foreground_extract (gimp_drawable_data (drawable), colormap,
                           offset_x, offset_y,
                           gimp_drawable_data (mask), x, y, width, height,
                           smoothness, sensitivity,
                           (SioxProgressFunc) gimp_progress_set_value,
                           progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_drawable_update (mask,
                        0, 0,
                        gimp_item_width (GIMP_ITEM (mask)),
                        gimp_item_height (GIMP_ITEM (mask)));
}
