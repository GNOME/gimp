/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * General, non-jittered adaptive supersampling library
 * Copyright (C) 1997 Federico Mena Quintero
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
 *
 */

#ifndef __ASUPSAMPLE_H__
#define __ASUPSAMPLE_H__


typedef void (* render_func_t)    (gdouble   x,
				   gdouble   y,
				   GimpRGB  *color,
				   gpointer  render_data);
typedef void (* put_pixel_func_t) (gint      x,
				   gint      y,
				   GimpRGB   color,
				   gpointer  put_pixel_data);


gulong   adaptive_supersample_area (gint              x1,
				    gint              y1,
				    gint              x2,
				    gint              y2,
				    gint              max_depth,
				    gdouble           threshold,
				    render_func_t     render_func,
				    gpointer          render_data,
				    put_pixel_func_t  put_pixel_func,
				    gpointer          put_pixel_data,
				    GimpProgressFunc  progress_func,
				    gpointer          progress_data);


#endif /* __ASUPSAMPLE_H__ */
