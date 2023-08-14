/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselectbutton.c
 * Copyright (C) 1998 Andy Thomas
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpbrushselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpbrushselectbutton
 * @title: GimpBrushSelectButton
 * @short_description: A button which pops up a brush selection dialog.
 *
 * A button which pops up a brush selection dialog.
 **/

#define CELL_SIZE 20

/* A B&W image. mask_data is allocated and must be freed. */
typedef struct _PreviewBitmap
{
  gint           width;
  gint           height;
  guchar        *mask_data;
} _PreviewBitmap;

struct _GimpBrushSelectButton
{
  GimpResourceSelectButton  parent_instance;

  GtkWidget                *preview;
  GtkWidget                *popup;
};


static void gimp_brush_select_button_draw_interior   (GimpResourceSelectButton *self);

static void     gimp_brush_select_on_preview_resize  (GimpBrushSelectButton *button);
static gboolean gimp_brush_select_on_preview_events  (GtkWidget             *widget,
                                                      GdkEvent              *event,
                                                      GimpBrushSelectButton *button);

static void     gimp_brush_select_preview_draw      (GimpPreviewArea       *area,
                                                     gint                   x,
                                                     gint                   y,
                                                     _PreviewBitmap         mask,
                                                     gint                   rowstride);
static void     gimp_brush_select_preview_fill_draw (GtkWidget             *preview,
                                                     _PreviewBitmap         mask);

static void           gimp_brush_select_button_draw             (GimpBrushSelectButton *self);
static _PreviewBitmap gimp_brush_select_button_get_brush_bitmap (GimpBrushSelectButton *self);

/* Popup methods. */
static void     gimp_brush_select_button_open_popup  (GimpBrushSelectButton *button,
                                                      gint                   x,
                                                      gint                   y);
static void     gimp_brush_select_button_close_popup (GimpBrushSelectButton *button);


static const GtkTargetEntry drag_target = { "application/x-gimp-brush-name", 0, 0 };

G_DEFINE_FINAL_TYPE (GimpBrushSelectButton,
                     gimp_brush_select_button,
                     GIMP_TYPE_RESOURCE_SELECT_BUTTON)


static void
gimp_brush_select_button_class_init (GimpBrushSelectButtonClass *klass)
{
  GimpResourceSelectButtonClass *superclass = GIMP_RESOURCE_SELECT_BUTTON_CLASS (klass);

  superclass->draw_interior = gimp_brush_select_button_draw_interior;
  superclass->resource_type = GIMP_TYPE_BRUSH;
}

static void
gimp_brush_select_button_init (GimpBrushSelectButton *self)
{
  GtkWidget *frame;
  GtkWidget *button;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (self), frame, FALSE, FALSE, 0);

  self->preview = gimp_preview_area_new ();
  gtk_widget_add_events (self->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (self->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), self->preview);

  g_signal_connect_swapped (self->preview, "size-allocate",
                            G_CALLBACK (gimp_brush_select_on_preview_resize),
                            self);

  g_signal_connect (self->preview, "event",
                    G_CALLBACK (gimp_brush_select_on_preview_events),
                    self);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (self), button, FALSE, FALSE, 0);

  gtk_widget_show_all (GTK_WIDGET (self));

  gimp_resource_select_button_set_drag_target (GIMP_RESOURCE_SELECT_BUTTON (self),
                                               self->preview,
                                               &drag_target);

  gimp_resource_select_button_set_clickable (GIMP_RESOURCE_SELECT_BUTTON (self),
                                             button);
}

static void
gimp_brush_select_button_draw_interior (GimpResourceSelectButton *self)
{
  gimp_brush_select_button_draw (GIMP_BRUSH_SELECT_BUTTON (self));
}


/**
 * gimp_brush_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @resource: (nullable): Initial brush.
 *
 * Creates a new #GtkWidget that lets a user choose a brush.
 * You can put this widget in a plug-in dialog.
 *
 * When brush is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_brush_select_button_new (const gchar  *title,
                              GimpResource *resource)
{
  GtkWidget *self;

  if (resource == NULL)
    resource = GIMP_RESOURCE (gimp_context_get_brush ());

   if (title)
     self = g_object_new (GIMP_TYPE_BRUSH_SELECT_BUTTON,
                          "title",     title,
                          "resource",  resource,
                          NULL);
   else
     self = g_object_new (GIMP_TYPE_BRUSH_SELECT_BUTTON,
                          "resource",  resource,
                          NULL);

  gimp_brush_select_button_draw_interior (GIMP_RESOURCE_SELECT_BUTTON (self));

  return self;
}


/*  private functions  */

/* Draw self.
 *
 * This knows that we only draw the preview. Gtk draws the browse button.
 */
static void
gimp_brush_select_button_draw (GimpBrushSelectButton *self)
{
  _PreviewBitmap mask;

  mask = gimp_brush_select_button_get_brush_bitmap (self);
  gimp_brush_select_preview_fill_draw (self->preview, mask);
  g_free (mask.mask_data);
}



/* Return the mask portion of self's brush's data.
 * Caller must free mask_data.
 */
