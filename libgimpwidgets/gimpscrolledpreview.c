/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscrolledpreview.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgets.h"

#include "gimpscrolledpreview.h"

#include "libgimp/libgimp-intl.h"


#define POPUP_SIZE       100
#define PEN_WIDTH          3


static void      gimp_scrolled_preview_class_init          (GimpScrolledPreviewClass *klass);
static void      gimp_scrolled_preview_init                (GimpScrolledPreview      *preview);
static void      gimp_scrolled_preview_dispose             (GObject                  *object);

static void      gimp_scrolled_preview_area_realize        (GtkWidget                *widget,
                                                            GimpScrolledPreview      *preview);
static void      gimp_scrolled_preview_area_unrealize      (GtkWidget                *widget,
                                                            GimpScrolledPreview      *preview);
static void      gimp_scrolled_preview_area_size_allocate  (GtkWidget                *widget,
                                                            GtkAllocation            *allocation,
                                                            GimpScrolledPreview      *preview);
static gboolean  gimp_scrolled_preview_area_event          (GtkWidget                *area,
                                                            GdkEvent                 *event,
                                                            GimpScrolledPreview      *preview);

static void      gimp_scrolled_preview_h_scroll            (GtkAdjustment            *hadj,
                                                            GimpPreview              *preview);
static void      gimp_scrolled_preview_v_scroll            (GtkAdjustment            *vadj,
                                                            GimpPreview              *preview);

static gboolean  gimp_scrolled_preview_nav_button_press    (GtkWidget                *widget,
                                                            GdkEventButton           *event,
                                                            GimpScrolledPreview      *preview);

static void      gimp_scrolled_preview_nav_popup_realize   (GtkWidget                *widget,
                                                            GimpScrolledPreview      *preview);
static void      gimp_scrolled_preview_nav_popup_unrealize (GtkWidget                *widget,
                                                            GimpScrolledPreview      *preview);
static gboolean  gimp_scrolled_preview_nav_popup_event     (GtkWidget                *widget,
                                                            GdkEvent                 *event,
                                                            GimpScrolledPreview      *preview);
static gboolean  gimp_scrolled_preview_nav_popup_expose    (GtkWidget                *widget,
                                                            GdkEventExpose           *event,
                                                            GimpScrolledPreview      *preview);
static void      gimp_scrolled_preview_set_cursor          (GimpPreview              *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_scrolled_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpScrolledPreviewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_scrolled_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpScrolledPreview),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_scrolled_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpScrolledPreview",
                                             &preview_info,
                                             G_TYPE_FLAG_ABSTRACT);
    }

  return preview_type;
}

static void
gimp_scrolled_preview_class_init (GimpScrolledPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose      = gimp_scrolled_preview_dispose;

  preview_class->set_cursor  = gimp_scrolled_preview_set_cursor;
}

static void
gimp_scrolled_preview_init (GimpScrolledPreview *preview)
{
  GtkWidget *image;
  GtkObject *adj;

  preview->in_drag   = FALSE;
  preview->nav_popup = NULL;

  /*  scrollbars  */
  adj = gtk_adjustment_new (0, 0, GIMP_PREVIEW (preview)->width - 1, 1.0,
                            GIMP_PREVIEW (preview)->width,
                            GIMP_PREVIEW (preview)->width);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_scrolled_preview_h_scroll),
                    preview);

  preview->hscr = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->hscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (GIMP_PREVIEW (preview)->table),
                    preview->hscr, 0,1, 1,2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);

  adj = gtk_adjustment_new (0, 0, GIMP_PREVIEW (preview)->height - 1, 1.0,
                            GIMP_PREVIEW (preview)->height,
                            GIMP_PREVIEW (preview)->height);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_scrolled_preview_v_scroll),
                    preview);

  preview->vscr = gtk_vscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->vscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (GIMP_PREVIEW (preview)->table),
                    preview->vscr, 1,2, 0,1,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "event",
                    G_CALLBACK (gimp_scrolled_preview_area_event),
                    preview);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "realize",
                    G_CALLBACK (gimp_scrolled_preview_area_realize),
                    preview);
  g_signal_connect (GIMP_PREVIEW (preview)->area, "unrealize",
                    G_CALLBACK (gimp_scrolled_preview_area_unrealize),
                    preview);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "size_allocate",
                    G_CALLBACK (gimp_scrolled_preview_area_size_allocate),
                    preview);

  /*  navigation icon  */
  preview->nav_icon = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (GIMP_PREVIEW(preview)->table),
                    preview->nav_icon, 1,2, 1,2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);

  image = gtk_image_new_from_stock (GIMP_STOCK_NAVIGATION, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (preview->nav_icon), image);
  gtk_widget_show (image);

  g_signal_connect (preview->nav_icon, "button_press_event",
                    G_CALLBACK (gimp_scrolled_preview_nav_button_press),
                    preview);
}

