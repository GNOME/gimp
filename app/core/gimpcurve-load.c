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

#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmacurve.h"
#include "ligmacurve-load.h"


GList *
ligma_curve_load (GFile         *file,
                 GInputStream  *input,
                 GError       **error)
{
  LigmaCurve *curve;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  curve = g_object_new (LIGMA_TYPE_CURVE, NULL);

  if (ligma_config_deserialize_stream (LIGMA_CONFIG (curve),
                                      input,
                                      NULL, error))
    {
      return g_list_prepend (NULL, curve);
    }

  g_object_unref (curve);

  return NULL;
}
