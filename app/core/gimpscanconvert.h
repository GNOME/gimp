/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_SCAN_CONVERT_H__
#define __LIGMA_SCAN_CONVERT_H__


LigmaScanConvert *
          ligma_scan_convert_new               (void);

LigmaScanConvert *
          ligma_scan_convert_new_from_boundary (const LigmaBoundSeg *bound_segs,
                                               gint                n_bound_segs,
                                               gint                offset_x,
                                               gint                offset_y);

void      ligma_scan_convert_free               (LigmaScanConvert   *sc);
void      ligma_scan_convert_set_pixel_ratio    (LigmaScanConvert   *sc,
                                                gdouble            ratio_xy);
void      ligma_scan_convert_set_clip_rectangle (LigmaScanConvert   *sc,
                                                gint               x,
                                                gint               y,
                                                gint               width,
                                                gint               height);
void      ligma_scan_convert_add_polyline       (LigmaScanConvert   *sc,
                                                guint              n_points,
                                                const LigmaVector2 *points,
                                                gboolean           closed);
void      ligma_scan_convert_add_bezier         (LigmaScanConvert      *sc,
                                                const LigmaBezierDesc *bezier);
void      ligma_scan_convert_stroke             (LigmaScanConvert   *sc,
                                                gdouble            width,
                                                LigmaJoinStyle      join,
                                                LigmaCapStyle       cap,
                                                gdouble            miter,
                                                gdouble            dash_offset,
                                                GArray            *dash_info);
void      ligma_scan_convert_render_full        (LigmaScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gboolean           replace,
                                                gboolean           antialias,
                                                gdouble            value);

void      ligma_scan_convert_render             (LigmaScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gboolean           antialias);
void      ligma_scan_convert_render_value       (LigmaScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gdouble            value);
void      ligma_scan_convert_compose            (LigmaScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y);
void      ligma_scan_convert_compose_value      (LigmaScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gdouble            value);


#endif /* __LIGMA_SCAN_CONVERT_H__ */
