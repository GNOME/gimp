/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppath-compat.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PATH_COMPAT_H__
#define __GIMP_PATH_COMPAT_H__


typedef struct _GimpPathCompatPoint GimpPathCompatPoint;

struct _GimpPathCompatPoint
{
  guint32 type;
  gdouble x;
  gdouble y;
};


GimpPath *    gimp_path_compat_new (GimpImage              *image,
                                    const gchar            *name,
                                    GimpPathCompatPoint    *points,
                                    gint                    n_points,
                                    gboolean                closed);

gboolean              gimp_path_compat_is_compatible (GimpImage   *image);

GimpPathCompatPoint * gimp_path_compat_get_points    (GimpPath    *path,
                                                      gint32      *n_points,
                                                      gint32      *closed);


#endif /* __GIMP_PATH_COMPAT_H__ */