static void
gimp_scrolled_preview_dispose (GObject *object)
{
  GimpScrolledPreview *preview = GIMP_SCROLLED_PREVIEW (object);

  if (preview->nav_popup)
    {
      gtk_widget_destroy (preview->nav_popup);
      preview->nav_popup = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_scrolled_preview_area_realize (GtkWidget           *widget,
                                    GimpScrolledPreview *preview)
{
  GdkDisplay *display = gtk_widget_get_display (widget);

  g_return_if_fail (preview->cursor_move == NULL);

  preview->cursor_move = gdk_cursor_new_for_display (display, GDK_FLEUR);

  gimp_scrolled_preview_set_cursor (GIMP_PREVIEW (preview));
}

static void
gimp_scrolled_preview_area_unrealize (GtkWidget           *widget,
                                      GimpScrolledPreview *preview)
{
  if (preview->cursor_move)
    {
      gdk_cursor_unref (preview->cursor_move);
      preview->cursor_move = NULL;
    }
}

static void
gimp_scrolled_preview_area_size_allocate (GtkWidget           *widget,
                                          GtkAllocation       *allocation,
                                          GimpScrolledPreview *preview)
{
  GdkCursor *cursor = GIMP_PREVIEW (preview)->default_cursor;
  gint       width  = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;
  gint       height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  GIMP_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  GIMP_PREVIEW (preview)->height = MIN (height, allocation->height);

  if (width > GIMP_PREVIEW (preview)->width)
    {
      GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));

      adj->lower          = 0;
      adj->upper          = width;
      adj->page_size      = GIMP_PREVIEW (preview)->width;
      adj->step_increment = 1.0;
      adj->page_increment = MAX (adj->page_size / 2.0, adj->step_increment);

      gtk_adjustment_changed (adj);

      gtk_widget_show (preview->hscr);

      cursor = preview->cursor_move;
    }
  else
    {
      gtk_widget_hide (preview->hscr);
    }

  if (height > GIMP_PREVIEW (preview)->height)
    {
      GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

      adj->lower          = 0;
      adj->upper          = height;
      adj->page_size      = GIMP_PREVIEW (preview)->height;
      adj->step_increment = 1.0;
      adj->page_increment = MAX (adj->page_size / 2.0, adj->step_increment);

      gtk_adjustment_changed (adj);

      gtk_widget_show (preview->vscr);

      cursor = preview->cursor_move;
    }
  else
    {
      gtk_widget_hide (preview->vscr);
    }

  if (GTK_WIDGET_VISIBLE (preview->vscr) &&
      GTK_WIDGET_VISIBLE (preview->hscr) &&
      GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb)
    {
      gtk_widget_show (preview->nav_icon);
    }
  else
    {
      gtk_widget_hide (preview->nav_icon);
    }

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_set_cursor (widget->window, cursor);

  gimp_preview_draw (GIMP_PREVIEW (preview));
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static gboolean
gimp_scrolled_preview_area_event (GtkWidget           *area,
                                  GdkEvent            *event,
                                  GimpScrolledPreview *preview)
{
  GdkEventButton *button_event = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (button_event->button)
        {
        case 1:
          gtk_widget_get_pointer (area, &preview->drag_x, &preview->drag_y);

          preview->drag_xoff = GIMP_PREVIEW (preview)->xoff;
          preview->drag_yoff = GIMP_PREVIEW (preview)->yoff;

          preview->in_drag = TRUE;
          gtk_grab_add (area);
          break;

        case 2:
          break;

        case 3:
          return TRUE;
        }
      break;

    case GDK_BUTTON_RELEASE:
      if (preview->in_drag && button_event->button == 1)
        {
          gtk_grab_remove (area);
          preview->in_drag = FALSE;
        }
      break;

    case GDK_MOTION_NOTIFY:
      if (preview->in_drag)
        {
          gint x, y;
          gint dx, dy;
          gint xoff, yoff;

          gtk_widget_get_pointer (area, &x, &y);

          dx = x - preview->drag_x;
          dy = y - preview->drag_y;

          xoff = CLAMP (preview->drag_xoff - dx,
                        0,
                        GIMP_PREVIEW (preview)->xmax -
                        GIMP_PREVIEW (preview)->xmin -
                        GIMP_PREVIEW (preview)->width);
          yoff = CLAMP (preview->drag_yoff - dy,
                        0,
                        GIMP_PREVIEW (preview)->ymax -
                        GIMP_PREVIEW (preview)->ymin -
                        GIMP_PREVIEW (preview)->height);

          if (GIMP_PREVIEW (preview)->xoff != xoff ||
              GIMP_PREVIEW (preview)->yoff != yoff)
            {
              gtk_range_set_value (GTK_RANGE (preview->hscr), xoff);
              gtk_range_set_value (GTK_RANGE (preview->vscr), yoff);

              gimp_preview_draw (GIMP_PREVIEW (preview));
              gimp_preview_invalidate (GIMP_PREVIEW (preview));
            }
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_scrolled_preview_h_scroll (GtkAdjustment *hadj,
                                GimpPreview   *preview)
{
  preview->xoff = hadj->value;

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (preview->area),
                                 preview->xoff, preview->yoff);

  if (! GIMP_SCROLLED_PREVIEW (preview)->in_drag)
    {
      gimp_preview_draw (preview);
      gimp_preview_invalidate (preview);
    }
}

static void
gimp_scrolled_preview_v_scroll (GtkAdjustment *vadj,
                                GimpPreview   *preview)
{
  preview->yoff = vadj->value;

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (preview->area),
                                 preview->xoff, preview->yoff);

  if (! GIMP_SCROLLED_PREVIEW (preview)->in_drag)
    {
      gimp_preview_draw (preview);
      gimp_preview_invalidate (preview);
    }
}

static gboolean
gimp_scrolled_preview_nav_button_press (GtkWidget           *widget,
                                        GdkEventButton      *event,
                                        GimpScrolledPreview *preview)
{
  GimpPreview *gimp_preview = GIMP_PREVIEW (preview);

  if (preview->nav_popup)
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      GtkWidget  *outer;
      GtkWidget  *inner;
      GtkWidget  *area;
      GdkCursor  *cursor;
      gint        x, y;
      gdouble     h, v;

      preview->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);

      gtk_window_set_screen (GTK_WINDOW (preview->nav_popup),
                             gtk_widget_get_screen (widget));

      outer = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (outer), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (preview->nav_popup), outer);
      gtk_widget_show (outer);

      inner = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (inner), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (outer), inner);
      gtk_widget_show (inner);

      area = gimp_preview_area_new ();
      gtk_container_add (GTK_CONTAINER (inner), area);

      g_signal_connect (area, "realize",
                        G_CALLBACK (gimp_scrolled_preview_nav_popup_realize),
                        preview);
      g_signal_connect (area, "unrealize",
                        G_CALLBACK (gimp_scrolled_preview_nav_popup_unrealize),
                        preview);
      g_signal_connect (area, "event",
                        G_CALLBACK (gimp_scrolled_preview_nav_popup_event),
                        preview);
      g_signal_connect_after (area, "expose_event",
                              G_CALLBACK (gimp_scrolled_preview_nav_popup_expose),
                              preview);

      GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb (gimp_preview,
                                                    GIMP_PREVIEW_AREA (area),
                                                    POPUP_SIZE, POPUP_SIZE);
      gtk_widget_realize (area);
      gtk_widget_show (area);

      gdk_window_get_origin (widget->window, &x, &y);

      h = ((gdouble) (gimp_preview->xoff + gimp_preview->width  / 2) /
           (gdouble) (gimp_preview->xmax - gimp_preview->xmin));
      v = ((gdouble) (gimp_preview->yoff + gimp_preview->height / 2) /
           (gdouble) (gimp_preview->ymax - gimp_preview->ymin));

      x += event->x - h * (gdouble) GIMP_PREVIEW_AREA (area)->width;
      y += event->y - v * (gdouble) GIMP_PREVIEW_AREA (area)->height;

      gtk_window_move (GTK_WINDOW (preview->nav_popup),
                       x - 2 * widget->style->xthickness,
                       y - 2 * widget->style->ythickness);

      gtk_widget_show (preview->nav_popup);

      gtk_grab_add (area);

      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                           GDK_FLEUR);

      gdk_pointer_grab (area->window, TRUE,
                        GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK,
                        area->window, cursor,
                        event->time);

      gdk_cursor_unref (cursor);
    }

  return TRUE;
}

