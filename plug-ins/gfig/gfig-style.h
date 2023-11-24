/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#ifndef __GFIG_STYLE_H__
#define __GFIG_STYLE_H__

#include <libgimp/gimpui.h>

struct _Style
{
  gchar     *name;

  GimpBrush    *brush;
  GimpPattern  *pattern;
  GimpGradient *gradient;

  gint       brush_width;
  gint       brush_height;
  gint       brush_spacing;
  BrushType  brush_type;
  gdouble    brushfade;
  gdouble    brushgradient;
  gdouble    airbrushpressure;
  FillType   fill_type;
  gdouble    fill_opacity;

  PaintType  paint_type;
  GeglColor *foreground;
  GeglColor *background;
  gboolean   reverselines;
  gint       ref_count;
};

gboolean gfig_load_style                   (Style                *style,
                                            FILE                 *fp);

gboolean gfig_skip_style                   (Style                *style,
                                            FILE                 *fp);

gboolean gfig_load_styles                  (GFigObj              *gfig,
                                            FILE                 *fp);

void     gfig_save_style                   (Style                *style,
                                            GString              *string);

void     gfig_style_save_as_attributes     (Style                *style,
                                            GString              *string);

void     gfig_save_styles                  (GString              *string);

void     set_foreground_callback           (GimpColorButton      *button,
                                            gpointer              data);

void     set_background_callback           (GimpColorButton      *button,
                                            gpointer              data);

void     set_paint_type_callback           (GtkToggleButton      *toggle,
                                            gpointer              data);

void     gfig_brush_changed_callback       (gpointer              user_data,
                                            GimpBrush            *brush,
                                            gboolean              dialog_closing);
void     gfig_pattern_changed_callback     (gpointer              user_data,
                                            GimpPattern          *pattern,
                                            gboolean              dialog_closing);
void     gfig_gradient_changed_callback    (gpointer              user_data,
                                            GimpGradient         *gradient,
                                            gboolean              dialog_closing);

void     gfig_style_copy                   (Style                *style1,
                                            Style                *style0,
                                            const gchar          *name);

void     gfig_style_apply                  (Style                *style);

void     gfig_read_gimp_style              (Style                *style,
                                            const gchar          *name);

void     gfig_style_set_context_from_style (Style                *style);

void     gfig_style_set_style_from_context (Style                *style);

void     set_context_bdesc                 (GimpBrush            *brush);

Style   *gfig_context_get_current_style    (void);

#endif /* __GFIG_STYLE_H__ */
