/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererviewable.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimppreview-popup.h"
#include "gimppreviewrenderer.h"


enum
{
  PROP_0,
  PROP_RENDERER
};


static void gimp_cell_renderer_viewable_class_init (GimpCellRendererViewableClass *klass);
static void gimp_cell_renderer_viewable_init       (GimpCellRendererViewable      *cell);

static void gimp_cell_renderer_viewable_get_property (GObject         *object,
                                                      guint            param_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);
static void gimp_cell_renderer_viewable_set_property (GObject         *object,
                                                      guint            param_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void gimp_cell_renderer_viewable_get_size     (GtkCellRenderer *cell,
                                                      GtkWidget       *widget,
                                                      GdkRectangle    *rectangle,
                                                      gint            *x_offset,
                                                      gint            *y_offset,
                                                      gint            *width,
                                                      gint            *height);
static void gimp_cell_renderer_viewable_render       (GtkCellRenderer *cell,
                                                      GdkWindow       *window,
                                                      GtkWidget       *widget,
                                                      GdkRectangle    *background_area,
                                                      GdkRectangle    *cell_area,
                                                      GdkRectangle    *expose_area,
                                                      GtkCellRendererState flags);
static gboolean gimp_cell_renderer_viewable_activate (GtkCellRenderer *cell,
                                                      GdkEvent        *event,
                                                      GtkWidget       *widget,
                                                      const gchar     *path,
                                                      GdkRectangle    *background_area,
                                                      GdkRectangle    *cell_area,
                                                      GtkCellRendererState flags);


GtkCellRendererClass *parent_class = NULL;


GType
gimp_cell_renderer_viewable_get_type (void)
{
  static GType cell_type = 0;

  if (! cell_type)
    {
      static const GTypeInfo cell_info =
      {
        sizeof (GimpCellRendererViewableClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) gimp_cell_renderer_viewable_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (GimpCellRendererViewable),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_cell_renderer_viewable_init,
      };

      cell_type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
                                          "GtkCellRendererViewable",
                                          &cell_info, 0);
    }

  return cell_type;
}

static void
gimp_cell_renderer_viewable_class_init (GimpCellRendererViewableClass *klass)
{
  GObjectClass         *object_class;
  GtkCellRendererClass *cell_class;

  object_class = G_OBJECT_CLASS (klass);
  cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = gimp_cell_renderer_viewable_get_property;
  object_class->set_property = gimp_cell_renderer_viewable_set_property;

  cell_class->get_size       = gimp_cell_renderer_viewable_get_size;
  cell_class->render         = gimp_cell_renderer_viewable_render;
  cell_class->activate       = gimp_cell_renderer_viewable_activate;

  g_object_class_install_property (object_class,
                                   PROP_RENDERER,
                                   g_param_spec_object ("renderer",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PREVIEW_RENDERER,
                                                        G_PARAM_READWRITE));
}

static void
gimp_cell_renderer_viewable_init (GimpCellRendererViewable *cellviewable)
{
  GTK_CELL_RENDERER (cellviewable)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
}

static void
gimp_cell_renderer_viewable_get_property (GObject    *object,
                                          guint       param_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpCellRendererViewable *cell;

  cell = GIMP_CELL_RENDERER_VIEWABLE (object);

  switch (param_id)
    {
    case PROP_RENDERER:
      g_value_set_object (value, cell->renderer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_viewable_set_property (GObject      *object,
                                          guint         param_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpCellRendererViewable *cell;
  
  cell = GIMP_CELL_RENDERER_VIEWABLE (object);

  switch (param_id)
    {
    case PROP_RENDERER:
      if (cell->renderer)
	g_object_unref (cell->renderer);
      cell->renderer = (GimpPreviewRenderer *) g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_viewable_get_size (GtkCellRenderer *cell,
                                      GtkWidget       *widget,
                                      GdkRectangle    *cell_area,
                                      gint            *x_offset,
                                      gint            *y_offset,
                                      gint            *width,
                                      gint            *height)
{
  GimpCellRendererViewable *cellviewable;
  gint                      preview_width  = 0;
  gint                      preview_height = 0;
  gint                      calc_width;
  gint                      calc_height;

  cellviewable = GIMP_CELL_RENDERER_VIEWABLE (cell);

  if (cellviewable->renderer)
    {
      preview_width  = (cellviewable->renderer->width  +
                        2 * cellviewable->renderer->border_width);
      preview_height = (cellviewable->renderer->height +
                        2 * cellviewable->renderer->border_width);
    }

  calc_width  = (gint) GTK_CELL_RENDERER (cell)->xpad * 2 + preview_width;
  calc_height = (gint) GTK_CELL_RENDERER (cell)->ypad * 2 + preview_height;

  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;

  if (cell_area && preview_width > 0 && preview_height > 0)
    {
      if (x_offset)
	{
	  *x_offset = (GTK_CELL_RENDERER (cellviewable)->xalign *
                       (cell_area->width - calc_width -
                        (2 * GTK_CELL_RENDERER (cellviewable)->xpad)));
	  *x_offset = (MAX (*x_offset, 0) +
                       GTK_CELL_RENDERER (cellviewable)->xpad);
	}
      if (y_offset)
	{
	  *y_offset = (GTK_CELL_RENDERER (cellviewable)->yalign *
                       (cell_area->height - calc_height -
                        (2 * GTK_CELL_RENDERER (cellviewable)->ypad)));
	  *y_offset = (MAX (*y_offset, 0) +
                       GTK_CELL_RENDERER (cellviewable)->ypad);
	}
    }

  if (width)  *width  = calc_width;
  if (height) *height = calc_height;
}

static void
gimp_cell_renderer_viewable_render (GtkCellRenderer      *cell,
                                    GdkWindow            *window,
                                    GtkWidget            *widget,
                                    GdkRectangle         *background_area,
                                    GdkRectangle         *cell_area,
                                    GdkRectangle         *expose_area,
                                    GtkCellRendererState  flags)
{
  GimpCellRendererViewable *cellviewable;

  cellviewable = GIMP_CELL_RENDERER_VIEWABLE (cell);

  if (cellviewable->renderer)
    gimp_preview_renderer_draw (cellviewable->renderer, window, widget,
                                cell_area, expose_area);
}

static gboolean
gimp_cell_renderer_viewable_activate (GtkCellRenderer      *cell,
                                      GdkEvent             *event,
                                      GtkWidget            *widget,
                                      const gchar          *path,
                                      GdkRectangle         *background_area,
                                      GdkRectangle         *cell_area,
                                      GtkCellRendererState  flags)
{
  GimpCellRendererViewable *cellviewable;

  cellviewable = GIMP_CELL_RENDERER_VIEWABLE (cell);

  if (cellviewable->renderer && event)
    {
      if (((GdkEventAny *) event)->type == GDK_BUTTON_PRESS &&
          ((GdkEventButton *) event)->button == 1)
        {
          return gimp_preview_popup_show (widget,
                                          (GdkEventButton *) event,
                                          cellviewable->renderer->viewable,
                                          cellviewable->renderer->width,
                                          cellviewable->renderer->height,
                                          TRUE);
        }
    }

  return FALSE;
}

GtkCellRenderer *
gimp_cell_renderer_viewable_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_VIEWABLE, NULL);
}
