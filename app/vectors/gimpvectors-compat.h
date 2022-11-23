/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectors-compat.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VECTORS_COMPAT_H__
#define __LIGMA_VECTORS_COMPAT_H__


typedef struct _LigmaVectorsCompatPoint LigmaVectorsCompatPoint;

struct _LigmaVectorsCompatPoint
{
  guint32 type;
  gdouble x;
  gdouble y;
};


LigmaVectors * ligma_vectors_compat_new (LigmaImage              *image,
                                       const gchar            *name,
                                       LigmaVectorsCompatPoint *points,
                                       gint                    n_points,
                                       gboolean                closed);

gboolean              ligma_vectors_compat_is_compatible (LigmaImage   *image);

LigmaVectorsCompatPoint * ligma_vectors_compat_get_points (LigmaVectors *vectors,
                                                         gint32      *n_points,
                                                         gint32      *closed);


#endif /* __LIGMA_VECTORS_COMPAT_H__ */
