/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_SCAN_CONVERT_H__
#define __GIMP_SCAN_CONVERT_H__


/* Create a new scan conversion context.
 */
GimpScanConvert * gimp_scan_convert_new        (void);

void              gimp_scan_convert_free       (GimpScanConvert *scan_converter);

/* set the Pixel-Ratio (width / height) for the pixels.
 */
void         gimp_scan_convert_set_pixel_ratio (GimpScanConvert *sc,
                                                gdouble          ratio_xy);

/* Add "npoints" from "pointlist" to the polygon currently being
 * described by "scan_converter". DEPRECATED.
 */
void              gimp_scan_convert_add_points (GimpScanConvert *scan_converter,
                                                guint            n_points,
                                                GimpVector2     *points,
                                                gboolean         new_polygon);

/* Add a polygon with "npoints" "points" that may be open or closed.
 * It is not recommended to mix gimp_scan_convert_add_polyline with
 * gimp_scan_convert_add_points.
 *
 * Please note that you should use gimp_scan_convert_stroke() if you
 * specify open polygons.
 */
void              gimp_scan_convert_add_polyline (GimpScanConvert *sc,
                                                  guint            n_points,
                                                  GimpVector2     *points,
                                                  gboolean         closed);

/* Stroke the content of a GimpScanConvert. The next
 * gimp_scan_convert_to_channel will result in the outline of the polygon
 * defined with the commands above.
 *
 * You cannot add additional polygons after this command.
 *
 * Note that if you have nonstandard resolution, "width" gives the
 * width (in pixels) for a vertical stroke, i.e. use the X-resolution
 * to calculate the width of a stroke when operating with real world
 * units.
 */

void              gimp_scan_convert_stroke     (GimpScanConvert *sc,
                                                gdouble          width,
                                                GimpJoinStyle    join,
                                                GimpCapStyle     cap,
                                                gdouble          miter,
                                                gdouble          dash_offset,
                                                GArray          *dash_info);


/* This is a more low level version. Expects a tile manager of depth 1.
 *
 * You cannot add additional polygons after this command.
 */
void              gimp_scan_convert_render     (GimpScanConvert *scan_converter,
                                                TileManager     *tile_manager,
                                                gint             off_x,
                                                gint             off_y,
                                                gboolean         antialias);


#endif /* __GIMP_SCAN_CONVERT_H__ */
