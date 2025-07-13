/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvas-style.c
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp-cairo.h"
#include "core/gimpgrid.h"
#include "core/gimplayer.h"

#include "path/gimpvectorlayer.h"

#include "gimpcanvas.h"
#include "gimpcanvas-style.h"

/* Styles for common and custom guides. */
static GeglColor *guide_normal_fg;
static GeglColor *guide_normal_bg;
static GeglColor *guide_active_fg;
static GeglColor *guide_active_bg;

static GeglColor *guide_mirror_normal_fg;
static GeglColor *guide_mirror_normal_bg;
static GeglColor *guide_mirror_active_fg;
static GeglColor *guide_mirror_active_bg;

static GeglColor *guide_mandala_normal_fg;
static GeglColor *guide_mandala_normal_bg;
static GeglColor *guide_mandala_active_fg;
static GeglColor *guide_mandala_active_bg;

static GeglColor *guide_split_normal_fg;
static GeglColor *guide_split_normal_bg;
static GeglColor *guide_split_active_fg;
static GeglColor *guide_split_active_bg;

/* Styles for other canvas items. */
static GeglColor *sample_point_normal;
static GeglColor *sample_point_active;

static GeglColor *layer_fg;
static GeglColor *layer_bg;

static GeglColor *layer_group_fg;
static GeglColor *layer_group_bg;

static GeglColor *layer_mask_fg;
static GeglColor *layer_mask_bg;

static GeglColor *canvas_fg;
static GeglColor *canvas_bg;

static GeglColor *selection_out_fg;
static GeglColor *selection_out_bg;

static GeglColor *selection_in_fg;
static GeglColor *selection_in_bg;

static GeglColor *path_normal_bg;
static GeglColor *path_normal_fg;

static GeglColor *path_active_bg;
static GeglColor *path_active_fg;

static GeglColor *outline_bg;
static GeglColor *outline_fg;

static GeglColor *passe_partout;

static GeglColor *tool_bg;
static GeglColor *tool_fg;
static GeglColor *tool_fg_highlight;


/*  public functions  */


void
gimp_canvas_styles_init (void)
{
  const Babl *format   = babl_format ("R'G'B'A double");
  gdouble     rgba1[4] = { 0.0, 0.8, 1.0, 1.0 };
  gdouble     rgba2[4] = { 1.0, 0.5, 0.0, 1.0 };
  gdouble     rgba3[4] = { 1.0, 0.8, 0.2, 0.8 };

  /* Styles for common and custom guides. */
  guide_normal_fg         = gegl_color_new ("black");
  guide_normal_bg         = gegl_color_new (NULL);
  gegl_color_set_pixel (guide_normal_bg, format, rgba1);
  guide_active_fg         = gegl_color_new ("black");
  guide_active_bg         = gegl_color_new ("red");

  guide_mirror_normal_fg  = gegl_color_new ("white");
  guide_mirror_normal_bg  = gegl_color_new ("lime");
  guide_mirror_active_fg  = gegl_color_new ("lime");
  guide_mirror_active_bg  = gegl_color_new ("red");

  guide_mandala_normal_fg = gegl_color_new ("white");
  guide_mandala_normal_bg = gegl_color_new ("aqua");
  guide_mandala_active_fg = gegl_color_new ("aqua");
  guide_mandala_active_bg = gegl_color_new ("red");

  guide_split_normal_fg   = gegl_color_new ("white");
  guide_split_normal_bg   = gegl_color_new ("fuchsia");
  guide_split_active_fg   = gegl_color_new ("fuchsia");
  guide_split_active_bg   = gegl_color_new ("red");

  /* Styles for other canvas items. */
  sample_point_normal     = gegl_color_new (NULL);
  gegl_color_set_pixel (sample_point_normal, format, rgba1);
  sample_point_active     = gegl_color_new ("red");

  layer_fg                = gegl_color_new ("black");
  layer_bg                = gegl_color_new ("yellow");

  layer_group_fg          = gegl_color_new ("black");
  layer_group_bg          = gegl_color_new ("aqua");

  layer_mask_fg           = gegl_color_new ("black");
  layer_mask_bg           = gegl_color_new ("lime");

  canvas_fg               = gegl_color_new ("black");
  canvas_bg               = gegl_color_new (NULL);
  gegl_color_set_pixel (canvas_bg, format, rgba2);

  selection_out_fg        = gegl_color_new ("white");
  selection_out_bg        = gegl_color_new ("gray");

  selection_in_fg         = gegl_color_new ("black");
  selection_in_bg         = gegl_color_new ("white");

  path_normal_bg          = gegl_color_new ("white");
  gimp_color_set_alpha (path_normal_bg, 0.6);
  path_normal_fg          = gegl_color_new ("blue");
  gimp_color_set_alpha (path_normal_fg, 0.8);

  path_active_bg          = gegl_color_new ("white");
  gimp_color_set_alpha (path_active_bg, 0.6);
  path_active_fg          = gegl_color_new ("red");
  gimp_color_set_alpha (path_active_fg, 0.8);

  outline_bg              = gegl_color_new ("white");
  gimp_color_set_alpha (outline_bg, 0.6);
  outline_fg              = gegl_color_new ("black");
  gimp_color_set_alpha (outline_fg, 0.8);

  passe_partout           = gegl_color_new ("black");

  tool_bg                 = gegl_color_new ("black");
  gimp_color_set_alpha (tool_bg, 0.4);
  tool_fg                 = gegl_color_new ("white");
  gimp_color_set_alpha (tool_fg, 0.8);
  tool_fg_highlight       = gegl_color_new (NULL);
  gegl_color_set_pixel (tool_fg_highlight, format, rgba3);
}

