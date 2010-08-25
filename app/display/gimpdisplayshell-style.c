/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdisplayshell-style.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpcontext.h"
#include "core/gimpgrid.h"
#include "core/gimplayermask.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-style.h"


static const GimpRGB guide_normal_fg     = { 0.0, 0.0, 0.0, 1.0 };
static const GimpRGB guide_normal_bg     = { 0.0, 0.5, 1.0, 1.0 };
static const GimpRGB guide_active_fg     = { 0.0, 0.0, 0.0, 1.0 };
static const GimpRGB guide_active_bg     = { 1.0, 0.0, 0.0, 1.0 };

static const GimpRGB sample_point_normal = { 0.0, 0.5, 1.0, 1.0 };
static const GimpRGB sample_point_active = { 1.0, 0.0, 0.0, 1.0 };

static const GimpRGB layer_fg            = { 0.0, 0.0, 0.0, 1.0 };
static const GimpRGB layer_bg            = { 1.0, 1.0, 0.0, 1.0 };

static const GimpRGB layer_group_fg      = { 0.0, 0.0, 0.0, 1.0 };
static const GimpRGB layer_group_bg      = { 0.0, 1.0, 1.0, 1.0 };

static const GimpRGB layer_mask_fg       = { 0.0, 0.0, 0.0, 1.0 };
static const GimpRGB layer_mask_bg       = { 0.0, 1.0, 0.0, 1.0 };


/*  local function prototypes  */

static void   gimp_display_shell_set_stipple_style (cairo_t       *cr,
                                                    const GimpRGB *fg,
                                                    const GimpRGB *bg);


/*  public functions  */

void
gimp_display_shell_set_guide_style (GimpDisplayShell *shell,
                                    cairo_t          *cr,
                                    gboolean          active)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);

  if (active)
    gimp_display_shell_set_stipple_style (cr,
                                          &guide_active_fg,
                                          &guide_active_bg);
  else
    gimp_display_shell_set_stipple_style (cr,
                                          &guide_normal_fg,
                                          &guide_normal_bg);
}

void
gimp_display_shell_set_sample_point_style (GimpDisplayShell *shell,
                                           cairo_t          *cr,
                                           gboolean          active)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);

  if (active)
    cairo_set_source_rgb (cr,
                          sample_point_active.r,
                          sample_point_active.g,
                          sample_point_active.b);
  else
    cairo_set_source_rgb (cr,
                          sample_point_normal.r,
                          sample_point_normal.g,
                          sample_point_normal.b);
}

void
gimp_display_shell_set_grid_style (GimpDisplayShell *shell,
                                   cairo_t          *cr,
                                   GimpGrid         *grid)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_GRID (grid));

  cairo_set_line_width (cr, 1.0);

  switch (grid->style)
    {
    case GIMP_GRID_ON_OFF_DASH:
    case GIMP_GRID_DOUBLE_DASH:
      if (grid->style == GIMP_GRID_DOUBLE_DASH)
        {
          gimp_display_shell_set_stipple_style (cr,
                                                &grid->fgcolor,
                                                &grid->bgcolor);
        }
      else
        {
          GimpRGB bg = { 0.0, 0.0, 0.0, 0.0 };

          gimp_display_shell_set_stipple_style (cr,
                                                &grid->fgcolor,
                                                &bg);
        }
      break;

    case GIMP_GRID_DOTS:
    case GIMP_GRID_INTERSECTIONS:
    case GIMP_GRID_SOLID:
      cairo_set_source_rgb (cr,
                            grid->fgcolor.r,
                            grid->fgcolor.g,
                            grid->fgcolor.b);
      break;
    }
}

void
gimp_display_shell_set_cursor_style (GimpDisplayShell *shell,
                                     cairo_t          *cr)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
}

void
gimp_display_shell_set_pen_style (GimpDisplayShell *shell,
                                  cairo_t          *cr,
                                  GimpContext      *context,
                                  GimpActiveColor   active,
                                  gint              width)
{
  GimpRGB rgb;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width (cr, width);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  switch (active)
    {
    case GIMP_ACTIVE_COLOR_FOREGROUND:
      gimp_context_get_foreground (context, &rgb);
      break;

    case GIMP_ACTIVE_COLOR_BACKGROUND:
      gimp_context_get_background (context, &rgb);
      break;
    }

  cairo_set_source_rgb (cr, rgb.r, rgb.g, rgb.b);
}

void
gimp_display_shell_set_layer_style (GimpDisplayShell *shell,
                                    cairo_t          *cr,
                                    GimpDrawable     *drawable)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  cairo_set_line_width (cr, 1.0);

  if (GIMP_IS_LAYER_MASK (drawable))
    {
      gimp_display_shell_set_stipple_style (cr,
                                            &layer_mask_fg,
                                            &layer_mask_bg);
    }
  else if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      gimp_display_shell_set_stipple_style (cr,
                                            &layer_group_fg,
                                            &layer_group_bg);
    }
  else
    {
      gimp_display_shell_set_stipple_style (cr,
                                            &layer_fg,
                                            &layer_bg);
    }
}


/*  private functions  */

static cairo_user_data_key_t surface_data_key = { 0, };

static void
gimp_display_shell_set_stipple_style (cairo_t       *cr,
                                      const GimpRGB *fg,
                                      const GimpRGB *bg)
{
  cairo_surface_t *surface;
  guchar          *data = g_malloc0 (8 * 8 * 4);
  guchar           fg_r, fg_g, fg_b, fg_a;
  guchar           bg_r, bg_g, bg_b, bg_a;
  gint             x, y;
  guchar          *d;

  gimp_rgba_get_uchar (fg, &fg_r, &fg_g, &fg_b, &fg_a);
  gimp_rgba_get_uchar (bg, &bg_r, &bg_g, &bg_b, &bg_a);

  d = data;

  for (y = 0; y < 8; y++)
    {
      for (x = 0; x < 8; x++)
        {
          if ((y < 4 && x < 4) || (y >= 4 && x >= 4))
            GIMP_CAIRO_ARGB32_SET_PIXEL (d, fg_r, fg_g, fg_b, fg_a);
          else
            GIMP_CAIRO_ARGB32_SET_PIXEL (d, bg_r, bg_g, bg_b, bg_a);

          d += 4;
        }
    }

  surface = cairo_image_surface_create_for_data (data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 8, 8, 8 * 4);
  cairo_surface_set_user_data (surface, &surface_data_key,
                               data, (cairo_destroy_func_t) g_free);

  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_surface_destroy (surface);

  cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
}
