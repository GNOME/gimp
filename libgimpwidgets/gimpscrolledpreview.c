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


#define POPUP_SIZE  100
#define PEN_WIDTH     3


typedef struct
{
  GtkPolicyType hscr_policy;
  GtkPolicyType vscr_policy;
  gint          drag_x;
  gint          drag_y;
  gint          drag_xoff;
  gint          drag_yoff;
  gboolean      in_drag;
  gint          frozen;
} GimpScrolledPreviewPrivate;

#define GIMP_SCROLLED_PREVIEW_GET_PRIVATE(obj) \
  ((GimpScrolledPreviewPrivate *) ((GimpScrolledPreview *) (obj))->priv)


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
      const GTypeInfo preview_info =
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

  object_class->dispose     = gimp_scrolled_preview_dispose;

  preview_class->set_cursor = gimp_scrolled_preview_set_cursor;

  g_type_class_add_private (object_class, sizeof (GimpScrolledPreviewPrivate));
}

static void
gimp_scrolled_preview_init (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv;
  GtkWidget                  *image;
  GtkObject                  *adj;

  preview->priv = G_TYPE_INSTANCE_GET_PRIVATE (preview,
                                               GIMP_TYPE_SCROLLED_PREVIEW,
                                               GimpScrolledPreviewPrivate);

  priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  preview->nav_popup = NULL;

  priv->hscr_policy = GTK_POLICY_AUTOMATIC;
  priv->vscr_policy = GTK_POLICY_AUTOMATIC;

  priv->in_drag     = FALSE;
  priv->frozen      = 1;  /* we are frozen during init */

  /*  scrollbars  */
  adj = gtk_adjustment_new (0, 0, GIMP_PREVIEW (preview)->width - 1, 1.0,
                            GIMP_PREVIEW (preview)->width,
                            GIMP_PREVIEW (preview)->width);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_scrolled_preview_h_scroll),
                    preview);

  preview->hscr = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->hscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (GIMP_PREVIEW (preview)->table),
                    preview->hscr, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  adj = gtk_adjustment_new (0, 0, GIMP_PREVIEW (preview)->height - 1, 1.0,
                            GIMP_PREVIEW (preview)->height,
                            GIMP_PREVIEW (preview)->height);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_scrolled_preview_v_scroll),
                    preview);

  preview->vscr = gtk_vscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->vscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (GIMP_PREVIEW (preview)->table),
                    preview->vscr, 1, 2, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  /* Connect after here so that plug-ins get a chance to override the
   * default behavior. See bug #364432.
   */
  g_signal_connect_after (GIMP_PREVIEW (preview)->area, "event",
                          G_CALLBACK (gimp_scrolled_preview_area_event),
                          preview);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "realize",
                    G_CALLBACK (gimp_scrolled_preview_area_realize),
                    preview);
  g_signal_connect (GIMP_PREVIEW (preview)->area, "unrealize",
                    G_CALLBACK (gimp_scrolled_preview_area_unrealize),
                    preview);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "size-allocate",
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

  g_signal_connect (preview->nav_icon, "button-press-event",
                    G_CALLBACK (gimp_scrolled_preview_nav_button_press),
                    preview);

  priv->frozen = 0;  /* thaw without actually calling draw/invalidate */
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
gimp_scrolled_preview_hscr_update (GimpScrolledPreview *preview)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  gint           width;

  width = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;

  adj->lower          = 0;
  adj->upper          = width;
  adj->page_size      = GIMP_PREVIEW (preview)->width;
  adj->step_increment = 1.0;
  adj->page_increment = MAX (adj->page_size / 2.0, adj->step_increment);
  adj->value          = CLAMP (adj->value,
                               adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_changed (adj);
}

static void
gimp_scrolled_preview_vscr_update (GimpScrolledPreview *preview)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  gint           height;

  height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  adj->lower          = 0;
  adj->upper          = height;
  adj->page_size      = GIMP_PREVIEW (preview)->height;
  adj->step_increment = 1.0;
  adj->page_increment = MAX (adj->page_size / 2.0, adj->step_increment);
  adj->value          = CLAMP (adj->value,
                               adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_changed (adj);
}