void
gimp_canvas_styles_exit (void)
{
  g_object_unref (guide_normal_fg);
  g_object_unref (guide_normal_bg);
  g_object_unref (guide_active_fg);
  g_object_unref (guide_active_bg);
  g_object_unref (guide_mirror_normal_fg);
  g_object_unref (guide_mirror_normal_bg);
  g_object_unref (guide_mirror_active_fg);
  g_object_unref (guide_mirror_active_bg);
  g_object_unref (guide_mandala_normal_fg);
  g_object_unref (guide_mandala_normal_bg);
  g_object_unref (guide_mandala_active_fg);
  g_object_unref (guide_mandala_active_bg);
  g_object_unref (guide_split_normal_fg);
  g_object_unref (guide_split_normal_bg);
  g_object_unref (guide_split_active_fg);
  g_object_unref (guide_split_active_bg);
  g_object_unref (sample_point_normal);
  g_object_unref (sample_point_active);
  g_object_unref (layer_fg);
  g_object_unref (layer_bg);
  g_object_unref (layer_group_fg);
  g_object_unref (layer_group_bg);
  g_object_unref (layer_mask_fg);
  g_object_unref (layer_mask_bg);
  g_object_unref (canvas_fg);
  g_object_unref (canvas_bg);
  g_object_unref (selection_out_fg);
  g_object_unref (selection_out_bg);
  g_object_unref (selection_in_fg);
  g_object_unref (selection_in_bg);
  g_object_unref (path_normal_bg);
  g_object_unref (path_normal_fg);
  g_object_unref (path_active_bg);
  g_object_unref (path_active_fg);
  g_object_unref (outline_bg);
  g_object_unref (outline_fg);
  g_object_unref (passe_partout);
  g_object_unref (tool_bg);
  g_object_unref (tool_fg);
  g_object_unref (tool_fg_highlight);
}

