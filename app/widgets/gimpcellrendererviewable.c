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


enum
{
  PROP_0,
  PROP_VIEWABLE,
  PROP_PREVIEW_SIZE
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


GtkCellRendererPixbufClass *parent_class = NULL;


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

      cell_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_PIXBUF,
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
                                   PROP_VIEWABLE,
                                   g_param_spec_object ("viewable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_VIEWABLE,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PREVIEW_SIZE,
                                   g_param_spec_int ("preview-size",
                                                     NULL, NULL,
                                                     0, 1024, 16,
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
    case PROP_VIEWABLE:
      g_value_set_object (value, cell->viewable);
      break;

    case PROP_PREVIEW_SIZE:
      g_value_set_int (value, cell->preview_size);
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
  GimpViewable             *viewable;
  
  cell = GIMP_CELL_RENDERER_VIEWABLE (object);

  switch (param_id)
    {
    case PROP_VIEWABLE:
      viewable = (GimpViewable *) g_value_get_object (value);
      if (cell->viewable)
	g_object_remove_weak_pointer (G_OBJECT (cell->viewable),
                                      (gpointer) &cell->viewable);
      cell->viewable = viewable;
      if (cell->viewable)
        g_object_add_weak_pointer (G_OBJECT (cell->viewable),
                                   (gpointer) &cell->viewable);
      break;

    case PROP_PREVIEW_SIZE:
      cell->preview_size = g_value_get_int (value);
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

  if (cellviewable->viewable)
    {
      gimp_viewable_get_preview_size (cellviewable->viewable,
                                      cellviewable->preview_size,
                                      FALSE, TRUE,
                                      &preview_width,
                                      &preview_height);
    }
  else
    {
      preview_width  = cellviewable->preview_size;
      preview_height = cellviewable->preview_size;
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
  GtkCellRendererPixbuf    *cellpixbuf;

  cellviewable = GIMP_CELL_RENDERER_VIEWABLE (cell);
  cellpixbuf   = GTK_CELL_RENDERER_PIXBUF (cell);

  if (! cellpixbuf->pixbuf && cellviewable->viewable)
    {
      GimpViewable *viewable;
      GtkIconSet   *icon_set;
      GtkIconSize  *sizes;
      gint          n_sizes;
      gint          i;
      gint          width_diff  = 1024;
      gint          height_diff = 1024;
      GtkIconSize   icon_size = GTK_ICON_SIZE_MENU;
      GdkPixbuf    *pixbuf;
      const gchar  *stock_id;

      viewable = cellviewable->viewable;

      stock_id = gimp_viewable_get_stock_id (viewable);
      icon_set = gtk_style_lookup_icon_set (widget->style, stock_id);

      gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

      for (i = 0; i < n_sizes; i++)
        {
          gint width;
          gint height;

          if (gtk_icon_size_lookup (sizes[i], &width, &height))
            {
              if (width  <= cell_area->width  &&
                  height <= cell_area->height &&
                  (ABS (width  - cell_area->width)  < width_diff ||
                   ABS (height - cell_area->height) < height_diff))
                {
                  width_diff  = ABS (width  - cell_area->width);
                  height_diff = ABS (height - cell_area->height);

                  icon_size = sizes[i];
                }
            }
        }

      g_free (sizes);

      pixbuf = gtk_icon_set_render_icon (icon_set,
                                         widget->style,
                                         gtk_widget_get_direction (widget),
                                         widget->state,
                                         GTK_ICON_SIZE_DIALOG,
                                         widget, NULL);

      if (pixbuf)
        {
          if (gdk_pixbuf_get_width (pixbuf)  > cell_area->width ||
              gdk_pixbuf_get_height (pixbuf) > cell_area->height)
            {
              GdkPixbuf *scaled_pixbuf;
              gint       pixbuf_width;
              gint       pixbuf_height;

              gimp_viewable_calc_preview_size (viewable,
                                               gdk_pixbuf_get_width (pixbuf),
                                               gdk_pixbuf_get_height (pixbuf),
                                               cell_area->width,
                                               cell_area->height,
                                               TRUE, 1.0, 1.0,
                                               &pixbuf_width,
                                               &pixbuf_height,
                                               NULL);

              scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                       pixbuf_width,
                                                       pixbuf_height,
                                                       GDK_INTERP_BILINEAR);

              g_object_unref (pixbuf);
              pixbuf = scaled_pixbuf;
            }

          g_object_set (G_OBJECT (cell), "pixbuf", pixbuf, NULL);
          g_object_unref (pixbuf);
        }
    }

  GTK_CELL_RENDERER_CLASS (parent_class)->render (cell, window, widget,
                                                  background_area,
                                                  cell_area,
                                                  expose_area,
                                                  flags);
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

  if (cellviewable->viewable &&
      ((GdkEventAny *) event)->type == GDK_BUTTON_PRESS &&
      ((GdkEventButton *) event)->button == 1)
    {
      gint preview_width;
      gint preview_height;

      gimp_viewable_get_preview_size (cellviewable->viewable,
                                      cellviewable->preview_size,
                                      FALSE, TRUE,
                                      &preview_width,
                                      &preview_height);

      return gimp_preview_popup_show (widget,
                                      (GdkEventButton *) event,
                                      cellviewable->viewable,
                                      preview_width,
                                      preview_height,
                                      TRUE);
    }

  return FALSE;
}

GtkCellRenderer *
gimp_cell_renderer_viewable_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_VIEWABLE, NULL);
}
