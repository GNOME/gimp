/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "config/config-types.h"

#include "core/gimpgrid.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"


/*  local function prototypes  */

static void    gimp_canvas_class_init    (GimpCanvasClass     *klass);
static void    gimp_canvas_init          (GimpCanvas          *gdisp);
static void    gimp_canvas_realize       (GtkWidget           *widget);
static void    gimp_canvas_unrealize     (GtkWidget           *widget);
static GdkGC * gimp_canvas_guides_gc_new (GtkWidget           *widget,
                                          GimpOrientationType  orientation,
                                          GdkColor            *fg,
                                          GdkColor            *bg);


static GtkDrawingAreaClass *parent_class = NULL;


GType
gimp_canvas_get_type (void)
{
  static GType canvas_type = 0;

  if (! canvas_type)
    {
      static const GTypeInfo canvas_info =
      {
        sizeof (GimpCanvasClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_canvas_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCanvas),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_canvas_init,
      };

      canvas_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                            "GimpCanvas",
                                            &canvas_info, 0);
    }

  return canvas_type;
}

static void
gimp_canvas_class_init (GimpCanvasClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->realize   = gimp_canvas_realize;
  widget_class->unrealize = gimp_canvas_unrealize;
}

static void
gimp_canvas_init (GimpCanvas *canvas)
{
  canvas->render_gc         = NULL;

  canvas->guides.normal_hgc = NULL;
  canvas->guides.active_hgc = NULL;
  canvas->guides.normal_vgc = NULL;
  canvas->guides.active_vgc = NULL;

  canvas->grid_gc           = NULL;
  canvas->vectors_gc        = NULL;
}

static void
gimp_canvas_realize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);
  GdkColor    fg;
  GdkColor    bg;

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  canvas->render_gc = gdk_gc_new (widget->window);
  gdk_gc_set_exposures (canvas->render_gc, TRUE);

  /* guides */
  fg.red   = 0x0;
  fg.green = 0x0;
  fg.blue  = 0x0;

  bg.red   = 0x0;
  bg.green = 0x7f7f;
  bg.blue  = 0xffff;

  canvas->guides.normal_hgc =
    gimp_canvas_guides_gc_new (widget,
                               GIMP_ORIENTATION_HORIZONTAL, &fg, &bg);
  canvas->guides.normal_vgc =
    gimp_canvas_guides_gc_new (widget,
                               GIMP_ORIENTATION_VERTICAL,   &fg, &bg);

  fg.red   = 0x0;
  fg.green = 0x0;
  fg.blue  = 0x0;

  bg.red   = 0xffff;
  bg.green = 0x0;
  bg.blue  = 0x0;

  canvas->guides.active_hgc =
    gimp_canvas_guides_gc_new (widget,
                               GIMP_ORIENTATION_HORIZONTAL, &fg, &bg);
  canvas->guides.active_vgc =
    gimp_canvas_guides_gc_new (widget,
                                 GIMP_ORIENTATION_VERTICAL,   &fg, &bg);

  canvas->grid_gc = NULL;

  canvas->vectors_gc = gdk_gc_new (widget->window);
  gdk_gc_set_function (canvas->vectors_gc, GDK_INVERT);
  fg.pixel = 0xFFFFFFFF;
  bg.pixel = 0x00000000;
  gdk_gc_set_foreground (canvas->vectors_gc, &fg);
  gdk_gc_set_background (canvas->vectors_gc, &bg);
  gdk_gc_set_line_attributes (canvas->vectors_gc,
                              0,
                              GDK_LINE_SOLID,
                              GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
}

static void
gimp_canvas_unrealize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  g_object_unref (canvas->render_gc);

  g_object_unref (canvas->guides.normal_hgc);
  g_object_unref (canvas->guides.normal_vgc);
  g_object_unref (canvas->guides.active_hgc);
  g_object_unref (canvas->guides.active_vgc);

  /* g_object_unref (canvas->grid_gc); */

  g_object_unref (canvas->vectors_gc);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static GdkGC *
gimp_canvas_guides_gc_new (GtkWidget           *widget,
                           GimpOrientationType  orient,
                           GdkColor            *fg,
                           GdkColor            *bg)
{
  const guchar stipple[] =
    {
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
    };

  GdkGC       *gc;
  GdkGCValues  values;

  values.fill = GDK_OPAQUE_STIPPLED;
  values.stipple =
    gdk_bitmap_create_from_data (widget->window,
                                 (const gchar *) stipple,
                                 orient == GIMP_ORIENTATION_HORIZONTAL ? 8 : 1,
                                 orient == GIMP_ORIENTATION_VERTICAL   ? 8 : 1);

  gc = gdk_gc_new_with_values (widget->window,
                               &values, GDK_GC_FILL | GDK_GC_STIPPLE);

  gdk_gc_set_rgb_fg_color (gc, fg);
  gdk_gc_set_rgb_bg_color (gc, bg);

  return gc;
}

GtkWidget *
gimp_canvas_new (void)
{
  return GTK_WIDGET (g_object_new (GIMP_TYPE_CANVAS,
                                   "name", "gimp-canvas",
                                   NULL));
}

GdkGC *
gimp_canvas_grid_gc_new (GimpCanvas *canvas,
                         GimpGrid   *grid)
{
  GdkGC       *gc;
  GdkGCValues  values;
  GdkColor     fg, bg;

  g_return_val_if_fail (GIMP_IS_CANVAS (canvas), NULL);
  g_return_val_if_fail (GIMP_IS_GRID (grid), NULL);

  switch (grid->style)
    {
    case GIMP_GRID_ON_OFF_DASH:
      values.line_style = GDK_LINE_ON_OFF_DASH;
      break;

    case GIMP_GRID_DOUBLE_DASH:
      values.line_style = GDK_LINE_DOUBLE_DASH;
      break;

    case GIMP_GRID_DOTS:
    case GIMP_GRID_INTERSECTIONS:
    case GIMP_GRID_SOLID:
      values.line_style = GDK_LINE_SOLID;
      break;
    }

  values.join_style = GDK_JOIN_MITER;

  gc = gdk_gc_new_with_values (GTK_WIDGET (canvas)->window, &values,
                               GDK_GC_LINE_STYLE | GDK_GC_JOIN_STYLE);

  gimp_rgb_get_gdk_color (&grid->fgcolor, &fg);
  gimp_rgb_get_gdk_color (&grid->bgcolor, &bg);

  gdk_gc_set_rgb_fg_color (gc, &fg);
  gdk_gc_set_rgb_bg_color (gc, &bg);

  return gc;
}

void
gimp_canvas_draw_cursor (GimpCanvas *canvas,
                         gint        x,
                         gint        y)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  gdk_draw_line (widget->window,
                 widget->style->white_gc, x - 7, y - 1, x + 7, y - 1);
  gdk_draw_line (widget->window,
                 widget->style->black_gc, x - 7, y,     x + 7, y);
  gdk_draw_line (widget->window,
                 widget->style->white_gc, x - 7, y + 1, x + 7, y + 1);
  gdk_draw_line (widget->window,
                 widget->style->white_gc, x - 1, y - 7, x - 1, y + 7);
  gdk_draw_line (widget->window,
                 widget->style->black_gc, x,     y - 7, x,     y + 7);
  gdk_draw_line (widget->window,
                 widget->style->white_gc, x + 1, y - 7, x + 1, y + 7);
}
