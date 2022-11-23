/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaanchor.c
 * Copyright (C) 2002 Simon Budig  <simon@ligma.org>
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

#include <glib-object.h>

#include "vectors-types.h"

#include "ligmaanchor.h"


G_DEFINE_BOXED_TYPE (LigmaAnchor, ligma_anchor, ligma_anchor_copy, ligma_anchor_free)

LigmaAnchor *
ligma_anchor_new (LigmaAnchorType    type,
                 const LigmaCoords *position)
{
  LigmaAnchor *anchor = g_slice_new0 (LigmaAnchor);

  anchor->type = type;

  if (position)
    anchor->position = *position;

  return anchor;
}

LigmaAnchor *
ligma_anchor_copy (const LigmaAnchor *anchor)
{
  g_return_val_if_fail (anchor != NULL, NULL);

  return g_slice_dup (LigmaAnchor, anchor);
}

void
ligma_anchor_free (LigmaAnchor *anchor)
{
  g_return_if_fail (anchor != NULL);

  g_slice_free (LigmaAnchor, anchor);
}
