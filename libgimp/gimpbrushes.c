/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpbrushes.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include "gimp.h"

/**
 * gimp_brushes_get_opacity:
 *
 * Get the brush opacity.
 *
 * This procedure returns the opacity setting for brushes. This value
 * is set globally and will remain the same even if the brush mask is
 * changed. The return value is a floating point number between 0 and
 * 100.
 *
 * Returns: The brush opacity.
 */
gdouble
gimp_brushes_get_opacity (void)
{
  return gimp_context_get_opacity ();
}

/**
 * gimp_brushes_set_opacity:
 * @opacity: The brush opacity.
 *
 * Set the brush opacity.
 *
 * This procedure modifies the opacity setting for brushes. This value
 * is set globally and will remain the same even if the brush mask is
 * changed. The value should be a floating point number between 0 and
 * 100.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_brushes_set_opacity (gdouble opacity)
{
  return gimp_context_set_opacity (opacity);
}

/**
 * gimp_brushes_get_paint_mode:
 *
 * Get the brush paint mode.
 *
 * This procedure returns the paint-mode setting for brushes. This
 * value is set globally and will not change if a different brush is
 * selected. The return value is an integer which corresponds to the
 * values listed in the argument description.
 *
 * Returns: The paint mode.
 */
GimpLayerModeEffects
gimp_brushes_get_paint_mode (void)
{
  return gimp_context_get_paint_mode ();
}

/**
 * gimp_brushes_set_paint_mode:
 * @paint_mode: The paint mode.
 *
 * Set the brush paint mode.
 *
 * This procedure modifies the paint_mode setting for the current
 * brush. This value is set globally and will not change if a different
 * brush mask is selected.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_brushes_set_paint_mode (GimpLayerModeEffects paint_mode)
{
  return gimp_context_set_paint_mode (paint_mode);
}
