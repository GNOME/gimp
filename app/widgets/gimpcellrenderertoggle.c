/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrenderertoggle.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include "gimpcellrenderertoggle.h"


#define DEFAULT_ICON_SIZE  GTK_ICON_SIZE_BUTTON

enum
{
  PROP_0,
  PROP_STOCK_ID,
  PROP_STOCK_SIZE
};


static void gimp_cell_renderer_toggle_class_init (GimpCellRendererToggleClass *klass);
static void gimp_cell_renderer_toggle_init       (GimpCellRendererToggle      *toggle);

static void gimp_cell_renderer_toggle_finalize     (GObject         *object);
static void gimp_cell_renderer_toggle_get_property (GObject         *object,
                                                    guint            param_id,
                                                    GValue          *value,
                                                    GParamSpec      *pspec);
static void gimp_cell_renderer_toggle_set_property (GObject         *object,
                                                    guint            param_id,
                                                    const GValue    *value,
                                                    GParamSpec      *pspec);
static void gimp_cell_renderer_toggle_get_size     (GtkCellRenderer *cell,
                                                    GtkWidget       *widget,
                                                    GdkRectangle    *rectangle,
                                                    gint            *x_offset,
                                                    gint            *y_offset,
                                                    gint            *width,
                                                    gint            *height);
static void gimp_cell_renderer_toggle_render       (GtkCellRenderer *cell,
                                                    GdkWindow       *window,
                                                    GtkWidget       *widget,
                                                    GdkRectangle    *background_area,
                                                    GdkRectangle    *cell_area,
                                                    GdkRectangle    *expose_area,
                                                    GtkCellRendererState flags);
static void gimp_cell_renderer_toggle_create_pixbuf (GimpCellRendererToggle *toggle,
                                                     GtkWidget              *widget);


static GtkCellRendererToggleClass *parent_class = NULL;


GType
gimp_cell_renderer_toggle_get_type (void)
{
  static GType cell_type = 0;

  if (! cell_type)
    {
      static const GTypeInfo cell_info =
      {
        sizeof (GimpCellRendererToggleClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) gimp_cell_renderer_toggle_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (GimpCellRendererToggle),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_cell_renderer_toggle_init,
      };

      cell_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_TOGGLE,
                                          "GimpCellRendererToggle",
                                          &cell_info, 0);
    }

  return cell_type;
}