static _PreviewBitmap
gimp_brush_select_button_get_brush_bitmap (GimpBrushSelectButton *self)
{
  GimpBrush                    *brush;
  gint                          mask_bpp;
  GBytes                       *mask_data;
  gsize                         mask_size;
  gint                          color_bpp;
  GBytes                       *color_data;
  _PreviewBitmap                result;

  g_object_get (self, "resource", &brush, NULL);

  gimp_brush_get_pixels (brush,
                         &result.width,
                         &result.height,
                         &mask_bpp,
                         &mask_data,
                         &color_bpp,
                         &color_data);

  result.mask_data = g_bytes_unref_to_data (mask_data, &mask_size);
  /* Discard any color data, bitmap is B&W i.e. i.e. depth one i.e. a mask */
  g_bytes_unref (color_data);

  return result;
}


/* On resize event, redraw. */
static void
gimp_brush_select_on_preview_resize (GimpBrushSelectButton *self)
{
  gimp_brush_select_button_draw (self);
}


/* On mouse events in self's preview, popup a zoom view of entire brush */
static gboolean
gimp_brush_select_on_preview_events (GtkWidget             *widget,
                                     GdkEvent              *event,
                                     GimpBrushSelectButton *self)
{
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_add (widget);
          gimp_brush_select_button_open_popup (self,
                                               bevent->x, bevent->y);
        }
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_remove (widget);
          gimp_brush_select_button_close_popup (self);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

/* Draw a GimpPreviewArea with a given bitmap. */
static void
gimp_brush_select_preview_draw (GimpPreviewArea *area,
                                gint             x,
                                gint             y,
                                _PreviewBitmap   mask,
                                gint             rowstride)
{
  const guchar *src;
  guchar       *dest;
  guchar       *buf;
  gint          i, j;

  buf = g_new (guchar, mask.width * mask.height);

  src  = mask.mask_data;
  dest = buf;

  for (j = 0; j < mask.height; j++)
    {
      const guchar *s = src;

      for (i = 0; i < mask.width; i++, s++, dest++)
        *dest = 255 - *s;

      src += rowstride;
    }

  gimp_preview_area_draw (area,
                          x, y, mask.width, mask.height,
                          GIMP_GRAY_IMAGE,
                          buf,
                          mask.width);

  g_free (buf);
}

/* Fill a GimpPreviewArea with a bitmap then draw. */
static void
gimp_brush_select_preview_fill_draw (GtkWidget      *preview,
                                     _PreviewBitmap  mask)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (preview);
  GtkAllocation    allocation;
  gint             x, y;
  gint             width, height;
  _PreviewBitmap   drawn_mask;

  gtk_widget_get_allocation (preview, &allocation);

  width  = MIN (mask.width,  allocation.width);
  height = MIN (mask.height, allocation.height);

  x = ((allocation.width  - width)  / 2);
  y = ((allocation.height - height) / 2);

  if (x || y)
    gimp_preview_area_fill (area,
                            0, 0,
                            allocation.width,
                            allocation.height,
                            0xFF, 0xFF, 0xFF);

  /* Draw same data to new bounds.
   * drawn_mask.mask_data points to same array.
   */
  drawn_mask.width = width;
  drawn_mask.height = height;
  drawn_mask.mask_data = mask.mask_data;

  gimp_brush_select_preview_draw (area,
                                  x, y,
                                  drawn_mask,
                                  mask.width);  /* row stride */
  /* Caller will free mask.mask_data */
}

/* popup methods. */

static void
gimp_brush_select_button_open_popup (GimpBrushSelectButton *self,
                                     gint                   x,
                                     gint                   y)
{
  GtkWidget                    *frame;
  GtkWidget                    *preview;
  GdkMonitor                   *monitor;
  GdkRectangle                  workarea;
  gint                          x_org;
  gint                          y_org;
  _PreviewBitmap mask;

  if (self->popup)
    gimp_brush_select_button_close_popup (self);

  mask = gimp_brush_select_button_get_brush_bitmap (self);

  if (mask.width <= CELL_SIZE && mask.height <= CELL_SIZE)
    return;

  self->popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (self->popup), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (self->popup),
                         gtk_widget_get_screen (GTK_WIDGET (self)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (self->popup), frame);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, mask.width, mask.height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup: near the preview i.e. at mousedown coords */
  gdk_window_get_origin (gtk_widget_get_window (self->preview),
                         &x_org, &y_org);

  monitor = gimp_widget_get_monitor (GTK_WIDGET (self));
  gdk_monitor_get_workarea (monitor, &workarea);

  x = x_org + x - (mask.width  / 2);
  y = y_org + y - (mask.height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - mask.width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - mask.height);

  gtk_window_move (GTK_WINDOW (self->popup), x, y);

  gtk_widget_show (self->popup);

  /*  Draw popup now. Usual events do not cause a draw. */
  gimp_brush_select_preview_draw (GIMP_PREVIEW_AREA (preview),
                                  0, 0,
                                  mask,
                                  mask.width);
  g_free (mask.mask_data);
  gdk_window_set_transient_for (gtk_widget_get_window (self->popup),
                                gtk_widget_get_window (gtk_widget_get_toplevel (GTK_WIDGET (self))));
}

static void
gimp_brush_select_button_close_popup (GimpBrushSelectButton *self)
{
  g_clear_pointer (&self->popup, gtk_widget_destroy);
}