static void
gimp_scrolled_preview_nav_popup_realize (GtkWidget           *widget,
                                         GimpScrolledPreview *preview)
{
  if (! preview->nav_gc)
    {
      preview->nav_gc = gdk_gc_new (widget->window);

      gdk_gc_set_function (preview->nav_gc, GDK_INVERT);
      gdk_gc_set_line_attributes (preview->nav_gc,
                                  PEN_WIDTH,
                                  GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    }
}

static void
gimp_scrolled_preview_nav_popup_unrealize (GtkWidget           *widget,
                                           GimpScrolledPreview *preview)
{
  if (preview->nav_gc)
    {
      g_object_unref (preview->nav_gc);
      preview->nav_gc = NULL;
    }
}

static gboolean
gimp_scrolled_preview_nav_popup_event (GtkWidget           *widget,
                                       GdkEvent            *event,
                                       GimpScrolledPreview *preview)
{
  switch (event->type)
    {
    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *button_event = (GdkEventButton *) event;

        if (button_event->button == 1)
          {
            gtk_grab_remove (widget);
            gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                        button_event->time);

            gtk_widget_destroy (preview->nav_popup);
            preview->nav_popup = NULL;
          }
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *motion_event = (GdkEventMotion *) event;
        GtkAdjustment  *hadj;
        GtkAdjustment  *vadj;
        gdouble         x, y;

        hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
        vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

        x = (motion_event->x *
             (hadj->upper - hadj->lower) / widget->allocation.width);
        y = (motion_event->y *
             (vadj->upper - vadj->lower) / widget->allocation.height);

        x += hadj->lower - hadj->page_size / 2;
        y += vadj->lower - vadj->page_size / 2;

        gtk_adjustment_set_value (hadj, CLAMP (x,
                                               hadj->lower,
                                               hadj->upper - hadj->page_size));
        gtk_adjustment_set_value (vadj, CLAMP (y,
                                               vadj->lower,
                                               vadj->upper - vadj->page_size));

        gtk_widget_queue_draw (widget);
      }
      break;

  default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_scrolled_preview_nav_popup_expose (GtkWidget           *widget,
                                        GdkEventExpose      *event,
                                        GimpScrolledPreview *preview)
{
  GimpPreview *gimp_preview = GIMP_PREVIEW (preview);
  gdouble      x, y;
  gdouble      w, h;

  x = ((gdouble) gimp_preview->xoff /
       (gdouble) (gimp_preview->xmax - gimp_preview->xmin));
  y = ((gdouble) gimp_preview->yoff /
       (gdouble) (gimp_preview->ymax - gimp_preview->ymin));

  w = ((gdouble) gimp_preview->width  /
       (gdouble) (gimp_preview->xmax - gimp_preview->xmin));
  h = ((gdouble) gimp_preview->height /
       (gdouble) (gimp_preview->ymax - gimp_preview->ymin));

  gdk_gc_set_clip_rectangle (preview->nav_gc, &event->area);

  gdk_draw_rectangle (widget->window, preview->nav_gc,
                      FALSE,
                      x * (gdouble) widget->allocation.width  + PEN_WIDTH / 2,
                      y * (gdouble) widget->allocation.height + PEN_WIDTH / 2,
                      MAX (1,
                           ceil (w * widget->allocation.width)  - PEN_WIDTH),
                      MAX (1,
                           ceil (h * widget->allocation.height) - PEN_WIDTH));

  return FALSE;
}

static void
gimp_scrolled_preview_set_cursor (GimpPreview *preview)
{
  if (preview->xmax - preview->xmin > preview->width  ||
      preview->ymax - preview->ymin > preview->height)
    {
      gdk_window_set_cursor (preview->area->window,
                             GIMP_SCROLLED_PREVIEW (preview)->cursor_move);
    }
  else
    {
      gdk_window_set_cursor (preview->area->window, preview->default_cursor);
    }
}
