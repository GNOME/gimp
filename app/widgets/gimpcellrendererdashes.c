/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererdashes.c
 * Copyright (C) 2005 Sven Neumann <sven@gimp.org>
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

#include <config.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpdashpattern.h"

#include "gimpcellrendererdashes.h"


#define DASHES_WIDTH   96
#define DASHES_HEIGHT   4

#define N_SEGMENTS     24
#define BLOCK_WIDTH    (DASHES_WIDTH / (2 * N_SEGMENTS))


enum
{
  PROP_0,
  PROP_PATTERN
};


static void gimp_cell_renderer_dashes_finalize     (GObject            *object);
static void gimp_cell_renderer_dashes_get_property (GObject            *object,
                                                    guint               param_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void gimp_cell_renderer_dashes_set_property (GObject            *object,
                                                    guint               param_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void gimp_cell_renderer_dashes_get_size     (GtkCellRenderer    *cell,
                                                    GtkWidget          *widget,
                                                    const GdkRectangle *rectangle,
                                                    gint               *x_offset,
                                                    gint               *y_offset,
                                                    gint               *width,
                                                    gint               *height);
static void gimp_cell_renderer_dashes_render       (GtkCellRenderer    *cell,
                                                    cairo_t            *cr,
                                                    GtkWidget          *widget,
                                                    const GdkRectangle *background_area,
                                                    const GdkRectangle *cell_area,
                                                    GtkCellRendererState flags);


G_DEFINE_TYPE (GimpCellRendererDashes, gimp_cell_renderer_dashes,
               GTK_TYPE_CELL_RENDERER)

#define parent_class gimp_cell_renderer_dashes_parent_class


static void
gimp_cell_renderer_dashes_class_init (GimpCellRendererDashesClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize     = gimp_cell_renderer_dashes_finalize;
  object_class->get_property = gimp_cell_renderer_dashes_get_property;
  object_class->set_property = gimp_cell_renderer_dashes_set_property;

  cell_class->get_size       = gimp_cell_renderer_dashes_get_size;
  cell_class->render         = gimp_cell_renderer_dashes_render;

  g_object_class_install_property (object_class, PROP_PATTERN,
                                   g_param_spec_boxed ("pattern", NULL, NULL,
                                                       GIMP_TYPE_DASH_PATTERN,
                                                       GIMP_PARAM_WRITABLE));
}

static void
gimp_cell_renderer_dashes_init (GimpCellRendererDashes *dashes)
{
  dashes->segments = g_new0 (gboolean, N_SEGMENTS);
}

static void
gimp_cell_renderer_dashes_finalize (GObject *object)
{
  GimpCellRendererDashes *dashes = GIMP_CELL_RENDERER_DASHES (object);

  g_free (dashes->segments);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_cell_renderer_dashes_get_property (GObject    *object,
                                        guint       param_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
}

static void
gimp_cell_renderer_dashes_set_property (GObject      *object,
                                        guint         param_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpCellRendererDashes *dashes = GIMP_CELL_RENDERER_DASHES (object);

  switch (param_id)
    {
    case PROP_PATTERN:
      gimp_dash_pattern_fill_segments (g_value_get_boxed (value),
                                       dashes->segments, N_SEGMENTS);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_dashes_get_size (GtkCellRenderer    *cell,
                                    GtkWidget          *widget,
                                    const GdkRectangle *cell_area,
                                    gint               *x_offset,
                                    gint               *y_offset,
                                    gint               *width,
                                    gint               *height)
{
  gfloat xalign, yalign;
  gint   xpad, ypad;

  gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  if (cell_area)
    {
      if (x_offset)
        {
          gdouble align;

          align = ((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                   1.0 - xalign : xalign);

          *x_offset = align * (cell_area->width - DASHES_WIDTH);
          *x_offset = MAX (*x_offset, 0) + xpad;
        }

      if (y_offset)
        {
          *y_offset = yalign * (cell_area->height - DASHES_HEIGHT);
          *y_offset = MAX (*y_offset, 0) + ypad;
        }
    }
  else
    {
      if (x_offset)
        *x_offset = 0;

      if (y_offset)
        *y_offset = 0;
    }

  *width  = DASHES_WIDTH  + 2 * xpad;
  *height = DASHES_HEIGHT + 2 * ypad;
}

static void
gimp_cell_renderer_dashes_render (GtkCellRenderer      *cell,
                                  cairo_t              *cr,
                                  GtkWidget            *widget,
                                  const GdkRectangle   *background_area,
                                  const GdkRectangle   *cell_area,
                                  GtkCellRendererState  flags)
{
  GimpCellRendererDashes *dashes = GIMP_CELL_RENDERER_DASHES (cell);
  GtkStyleContext        *style  = gtk_widget_get_style_context (widget);
  GtkStateType            state;
  GdkRGBA                 color;
  gint                    xpad, ypad;
  gint                    width;
  gint                    x, y;

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  state = gtk_cell_renderer_get_state (cell, widget, flags);

  y = cell_area->y + (cell_area->height - DASHES_HEIGHT) / 2;
  width = cell_area->width - 2 * xpad;

  for (x = 0; x < width + BLOCK_WIDTH; x += BLOCK_WIDTH)
    {
      guint index = ((guint) x / BLOCK_WIDTH) % N_SEGMENTS;

      if (dashes->segments[index])
        {
          cairo_rectangle (cr,
                           cell_area->x + xpad + x, y,
                           MIN (BLOCK_WIDTH, width - x), DASHES_HEIGHT);
        }
    }

  gtk_style_context_get_color (style, state, &color);
  gdk_cairo_set_source_rgba (cr, &color);

  cairo_fill (cr);
}

GtkCellRenderer *
gimp_cell_renderer_dashes_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_DASHES, NULL);
}
