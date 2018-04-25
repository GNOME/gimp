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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimpdrawable.h"
#include "gimpdrawable-edit.h"
#include "gimpcontext.h"
#include "gimpfilloptions.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_edit_clear (GimpDrawable *drawable,
                          GimpContext  *context)
{
  GimpFillOptions *options;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  options = gimp_fill_options_new (context->gimp, NULL, FALSE);

  if (gimp_drawable_has_alpha (drawable))
    gimp_fill_options_set_by_fill_type (options, context,
                                        GIMP_FILL_TRANSPARENT, NULL);
  else
    gimp_fill_options_set_by_fill_type (options, context,
                                        GIMP_FILL_BACKGROUND, NULL);

  gimp_drawable_edit_fill (drawable, options, C_("undo-type", "Clear"));

  g_object_unref (options);
}

void
gimp_drawable_edit_fill (GimpDrawable    *drawable,
                         GimpFillOptions *options,
                         const gchar     *undo_desc)
{
  GeglBuffer  *buffer;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return;  /*  nothing to do, but the fill succeeded  */

  buffer = gimp_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height));

  if (! undo_desc)
    undo_desc = gimp_fill_options_get_undo_desc (options);

  gimp_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (0, 0, width, height),
                              TRUE, undo_desc,
                              gimp_context_get_opacity (GIMP_CONTEXT (options)),
                              gimp_context_get_paint_mode (GIMP_CONTEXT (options)),
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              gimp_layer_mode_get_paint_composite_mode (
                                gimp_context_get_paint_mode (GIMP_CONTEXT (options))),
                              NULL, x, y);

  g_object_unref (buffer);

  gimp_drawable_update (drawable, x, y, width, height);
}
