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

#ifndef __GIMP_ADAPTIVE_SUPERSAMPLE_H__
#define __GIMP_ADAPTIVE_SUPERSAMPLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  adaptive supersampling function taken from LibGCK  */


gulong   gimp_adaptive_supersample_area (gint              x1,
                                         gint              y1,
                                         gint              x2,
                                         gint              y2,
                                         gint              max_depth,
                                         gdouble           threshold,
                                         GimpRenderFunc    render_func,
                                         gpointer          render_data,
                                         GimpPutPixelFunc  put_pixel_func,
                                         gpointer          put_pixel_data,
                                         GimpProgressFunc  progress_func,
                                         gpointer          progress_data);


G_END_DECLS

#endif  /* __GIMP_ADAPTIVE_SUPERSAMPLE_H__ */
