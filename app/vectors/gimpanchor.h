/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaanchor.h
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

#ifndef __LIGMA_ANCHOR_H__
#define __LIGMA_ANCHOR_H__

#define LIGMA_ANCHOR(anchor)  ((LigmaAnchor *) (anchor))

#define LIGMA_TYPE_ANCHOR               (ligma_anchor_get_type ())
#define LIGMA_VALUE_HOLDS_ANCHOR(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_ANCHOR))

GType   ligma_anchor_get_type           (void) G_GNUC_CONST;


struct _LigmaAnchor
{
  LigmaCoords        position;

  LigmaAnchorType    type;   /* Interpretation dependent on LigmaStroke type */
  gboolean          selected;
};


LigmaAnchor  * ligma_anchor_new  (LigmaAnchorType    type,
                                const LigmaCoords *position);

LigmaAnchor  * ligma_anchor_copy (const LigmaAnchor *anchor);
void          ligma_anchor_free (LigmaAnchor       *anchor);


#endif /* __LIGMA_ANCHOR_H__ */
