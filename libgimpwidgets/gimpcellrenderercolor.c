/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcellrenderercolor.c
 * Copyright (C) 2004  Sven Neuman <sven1@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"
#include "gimpcellrenderercolor.h"


#define DEFAULT_ICON_SIZE  GTK_ICON_SIZE_MENU


enum
{
  PROP_0,
  PROP_COLOR,
  PROP_OPAQUE,
  PROP_SIZE
};


static void gimp_cell_renderer_color_get_property (GObject         *object,
                                                   guint            param_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
static void gimp_cell_renderer_color_set_property (GObject         *object,
                                                   guint            param_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void gimp_cell_renderer_color_get_size     (GtkCellRenderer *cell,
                                                   GtkWidget       *widget,
                                                   GdkRectangle    *rectangle,
                                                   gint            *x_offset,
                                                   gint            *y_offset,
                                                   gint            *width,
                                                   gint            *height);
static void gimp_cell_renderer_color_render       (GtkCellRenderer *cell,
                                                   GdkWindow       *window,
                                                   GtkWidget       *widget,
                                                   GdkRectangle    *background_area,
                                                   GdkRectangle    *cell_area,
                                                   GdkRectangle    *expose_area,
                                                   GtkCellRendererState flags);


G_DEFINE_TYPE (GimpCellRendererColor, gimp_cell_renderer_color,
               GTK_TYPE_CELL_RENDERER)

#define parent_class gimp_cell_renderer_color_parent_class


static void
gimp_cell_renderer_color_class_init (GimpCellRendererColorClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->get_property = gimp_cell_renderer_color_get_property;
  object_class->set_property = gimp_cell_renderer_color_set_property;

  cell_class->get_size       = gimp_cell_renderer_color_get_size;
  cell_class->render         = gimp_cell_renderer_color_render;

  g_object_class_install_property (object_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color", NULL, NULL,
                                                       GIMP_TYPE_RGB,
                                                       GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_OPAQUE,
                                   g_param_spec_boolean ("opaque", NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   g_param_spec_int ("icon-size", NULL, NULL,
                                                     0, G_MAXINT,
                                                     DEFAULT_ICON_SIZE,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_cell_renderer_color_init (GimpCellRendererColor *cell)
{
  gimp_rgba_set (&cell->color, 0.0, 0.0, 0.0, 1.0);
}

static void
gimp_cell_renderer_color_get_property (GObject    *object,
                                       guint       param_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpCellRendererColor *cell = GIMP_CELL_RENDERER_COLOR (object);

  switch (param_id)
    {
    case PROP_COLOR:
      g_value_set_boxed (value, &cell->color);
      break;
    case PROP_OPAQUE:
      g_value_set_boolean (value, cell->opaque);
      break;
    case PROP_SIZE:
      g_value_set_int (value, cell->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_color_set_property (GObject      *object,
                                       guint         param_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpCellRendererColor *cell = GIMP_CELL_RENDERER_COLOR (object);
  GimpRGB               *color;

  switch (param_id)
    {
    case PROP_COLOR:
      color = g_value_get_boxed (value);
      cell->color = *color;
      break;
    case PROP_OPAQUE:
      cell->opaque = g_value_get_boolean (value);
      break;
    case PROP_SIZE:
      cell->size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_color_get_size (GtkCellRenderer *cell,
                                   GtkWidget       *widget,
                                   GdkRectangle    *cell_area,
                                   gint            *x_offset,
                                   gint            *y_offset,
                                   gint            *width,
                                   gint            *height)
{
  GimpCellRendererColor *color = GIMP_CELL_RENDERER_COLOR (cell);
  gint                   calc_width;
  gint                   calc_height;

  gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
                                     color->size, &calc_width, &calc_height);

  if (cell_area && calc_width > 0 && calc_height > 0)
    {
      if (x_offset)
        {
          *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        1.0 - cell->xalign : cell->xalign) *
                       (cell_area->width - calc_width));
          *x_offset = MAX (*x_offset, 0) + cell->xpad;
        }
      if (y_offset)
        {
          *y_offset = (cell->yalign *
                       (cell_area->height - calc_height));
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

  if (width)
    *width  = calc_width  + 2 * cell->xpad;
  if (height)
    *height = calc_height + 2 * cell->ypad;
}

static void
gimp_cell_renderer_color_render (GtkCellRenderer      *cell,
                                 GdkWindow            *window,
                                 GtkWidget            *widget,
                                 GdkRectangle         *background_area,
                                 GdkRectangle         *cell_area,
                                 GdkRectangle         *expose_area,
                                 GtkCellRendererState  flags)
{
  GimpCellRendererColor *color = GIMP_CELL_RENDERER_COLOR (cell);
  GdkRectangle           rect;

  gimp_cell_renderer_color_get_size (cell, widget, cell_area,
                                     &rect.x,
                                     &rect.y,
                                     &rect.width,
                                     &rect.height);

  rect.x      += cell_area->x + cell->xpad;
  rect.y      += cell_area->y + cell->ypad;
  rect.width  -= 2 * cell->xpad;
  rect.height -= 2 * cell->ypad;

  if (rect.width > 2 && rect.height > 2)
    {
      GtkStateType  state;
      guchar       *buf;
      guint         rowstride = 3 * (rect.width - 2);

      if (rowstride & 3)
        rowstride += 4 - (rowstride & 3);

      buf = g_alloca (rowstride * (rect.height - 2));

      _gimp_color_area_render_buf (widget,
                                   (GTK_WIDGET_STATE (widget) ==
                                    GTK_STATE_INSENSITIVE || !cell->sensitive),
                                   (color->opaque ?
                                    GIMP_COLOR_AREA_FLAT :
                                    GIMP_COLOR_AREA_SMALL_CHECKS),
                                   buf,
                                   rect.width - 2, rect.height - 2, rowstride,
                                   &color->color);

      gdk_draw_rgb_image_dithalign (window,
                                    widget->style->black_gc,
                                    rect.x + 1, rect.y + 1,
                                    rect.width - 2, rect.height - 2,
                                    GDK_RGB_DITHER_MAX,
                                    buf, rowstride, rect.x, rect.y);

      state = (flags & GTK_CELL_RENDERER_SELECTED ?
               GTK_STATE_SELECTED : GTK_STATE_NORMAL);

      gdk_draw_rectangle (window,
                          widget->style->fg_gc[state],
                          FALSE,
                          rect.x, rect.y, rect.width - 1, rect.height - 1);
    }
}

/**
 * gimp_cell_renderer_color_new:
 *
 * Creates a #GtkCellRenderer that displays a color.
 *
 * Return value: a new #GimpCellRendererColor
 *
 * Since: GIMP 2.2
 **/
GtkCellRenderer *
gimp_cell_renderer_color_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_COLOR, NULL);
}
