/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdisplayshell-style.h
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CANVAS_STYLE_H__
#define __GIMP_CANVAS_STYLE_H__


void   gimp_canvas_styles_init             (void);
void   gimp_canvas_styles_exit             (void);

void   gimp_canvas_set_guide_style         (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            GimpGuideStyle style,
                                            gboolean       active,
                                            gdouble        offset_x,
                                            gdouble        offset_y);
void   gimp_canvas_set_sample_point_style  (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gboolean       active);
void   gimp_canvas_set_grid_style          (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            GimpGrid      *grid,
                                            gdouble        offset_x,
                                            gdouble        offset_y);
void   gimp_canvas_set_pen_style           (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            GeglColor     *color,
                                            gint           width);
void   gimp_canvas_set_layer_style         (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            GimpLayer     *layer,
                                            gdouble        offset_x,
                                            gdouble        offset_y);
void   gimp_canvas_set_canvas_style        (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gdouble        offset_x,
                                            gdouble        offset_y);
void   gimp_canvas_set_selection_out_style (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gdouble        offset_x,
                                            gdouble        offset_y);
void   gimp_canvas_set_selection_in_style  (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gint           index,
                                            gdouble        offset_x,
                                            gdouble        offset_y);
void   gimp_canvas_set_vectors_bg_style    (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gboolean       active);
void   gimp_canvas_set_vectors_fg_style    (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gboolean       active);
void   gimp_canvas_set_outline_bg_style    (GtkWidget     *canvas,
                                            cairo_t       *cr);
void   gimp_canvas_set_outline_fg_style    (GtkWidget     *canvas,
                                            cairo_t       *cr);
void   gimp_canvas_set_passe_partout_style (GtkWidget     *canvas,
                                            cairo_t       *cr);

void   gimp_canvas_set_tool_bg_style       (GtkWidget     *canvas,
                                            cairo_t       *cr);
void   gimp_canvas_set_tool_fg_style       (GtkWidget     *canvas,
                                            cairo_t       *cr,
                                            gboolean       highlight);


#endif /* __GIMP_CANVAS_STYLE_H__ */
