/* GIMP - The GNU Image Manipulation Program
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


GimpScanConvert * gimp_scan_convert_new        (void);

void      gimp_scan_convert_free               (GimpScanConvert *sc);
void      gimp_scan_convert_set_pixel_ratio    (GimpScanConvert *sc,
                                                gdouble          ratio_xy);
void      gimp_scan_convert_set_clip_rectangle (GimpScanConvert *sc,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height);
void      gimp_scan_convert_add_points         (GimpScanConvert *sc,
                                                guint            n_points,
                                                GimpVector2     *points,
                                                gboolean         new_polygon);
void      gimp_scan_convert_add_polyline       (GimpScanConvert *sc,
                                                guint            n_points,
                                                GimpVector2     *points,
                                                gboolean         closed);
void      gimp_scan_convert_stroke             (GimpScanConvert *sc,
                                                gdouble          width,
                                                GimpJoinStyle    join,
                                                GimpCapStyle     cap,
                                                gdouble          miter,
                                                gdouble          dash_offset,
                                                GArray          *dash_info);
void      gimp_scan_convert_render             (GimpScanConvert *sc,
                                                TileManager     *tile_manager,
                                                gint             off_x,
                                                gint             off_y,
                                                gboolean         antialias);
void      gimp_scan_convert_render_value       (GimpScanConvert *sc,
                                                TileManager     *tile_manager,
                                                gint             off_x,
                                                gint             off_y,
                                                guchar           value);
void      gimp_scan_convert_compose            (GimpScanConvert *sc,
                                                GimpChannelOps   op,
                                                TileManager     *tile_manager,
                                                gint             off_x,
                                                gint             off_y);


#endif /* __GIMP_SCAN_CONVERT_H__ */