static void
gimp_scrolled_preview_area_size_allocate (GtkWidget           *widget,
                                          GtkAllocation       *allocation,
                                          GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  gint width  = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;
  gint height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  gimp_scrolled_preview_freeze (preview);

  GIMP_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  GIMP_PREVIEW (preview)->height = MIN (height, allocation->height);

  gimp_scrolled_preview_hscr_update (preview);

  switch (priv->hscr_policy)
    {
    case GTK_POLICY_AUTOMATIC:
      if (width > GIMP_PREVIEW (preview)->width)
        gtk_widget_show (preview->hscr);
      else
        gtk_widget_hide (preview->hscr);
      break;

    case GTK_POLICY_ALWAYS:
      gtk_widget_show (preview->hscr);
      break;

    case GTK_POLICY_NEVER:
      gtk_widget_hide (preview->hscr);
      break;
    }

  gimp_scrolled_preview_vscr_update (preview);

  switch (priv->vscr_policy)
    {
    case GTK_POLICY_AUTOMATIC:
      if (height > GIMP_PREVIEW (preview)->height)
        gtk_widget_show (preview->vscr);
      else
        gtk_widget_hide (preview->vscr);
      break;

    case GTK_POLICY_ALWAYS:
      gtk_widget_show (preview->vscr);
      break;

    case GTK_POLICY_NEVER:
      gtk_widget_hide (preview->vscr);
      break;
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

  gimp_scrolled_preview_thaw (preview);
}

static gboolean
gimp_scrolled_preview_area_event (GtkWidget           *area,
                                  GdkEvent            *event,
                                  GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);
  GdkEventButton             *button_event = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (button_event->button)
        {
        case 1:
        case 2:
          gtk_widget_get_pointer (area, &priv->drag_x, &priv->drag_y);

          priv->drag_xoff = GIMP_PREVIEW (preview)->xoff;
          priv->drag_yoff = GIMP_PREVIEW (preview)->yoff;
          priv->in_drag   = TRUE;
          gtk_grab_add (area);
          break;

        case 3:
          return TRUE;
        }
      break;

    case GDK_BUTTON_RELEASE:
      if (priv->in_drag &&
          (button_event->button == 1 || button_event->button == 2))
        {
          gtk_grab_remove (area);
          priv->in_drag = FALSE;
        }
      break;

    case GDK_MOTION_NOTIFY:
      if (priv->in_drag)
        {
          GtkAdjustment *hadj;
          GtkAdjustment *vadj;
          gint           x, y;

          hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
          vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

          gtk_widget_get_pointer (area, &x, &y);

          x = priv->drag_xoff - (x - priv->drag_x);
          y = priv->drag_yoff - (y - priv->drag_y);

          x = CLAMP (x, hadj->lower, hadj->upper - hadj->page_size);
          y = CLAMP (y, vadj->lower, vadj->upper - vadj->page_size);

          if (GIMP_PREVIEW (preview)->xoff != x ||
              GIMP_PREVIEW (preview)->yoff != y)
            {
              gtk_adjustment_set_value (hadj, x);
              gtk_adjustment_set_value (vadj, y);

              gimp_preview_draw (GIMP_PREVIEW (preview));
              gimp_preview_invalidate (GIMP_PREVIEW (preview));
            }
        }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll     *sevent    = (GdkEventScroll *) event;
        GdkScrollDirection  direction = sevent->direction;
        GtkAdjustment      *adj;
        gfloat              value;

        /*  Ctrl-Scroll is reserved for zooming  */
        if (sevent->state & GDK_CONTROL_MASK)
          return FALSE;

        if (sevent->state & GDK_SHIFT_MASK)
          switch (direction)
            {
            case GDK_SCROLL_UP:    direction = GDK_SCROLL_LEFT;  break;
            case GDK_SCROLL_DOWN:  direction = GDK_SCROLL_RIGHT; break;
            case GDK_SCROLL_LEFT:  direction = GDK_SCROLL_UP;    break;
            case GDK_SCROLL_RIGHT: direction = GDK_SCROLL_DOWN;  break;
            }

        switch (direction)
          {
          case GDK_SCROLL_UP:
          case GDK_SCROLL_DOWN:
          default:
            adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
            break;

          case GDK_SCROLL_RIGHT:
          case GDK_SCROLL_LEFT:
            adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
            break;
          }

        value = gtk_adjustment_get_value (adj);

        switch (direction)
          {
          case GDK_SCROLL_UP:
          case GDK_SCROLL_LEFT:
            value -= adj->page_increment / 2;
            break;

          case GDK_SCROLL_DOWN:
          case GDK_SCROLL_RIGHT:
            value += adj->page_increment / 2;
            break;
          }

        gtk_adjustment_set_value (adj, CLAMP (value,
                                              adj->lower,
                                              adj->upper - adj->page_size));
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
  GimpScrolledPreviewPrivate *priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  preview->xoff = hadj->value;

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (preview->area),
                                 preview->xoff, preview->yoff);

  if (! (priv->in_drag || priv->frozen))
    {
      gimp_preview_draw (preview);
      gimp_preview_invalidate (preview);
    }
}

