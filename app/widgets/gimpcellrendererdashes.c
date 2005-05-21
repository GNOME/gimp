/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererdashes.c
 * Copyright (C) 2005 Sven Neumann <sven@gimp.org>
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

#include <config.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpdashpattern.h"

#include "gimpcellrendererdashes.h"


#define DASHES_WIDTH   72
#define DASHES_HEIGHT   6

#define N_SEGMENTS     24
#define BLOCK_WIDTH    (DASHES_WIDTH / N_SEGMENTS)


enum
{
  PROP_0,
  PROP_PATTERN
};


static void gimp_cell_renderer_dashes_class_init (GimpCellRendererDashesClass *klass);
static void gimp_cell_renderer_dashes_init       (GimpCellRendererDashes      *cell);

static void gimp_cell_renderer_dashes_finalize     (GObject         *object);
static void gimp_cell_renderer_dashes_get_property (GObject         *object,
                                                    guint            param_id,
                                                    GValue          *value,
                                                    GParamSpec      *pspec);
static void gimp_cell_renderer_dashes_set_property (GObject         *object,
                                                    guint            param_id,
                                                    const GValue    *value,
                                                    GParamSpec      *pspec);
static void gimp_cell_renderer_dashes_get_size     (GtkCellRenderer *cell,
                                                    GtkWidget       *widget,
                                                    GdkRectangle    *rectangle,
                                                    gint            *x_offset,
                                                    gint            *y_offset,
                                                    gint            *width,
                                                    gint            *height);
static void gimp_cell_renderer_dashes_render       (GtkCellRenderer *cell,
                                                    GdkWindow       *window,
                                                    GtkWidget       *widget,
                                                    GdkRectangle    *background_area,
                                                    GdkRectangle    *cell_area,
                                                    GdkRectangle    *expose_area,
                                                    GtkCellRendererState flags);


static GtkCellRendererClass *parent_class = NULL;


GType
gimp_cell_renderer_dashes_get_type (void)
{
  static GType cell_type = 0;

  if (! cell_type)
    {
      static const GTypeInfo cell_info =
      {
        sizeof (GimpCellRendererDashesClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) gimp_cell_renderer_dashes_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (GimpCellRendererDashes),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_cell_renderer_dashes_init,
      };

      cell_type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
                                          "GimpCellRendererDashes",
                                          &cell_info, 0);
    }

  return cell_type;
}

static void
gimp_cell_renderer_dashes_class_init (GimpCellRendererDashesClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_cell_renderer_dashes_finalize;
  object_class->get_property = gimp_cell_renderer_dashes_get_property;
  object_class->set_property = gimp_cell_renderer_dashes_set_property;

  cell_class->get_size       = gimp_cell_renderer_dashes_get_size;
  cell_class->render         = gimp_cell_renderer_dashes_render;

  g_object_class_install_property (object_class, PROP_PATTERN,
                                   g_param_spec_pointer ("pattern", NULL, NULL,
                                                         G_PARAM_READWRITE));
}

static void
gimp_cell_renderer_dashes_init (GimpCellRendererDashes *dashes)
{
  dashes->pattern  = NULL;
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
  GimpCellRendererDashes *dashes = GIMP_CELL_RENDERER_DASHES (object);

  switch (param_id)
    {
    case PROP_PATTERN:
      g_value_set_pointer (value, dashes->pattern);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
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
      dashes->pattern = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_dashes_get_size (GtkCellRenderer *cell,
                                    GtkWidget       *widget,
                                    GdkRectangle    *cell_area,
                                    gint            *x_offset,
                                    gint            *y_offset,
                                    gint            *width,
                                    gint            *height)
{
  if (cell_area)
    {
      if (x_offset)
	{
          gdouble align;

          align = ((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                   1.0 - cell->xalign : cell->xalign);

	  *x_offset = align * (cell_area->width - DASHES_WIDTH);
	  *x_offset = MAX (*x_offset, 0) + cell->xpad;
	}

      if (y_offset)
	{
	  *y_offset = cell->yalign * (cell_area->height - DASHES_HEIGHT);
	  *y_offset = MAX (*y_offset, 0) + cell->ypad;
	}
    }
  else
    {
      if (x_offset)
        *x_offset = 0;

      if (y_offset)
        *y_offset = 0;
    }

  *width  = DASHES_WIDTH  + 2 * cell->xpad;
  *height = DASHES_HEIGHT + 2 * cell->ypad;
}

static void
gimp_cell_renderer_dashes_render (GtkCellRenderer      *cell,
                                  GdkWindow            *window,
                                  GtkWidget            *widget,
                                  GdkRectangle         *background_area,
                                  GdkRectangle         *cell_area,
                                  GdkRectangle         *expose_area,
                                  GtkCellRendererState  flags)
{
  GimpCellRendererDashes *dashes = GIMP_CELL_RENDERER_DASHES (cell);
  GdkRectangle            rect;
  gint                    x, y;

  gimp_dash_pattern_segments_set (dashes->pattern,
                                  dashes->segments, N_SEGMENTS);

  gdk_rectangle_intersect (cell_area, expose_area, &rect);
  gdk_draw_rectangle (window,
                      widget->style->base_gc[GTK_STATE_NORMAL], TRUE,
                      rect.x, rect.y, rect.width, rect.height);

  y = cell_area->y + (cell_area->height - DASHES_HEIGHT) / 2;

  for (x = 0; x < cell_area->width + BLOCK_WIDTH; x += BLOCK_WIDTH)
    {
      guint index = ((guint) x / BLOCK_WIDTH) % N_SEGMENTS;

      if (dashes->segments[index])
        {
          rect.x      = cell_area->x + x;
          rect.y      = y;
          rect.width  = BLOCK_WIDTH;
          rect.height = DASHES_HEIGHT;

          gdk_rectangle_intersect (&rect, expose_area, &rect);
          gdk_draw_rectangle (widget->window,
                              widget->style->text_gc[GTK_STATE_NORMAL], TRUE,
                              rect.x, rect.y, rect.width, rect.height);
        }
    }
}

GtkCellRenderer *
gimp_cell_renderer_dashes_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_DASHES, NULL);
}
