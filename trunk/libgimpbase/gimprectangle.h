/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_RECTANGLE_H__
#define __GIMP_RECTANGLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean  gimp_rectangle_intersect (gint  x1,
                                    gint  y1,
                                    gint  width1,
                                    gint  height1,
                                    gint  x2,
                                    gint  y2,
                                    gint  width2,
                                    gint  height2,
                                    gint *dest_x,
                                    gint *dest_y,
                                    gint *dest_width,
                                    gint *dest_height);


G_END_DECLS

#endif  /* __GIMP_RECTANGLE_H__ */
