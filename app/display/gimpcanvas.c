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

#include "gimpcanvas.h"


/*  local function prototypes  */

static void  gimp_canvas_class_init (GimpCanvasClass *klass);
static void  gimp_canvas_init       (GimpCanvas      *gdisp);
static void  gimp_canvas_realize    (GtkWidget       *widget);
static void  gimp_canvas_unrealize  (GtkWidget       *widget);


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
  canvas->render_gc = NULL;
}

static void
gimp_canvas_realize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  canvas->render_gc = gdk_gc_new (widget->window);
  gdk_gc_set_exposures (canvas->render_gc, TRUE);
}

static void
gimp_canvas_unrealize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  if (canvas->render_gc)
    {
      g_object_unref (canvas->render_gc);
      canvas->render_gc = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

GtkWidget *
gimp_canvas_new (void)
{
  return GTK_WIDGET (g_object_new (GIMP_TYPE_CANVAS,
                                   "name", "gimp-canvas",
                                   NULL));
}