static void
gimp_scrolled_preview_v_scroll (GtkAdjustment *vadj,
                                GimpPreview   *preview)
{
  GimpScrolledPreviewPrivate *priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  preview->yoff = vadj->value;

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (preview->area),
                                 preview->xoff, preview->yoff);

  if (! (priv->in_drag || priv->frozen))
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
  GimpPreview   *gimp_preview = GIMP_PREVIEW (preview);
  GtkAdjustment *adj;

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

      area = g_object_new (GIMP_TYPE_PREVIEW_AREA,
                           "check-size", GIMP_CHECK_SIZE_SMALL_CHECKS,
                           "check-type", GIMP_PREVIEW_AREA (gimp_preview->area)->check_type,
                           NULL);

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
      g_signal_connect_after (area, "expose-event",
                              G_CALLBACK (gimp_scrolled_preview_nav_popup_expose),
                              preview);

      GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb (gimp_preview,
                                                    GIMP_PREVIEW_AREA (area),
                                                    POPUP_SIZE, POPUP_SIZE);
      gtk_widget_realize (area);
      gtk_widget_show (area);

      gdk_window_get_origin (widget->window, &x, &y);

      adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
      h = (adj->value / adj->upper) + (adj->page_size / adj->upper) / 2.0;

      adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
      v = (adj->value / adj->upper) + (adj->page_size / adj->upper) / 2.0;

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
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_BUTTON_MOTION_MASK  |
                        GDK_POINTER_MOTION_HINT_MASK,
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
        GtkAdjustment *hadj;
        GtkAdjustment *vadj;
        gint           cx, cy;
        gdouble        x, y;

        hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
        vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

        gtk_widget_get_pointer (widget, &cx, &cy);

        x = cx * (hadj->upper - hadj->lower) / widget->allocation.width;
        y = cy * (vadj->upper - vadj->lower) / widget->allocation.height;

        x += hadj->lower - hadj->page_size / 2;
        y += vadj->lower - vadj->page_size / 2;

        gtk_adjustment_set_value (hadj, CLAMP (x,
                                               hadj->lower,
                                               hadj->upper - hadj->page_size));
        gtk_adjustment_set_value (vadj, CLAMP (y,
                                               vadj->lower,
                                               vadj->upper - vadj->page_size));

        gtk_widget_queue_draw (widget);
        gdk_window_process_updates (widget->window, FALSE);
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
  GtkAdjustment *adj;
  gdouble        x, y;
  gdouble        w, h;

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));

  x = adj->value / (adj->upper - adj->lower);
  w = adj->page_size / (adj->upper - adj->lower);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

  y = adj->value / (adj->upper - adj->lower);
  h = adj->page_size / (adj->upper - adj->lower);

  if (w >= 1.0 && h >= 1.0)
    return FALSE;

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
  if (! GTK_WIDGET_REALIZED (preview->area))
    return;

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

/**
 * gimp_scrolled_preview_set_position:
 * @preview: a #GimpScrolledPreview
 * @x:       horizontal scroll offset
 * @y:       vertical scroll offset
 *
 * Since: GIMP 2.4
 **/
void
gimp_scrolled_preview_set_position (GimpScrolledPreview *preview,
                                    gint                 x,
                                    gint                 y)
{
  GtkAdjustment *adj;

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

  gimp_scrolled_preview_freeze (preview);

  gimp_scrolled_preview_hscr_update (preview);
  gimp_scrolled_preview_vscr_update (preview);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  gtk_adjustment_set_value (adj, x - GIMP_PREVIEW (preview)->xmin);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  gtk_adjustment_set_value (adj, y - GIMP_PREVIEW (preview)->ymin);

  gimp_scrolled_preview_thaw (preview);
}

/**
 * gimp_scrolled_preview_set_policy
 * @preview:           a #GimpScrolledPreview
 * @hscrollbar_policy: policy for horizontal scrollbar
 * @vscrollbar_policy: policy for vertical scrollbar
 *
 * Since: GIMP 2.4
 **/
void
gimp_scrolled_preview_set_policy (GimpScrolledPreview *preview,
                                  GtkPolicyType        hscrollbar_policy,
                                  GtkPolicyType        vscrollbar_policy)
{
  GimpScrolledPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

  priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  priv->hscr_policy = hscrollbar_policy;
  priv->vscr_policy = vscrollbar_policy;

  gtk_widget_queue_resize (GIMP_PREVIEW (preview)->area);
}


/**
 * gimp_scrolled_preview_freeze:
 * @preview: a #GimpScrolledPreview
 *
 * While the @preview is frozen, it is not going to redraw itself in
 * response to scroll events.
 *
 * This function should only be used to implement widgets derived from
 * #GimpScrolledPreview. There is no point in calling this from a plug-in.
 *
 * Since: GIMP 2.4
 **/
void
gimp_scrolled_preview_freeze (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

  priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  priv->frozen++;
}

/**
 * gimp_scrolled_preview_thaw:
 * @preview: a #GimpScrolledPreview
 *
 * While the @preview is frozen, it is not going to redraw itself in
 * response to scroll events.
 *
 * This function should only be used to implement widgets derived from
 * #GimpScrolledPreview. There is no point in calling this from a plug-in.
 *
 * Since: GIMP 2.4
 **/
void
gimp_scrolled_preview_thaw (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

  priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);

  g_return_if_fail (priv->frozen > 0);

  priv->frozen--;

  if (! priv->frozen)
    {
      gimp_preview_draw (GIMP_PREVIEW (preview));
      gimp_preview_invalidate (GIMP_PREVIEW (preview));
    }
}