void
gimp_canvas_set_guide_style (GtkWidget      *canvas,
                             cairo_t        *cr,
                             GimpGuideStyle  style,
                             gboolean        active,
                             gdouble         offset_x,
                             gdouble         offset_y)
{
  const Babl      *render_space;
  GimpColorConfig *config = NULL;
  cairo_pattern_t *pattern;
  GeglColor       *normal_fg;
  GeglColor       *normal_bg;
  GeglColor       *active_fg;
  GeglColor       *active_bg;
  gdouble          line_width;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  switch (style)
    {
    case GIMP_GUIDE_STYLE_NORMAL:
      normal_fg  = guide_normal_fg;
      normal_bg  = guide_normal_bg;
      active_fg  = guide_active_fg;
      active_bg  = guide_active_bg;
      line_width = 1.0;
      break;

    case GIMP_GUIDE_STYLE_MIRROR:
      normal_fg  = guide_mirror_normal_fg;
      normal_bg  = guide_mirror_normal_bg;
      active_fg  = guide_mirror_active_fg;
      active_bg  = guide_mirror_active_bg;
      line_width = 1.0;
      break;

    case GIMP_GUIDE_STYLE_MANDALA:
      normal_fg  = guide_mandala_normal_fg;
      normal_bg  = guide_mandala_normal_bg;
      active_fg  = guide_mandala_active_fg;
      active_bg  = guide_mandala_active_bg;
      line_width = 1.0;
      break;

    case GIMP_GUIDE_STYLE_SPLIT_VIEW:
      normal_fg  = guide_split_normal_fg;
      normal_bg  = guide_split_normal_bg;
      active_fg  = guide_split_active_fg;
      active_bg  = guide_split_active_bg;
      line_width = 1.0;
      break;

    default: /* GIMP_GUIDE_STYLE_NONE */
      /* This should not happen. */
      g_return_if_reached ();
    }

  cairo_set_line_width (cr, line_width);

  if (GIMP_IS_CANVAS (canvas))
    config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  render_space = gimp_widget_get_render_space (canvas, config);
  if (active)
    pattern = gimp_cairo_pattern_create_stipple (active_fg, active_bg, 0,
                                                 offset_x, offset_y, render_space);
  else
    pattern = gimp_cairo_pattern_create_stipple (normal_fg, normal_bg, 0,
                                                 offset_x, offset_y, render_space);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
gimp_canvas_set_sample_point_style (GtkWidget *canvas,
                                    cairo_t   *cr,
                                    gboolean   active)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  if (active)
    gimp_cairo_set_source_color (cr, sample_point_active, config, FALSE, canvas);
  else
    gimp_cairo_set_source_color (cr, sample_point_normal, config, FALSE, canvas);
}

void
gimp_canvas_set_grid_style (GtkWidget *canvas,
                            cairo_t   *cr,
                            GimpGrid  *grid,
                            gdouble    offset_x,
                            gdouble    offset_y)
{
  const Babl      *render_space;
  GimpColorConfig *config = NULL;
  GeglColor       *fg;
  GeglColor       *bg;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_GRID (grid));

  cairo_set_line_width (cr, 1.0);

  fg = gimp_grid_get_fgcolor (grid);

  if (GIMP_IS_CANVAS (canvas))
    config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  render_space = gimp_widget_get_render_space (canvas, config);
  switch (gimp_grid_get_style (grid))
    {
      cairo_pattern_t *pattern;

    case GIMP_GRID_ON_OFF_DASH:
    case GIMP_GRID_DOUBLE_DASH:
      if (grid->style == GIMP_GRID_DOUBLE_DASH)
        {
          bg = gimp_grid_get_bgcolor (grid);

          pattern = gimp_cairo_pattern_create_stipple (fg, bg, 0,
                                                       offset_x, offset_y,
                                                       render_space);
        }
      else
        {
          bg = gegl_color_new ("transparent");

          pattern = gimp_cairo_pattern_create_stipple (fg, bg, 0,
                                                       offset_x, offset_y,
                                                       render_space);
          g_clear_object (&bg);
        }

      cairo_set_source (cr, pattern);
      cairo_pattern_destroy (pattern);
      break;

    case GIMP_GRID_DOTS:
    case GIMP_GRID_INTERSECTIONS:
    case GIMP_GRID_SOLID:
      gimp_cairo_set_source_color (cr, fg, config, FALSE, canvas);
      break;
    }
}

void
gimp_canvas_set_pen_style (GtkWidget *canvas,
                           cairo_t   *cr,
                           GeglColor *color,
                           gint       width)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GEGL_IS_COLOR (color));

  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width (cr, width);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  gimp_cairo_set_source_color (cr, color, config, FALSE, canvas);
}

