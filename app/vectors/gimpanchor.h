/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpanchor.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_ANCHOR_H__
#define __GIMP_ANCHOR_H__

#define GIMP_ANCHOR(anchor)  ((GimpAnchor *) (anchor))

#define GIMP_TYPE_ANCHOR               (gimp_anchor_get_type ())
#define GIMP_VALUE_HOLDS_ANCHOR(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_ANCHOR))

GType   gimp_anchor_get_type           (void) G_GNUC_CONST;


struct _GimpAnchor
{
  GimpCoords        position;

  GimpAnchorType    type;   /* Interpretation dependant on GimpStroke type */
  gboolean          selected;
};


GimpAnchor  * gimp_anchor_new       (GimpAnchorType    type,
                                     const GimpCoords *position);
void          gimp_anchor_free      (GimpAnchor       *anchor);
GimpAnchor  * gimp_anchor_duplicate (const GimpAnchor *anchor);


#endif /* __GIMP_ANCHOR_H__ */