static void
gimp_cell_renderer_toggle_class_init (GimpCellRendererToggleClass *klass)
{
  GObjectClass         *object_class;
  GtkCellRendererClass *cell_class;

  object_class = G_OBJECT_CLASS (klass);
  cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_cell_renderer_toggle_finalize;
  object_class->get_property = gimp_cell_renderer_toggle_get_property;
  object_class->set_property = gimp_cell_renderer_toggle_set_property;

  cell_class->get_size       = gimp_cell_renderer_toggle_get_size;
  cell_class->render         = gimp_cell_renderer_toggle_render;

  g_object_class_install_property (object_class,
                                   PROP_STOCK_ID,
                                   g_param_spec_string ("stock_id",
                                                        NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_STOCK_SIZE,
                                   g_param_spec_enum ("stock_size",
                                                      NULL, NULL,
                                                      GTK_TYPE_ICON_SIZE,
                                                      DEFAULT_ICON_SIZE,
                                                      G_PARAM_READWRITE));
}

static void
gimp_cell_renderer_toggle_init (GimpCellRendererToggle *cell)
{
  GTK_CELL_RENDERER (cell)->xpad = 0;
  GTK_CELL_RENDERER (cell)->ypad = 0;

  cell->stock_id   = NULL;
  cell->stock_size = DEFAULT_ICON_SIZE;
}

static void
gimp_cell_renderer_toggle_finalize (GObject *object)
{
  GimpCellRendererToggle *toggle;

  toggle = GIMP_CELL_RENDERER_TOGGLE (object);

  if (toggle->stock_id)
    {
      g_free (toggle->stock_id);
      toggle->stock_id = NULL;
    }
  if (toggle->pixbuf)
    {
      g_object_unref (toggle->pixbuf);
      toggle->pixbuf = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_cell_renderer_toggle_get_property (GObject    *object,
                                        guint       param_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpCellRendererToggle *toggle;

  toggle = GIMP_CELL_RENDERER_TOGGLE (object);

  switch (param_id)
    {
    case PROP_STOCK_ID:
      g_value_set_string (value, toggle->stock_id);
      break;
    case PROP_STOCK_SIZE:
      g_value_set_enum (value, toggle->stock_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_toggle_set_property (GObject      *object,
                                        guint         param_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpCellRendererToggle *toggle;
  
  toggle = GIMP_CELL_RENDERER_TOGGLE (object);

  switch (param_id)
    {
    case PROP_STOCK_ID:
      if (toggle->stock_id)
	g_free (toggle->stock_id);
      toggle->stock_id = g_value_dup_string (value);
      break;
    case PROP_STOCK_SIZE:
      toggle->stock_size = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

  if (toggle->pixbuf)
    {
      g_object_unref (toggle->pixbuf);
      toggle->pixbuf = NULL;
    }
}

static void
gimp_cell_renderer_toggle_get_size (GtkCellRenderer *cell,
                                    GtkWidget       *widget,
                                    GdkRectangle    *cell_area,
                                    gint            *x_offset,
                                    gint            *y_offset,
                                    gint            *width,
                                    gint            *height)
{
  GimpCellRendererToggle *toggle;
  gint                    calc_width;
  gint                    calc_height;
  gint                    pixbuf_width;
  gint                    pixbuf_height;

  toggle = GIMP_CELL_RENDERER_TOGGLE (cell);

  if (!toggle->stock_id)
    {
      GTK_CELL_RENDERER_CLASS (parent_class)->get_size (cell,
                                                        widget,
                                                        cell_area,
                                                        x_offset, y_offset,
                                                        width, height);
      return;
    }

  if (!toggle->pixbuf)
    gimp_cell_renderer_toggle_create_pixbuf (toggle, widget);

  pixbuf_width  = gdk_pixbuf_get_width  (toggle->pixbuf);
  pixbuf_height = gdk_pixbuf_get_height (toggle->pixbuf);

  calc_width  = (pixbuf_width +
                 (gint) cell->xpad * 2 + widget->style->xthickness * 2);
  calc_height = (pixbuf_height +
                 (gint) cell->ypad * 2 + widget->style->ythickness * 2);

  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;

  if (cell_area)
    {
      if (x_offset)
	{
	  *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        (1.0 - cell->xalign) : cell->xalign) *
                       (cell_area->width - calc_width));
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = cell->yalign * (cell_area->height - calc_height);
	  *y_offset = MAX (*y_offset, 0);
	}
    }

  if (width)  *width  = calc_width;
  if (height) *height = calc_height;
}

static void
gimp_cell_renderer_toggle_render (GtkCellRenderer      *cell,
                                  GdkWindow            *window,
                                  GtkWidget            *widget,
                                  GdkRectangle         *background_area,
                                  GdkRectangle         *cell_area,
                                  GdkRectangle         *expose_area,
                                  GtkCellRendererState  flags)
{
  GimpCellRendererToggle *toggle;
  GdkRectangle            toggle_rect;
  GdkRectangle            draw_rect;
  GtkStateType            state;
  gboolean                active;

  toggle = GIMP_CELL_RENDERER_TOGGLE (cell);

  if (!toggle->stock_id)
    {
      GTK_CELL_RENDERER_CLASS (parent_class)->render (cell, window, widget,
                                                      background_area,
                                                      cell_area, expose_area,
                                                      flags);
      return;
    }

  gimp_cell_renderer_toggle_get_size (cell, widget, cell_area,
                                      &toggle_rect.x,
                                      &toggle_rect.y,
                                      &toggle_rect.width,
                                      &toggle_rect.height);

  toggle_rect.x      += cell_area->x + cell->xpad;
  toggle_rect.y      += cell_area->y + cell->ypad;
  toggle_rect.width  -= cell->xpad * 2;
  toggle_rect.height -= cell->ypad * 2;

  if (toggle_rect.width <= 0 || toggle_rect.height <= 0)
    return;

  active =
    gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (cell));

  if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED)
    {
      if (GTK_WIDGET_HAS_FOCUS (widget))
	state = GTK_STATE_SELECTED;
      else
	state = GTK_STATE_ACTIVE;
    }
  else
    {
      if (GTK_CELL_RENDERER_TOGGLE (cell)->activatable)
        state = GTK_STATE_NORMAL;
      else
        state = GTK_STATE_INSENSITIVE;
    }

  if (gdk_rectangle_intersect (expose_area, cell_area, &draw_rect) &&
      (flags & GTK_CELL_RENDERER_PRELIT))
    gtk_paint_shadow (widget->style,
                      window,
                      state,
                      active ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
                      &draw_rect,
                      widget, NULL,
                      toggle_rect.x,     toggle_rect.y,
                      toggle_rect.width, toggle_rect.height);

  if (active)
    {
      toggle_rect.x      += widget->style->xthickness;
      toggle_rect.y      += widget->style->ythickness;
      toggle_rect.width  -= widget->style->xthickness * 2;
      toggle_rect.height -= widget->style->ythickness * 2;  
      
      if (gdk_rectangle_intersect (&draw_rect, &toggle_rect, &draw_rect))
        gdk_draw_pixbuf (window,
                         widget->style->black_gc,
                         toggle->pixbuf,
                         /* pixbuf 0, 0 is at toggle_rect.x, toggle_rect.y */
                         draw_rect.x - toggle_rect.x,
                         draw_rect.y - toggle_rect.y,
                         draw_rect.x,
                         draw_rect.y,
                         draw_rect.width,
                         draw_rect.height,
                         GDK_RGB_DITHER_NORMAL,
                         0, 0);
    }
}

static void
gimp_cell_renderer_toggle_create_pixbuf (GimpCellRendererToggle *toggle,
                                         GtkWidget              *widget)
{
  if (toggle->pixbuf)
    g_object_unref (toggle->pixbuf);

  toggle->pixbuf = gtk_widget_render_icon (widget,
                                           toggle->stock_id,
                                           toggle->stock_size, NULL);
}

GtkCellRenderer *
gimp_cell_renderer_toggle_new (const gchar *stock_id)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_TOGGLE,
                       "stock_id", stock_id,
                       NULL);
}