void
gimp_canvas_set_layer_style (GtkWidget *canvas,
                             cairo_t   *cr,
                             GimpLayer *layer,
                             gdouble    offset_x,
                             gdouble    offset_y)
{
  const Babl      *render_space;
  GimpColorConfig *config = NULL;
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_LAYER (layer));

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  if (GIMP_IS_CANVAS (canvas))
    config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  render_space = gimp_widget_get_render_space (canvas, config);
  if (gimp_layer_get_mask (layer) &&
      gimp_layer_get_edit_mask (layer))
    {
      pattern = gimp_cairo_pattern_create_stipple (layer_mask_fg,
                                                   layer_mask_bg,
                                                   0,
                                                   offset_x, offset_y, render_space);
    }
  else if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      pattern = gimp_cairo_pattern_create_stipple (layer_group_fg, layer_group_bg, 0,
                                                   offset_x, offset_y, render_space);
    }
  else if (gimp_item_is_vector_layer (GIMP_ITEM (layer)))
    {
      GeglColor *transparent = gegl_color_new ("transparent");

      pattern = gimp_cairo_pattern_create_stipple (transparent, transparent, 0,
                                                   offset_x, offset_y, render_space);

      g_clear_object (&transparent);
    }
  else
    {
      pattern = gimp_cairo_pattern_create_stipple (layer_fg, layer_bg, 0,
                                                   offset_x, offset_y, render_space);
    }

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
gimp_canvas_set_canvas_style (GtkWidget *canvas,
                              cairo_t   *cr,
                              gdouble    offset_x,
                              gdouble    offset_y)
{
  const Babl      *render_space;
  GimpColorConfig *config = NULL;
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  if (GIMP_IS_CANVAS (canvas))
    config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  render_space = gimp_widget_get_render_space (canvas, config);
  pattern = gimp_cairo_pattern_create_stipple (canvas_fg, canvas_bg, 0,
                                               offset_x, offset_y, render_space);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
gimp_canvas_set_selection_out_style (GtkWidget *canvas,
                                     cairo_t   *cr,
                                     gdouble    offset_x,
                                     gdouble    offset_y)
{
  const Babl      *render_space;
  GimpColorConfig *config = NULL;
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  if (GIMP_IS_CANVAS (canvas))
    config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  render_space = gimp_widget_get_render_space (canvas, config);
  pattern = gimp_cairo_pattern_create_stipple (selection_out_fg, selection_out_bg, 0,
                                               offset_x, offset_y, render_space);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
gimp_canvas_set_selection_in_style (GtkWidget *canvas,
                                    cairo_t   *cr,
                                    gint       index,
                                    gdouble    offset_x,
                                    gdouble    offset_y)
{
  const Babl      *render_space;
  GimpColorConfig *config = NULL;
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  if (GIMP_IS_CANVAS (canvas))
    config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  render_space = gimp_widget_get_render_space (canvas, config);
  pattern = gimp_cairo_pattern_create_stipple (selection_in_fg, selection_in_bg, index,
                                               offset_x, offset_y, render_space);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
gimp_canvas_set_path_bg_style (GtkWidget *canvas,
                               cairo_t   *cr,
                               gboolean   active)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 3.0);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  if (active)
    gimp_cairo_set_source_color (cr, path_active_bg, config, FALSE, canvas);
  else
    gimp_cairo_set_source_color (cr, path_normal_bg, config, FALSE, canvas);
}

void
gimp_canvas_set_path_fg_style (GtkWidget *canvas,
                               cairo_t   *cr,
                               gboolean   active)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  if (active)
    gimp_cairo_set_source_color (cr, path_active_fg, config, FALSE, canvas);
  else
    gimp_cairo_set_source_color (cr, path_normal_fg, config, FALSE, canvas);
}

void
gimp_canvas_set_outline_bg_style (GtkWidget *canvas,
                                  cairo_t   *cr)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  gimp_cairo_set_source_color (cr, outline_bg, config, FALSE, canvas);
}

void
gimp_canvas_set_outline_fg_style (GtkWidget *canvas,
                                  cairo_t   *cr)
{
  static const double  dashes[] = { 4.0, 4.0 };
  GimpColorConfig     *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  gimp_cairo_set_source_color (cr, outline_fg, config, FALSE, canvas);
  cairo_set_dash (cr, dashes, G_N_ELEMENTS (dashes), 0);
}

void
gimp_canvas_set_passe_partout_style (GtkWidget *canvas,
                                     cairo_t   *cr)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  gimp_cairo_set_source_color (cr, passe_partout, config, FALSE, canvas);
}

void
gimp_canvas_set_tool_bg_style (GtkWidget *canvas,
                               cairo_t   *cr)
{
  GimpColorConfig *config;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 3.0);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  gimp_cairo_set_source_color (cr, tool_bg, config, FALSE, canvas);
}

void
gimp_canvas_set_tool_fg_style (GtkWidget *canvas,
                               cairo_t   *cr,
                               gboolean   highlight)
{
  GimpColorConfig *config;

  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  config = GIMP_CORE_CONFIG (GIMP_CANVAS (canvas)->config)->color_management;
  if (highlight)
    gimp_cairo_set_source_color (cr, tool_fg_highlight, config, FALSE, canvas);
  else
    gimp_cairo_set_source_color (cr, tool_fg, config, FALSE, canvas);
}
