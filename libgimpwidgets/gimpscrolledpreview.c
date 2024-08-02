/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscrolledpreview.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpicons.h"
#include "gimppreviewarea.h"
#include "gimpscrolledpreview.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpscrolledpreview
 * @title: GimpScrolledPreview
 * @short_description: A widget providing a #GimpPreview enhanced by
 *                     scrolling capabilities.
 *
 * A widget providing a #GimpPreview enhanced by scrolling capabilities.
 **/


#define POPUP_SIZE  100


typedef struct _GimpScrolledPreviewPrivate
{
  GtkWidget     *hscr;
  GtkWidget     *vscr;
  GtkWidget     *nav_icon;
  GtkWidget     *nav_popup;
  GdkCursor     *cursor_move;
  GtkPolicyType  hscr_policy;
  GtkPolicyType  vscr_policy;
  gint           drag_xoff;
  gint           drag_yoff;
  gboolean       in_drag;
  gint           frozen;
} GimpScrolledPreviewPrivate;

#define GET_PRIVATE(obj) (gimp_scrolled_preview_get_instance_private ((GimpScrolledPreview *) (obj)))


static void      gimp_scrolled_preview_dispose             (GObject                  *object);

static void      gimp_scrolled_preview_drag_begin          (GtkGestureDrag           *gesture,
                                                            gdouble                   start_x,
                                                            gdouble                   start_y,
                                                            gpointer                  user_data);
static void      gimp_scrolled_preview_drag_update         (GtkGestureDrag           *gesture,
                                                            gdouble                   offset_x,
                                                            gdouble                   offset_y,
                                                            gpointer                  user_data);
static void      gimp_scrolled_preview_drag_end            (GtkGestureDrag           *gesture,
                                                            gdouble                   offset_x,
                                                            gdouble                   offset_y,
                                                            gpointer                  user_data);
static void      gimp_scrolled_preview_drag_cancel         (GtkGesture               *gesture,
                                                            GdkEventSequence         *sequence,
                                                            gpointer                  user_data);

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

static gboolean  gimp_scrolled_preview_nav_popup_event     (GtkWidget                *widget,
                                                            GdkEvent                 *event,
                                                            GimpScrolledPreview      *preview);
static gboolean  gimp_scrolled_preview_nav_popup_draw      (GtkWidget                *widget,
                                                            cairo_t                  *cr,
                                                            GimpScrolledPreview      *preview);

static void      gimp_scrolled_preview_set_cursor          (GimpPreview              *preview);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpScrolledPreview, gimp_scrolled_preview,
                                     GIMP_TYPE_PREVIEW)

#define parent_class gimp_scrolled_preview_parent_class


static void
gimp_scrolled_preview_class_init (GimpScrolledPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  object_class->dispose     = gimp_scrolled_preview_dispose;

  preview_class->set_cursor = gimp_scrolled_preview_set_cursor;
}

static void
gimp_scrolled_preview_init (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget                  *image;
  GtkWidget                  *grid;
  GtkWidget                  *area;
  GtkGesture                 *gesture;
  GtkAdjustment              *adj;
  gint                        width;
  gint                        height;


  priv->nav_popup = NULL;

  priv->hscr_policy = GTK_POLICY_AUTOMATIC;
  priv->vscr_policy = GTK_POLICY_AUTOMATIC;

  priv->in_drag     = FALSE;
  priv->frozen      = 1;  /* we are frozen during init */

  grid = gimp_preview_get_grid (GIMP_PREVIEW (preview));

  gimp_preview_get_size (GIMP_PREVIEW (preview), &width, &height);

  /*  scrollbars  */
  adj = gtk_adjustment_new (0, 0, width - 1, 1.0, width, width);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_scrolled_preview_h_scroll),
                    preview);

  priv->hscr = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, adj);
  gtk_widget_set_hexpand (priv->hscr, TRUE);
  gtk_grid_attach (GTK_GRID (grid), priv->hscr, 0, 1, 1, 1);

  adj = gtk_adjustment_new (0, 0, height - 1, 1.0, height, height);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_scrolled_preview_v_scroll),
                    preview);

  priv->vscr = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adj);
  gtk_widget_set_vexpand (priv->vscr, TRUE);
  gtk_grid_attach (GTK_GRID (grid), priv->vscr, 1, 0, 1, 1);

  area = gimp_preview_get_area (GIMP_PREVIEW (preview));

  /* Connect after here so that plug-ins get a chance to override the
   * default behavior. See bug #364432.
   */
  g_signal_connect_after (area, "event",
                          G_CALLBACK (gimp_scrolled_preview_area_event),
                          preview);

  /* Allow the user to drag the rectangle on the preview */
  gesture = gtk_gesture_drag_new (GTK_WIDGET (preview));
  g_signal_connect (gesture, "drag-begin",
                    G_CALLBACK (gimp_scrolled_preview_drag_begin),
                    preview);
  g_signal_connect (gesture, "drag-update",
                    G_CALLBACK (gimp_scrolled_preview_drag_update),
                    preview);
  g_signal_connect (gesture, "drag-end",
                    G_CALLBACK (gimp_scrolled_preview_drag_end),
                    preview);
  g_signal_connect (gesture, "cancel",
                    G_CALLBACK (gimp_scrolled_preview_drag_cancel),
                    preview);

  g_signal_connect (area, "realize",
                    G_CALLBACK (gimp_scrolled_preview_area_realize),
                    preview);
  g_signal_connect (area, "unrealize",
                    G_CALLBACK (gimp_scrolled_preview_area_unrealize),
                    preview);

  g_signal_connect (area, "size-allocate",
                    G_CALLBACK (gimp_scrolled_preview_area_size_allocate),
                    preview);

  /*  navigation icon  */
  priv->nav_icon = gtk_event_box_new ();
  gtk_grid_attach (GTK_GRID (grid), priv->nav_icon, 1, 1, 1, 1);

  image = gtk_image_new_from_icon_name (GIMP_ICON_DIALOG_NAVIGATION,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (priv->nav_icon), image);
  gtk_widget_show (image);

  g_signal_connect (priv->nav_icon, "button-press-event",
                    G_CALLBACK (gimp_scrolled_preview_nav_button_press),
                    preview);

  priv->frozen = 0;  /* thaw without actually calling draw/invalidate */
}

static void
gimp_scrolled_preview_dispose (GObject *object)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->nav_popup, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_scrolled_preview_drag_begin (GtkGestureDrag *gesture,
                                  gdouble         start_x,
                                  gdouble         start_y,
                                  gpointer        user_data)
{
  GimpScrolledPreview        *preview = GIMP_SCROLLED_PREVIEW (user_data);
  GimpScrolledPreviewPrivate *priv    = GET_PRIVATE (preview);

  priv->in_drag = TRUE;
  gimp_preview_get_offsets (GIMP_PREVIEW (preview),
                            &priv->drag_xoff, &priv->drag_yoff);

}

static void
gimp_scrolled_preview_drag_update (GtkGestureDrag *gesture,
                                   gdouble         offset_x,
                                   gdouble         offset_y,
                                   gpointer        user_data)
{
  GimpScrolledPreview        *preview = GIMP_SCROLLED_PREVIEW (user_data);
  GimpScrolledPreviewPrivate *priv    = GET_PRIVATE (preview);
  GtkAdjustment              *hadj;
  GtkAdjustment              *vadj;
  gint                        x, y;
  gint                        xoff, yoff;

  hadj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));
  vadj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));

  x = priv->drag_xoff - (int) offset_x;
  y = priv->drag_yoff - (int) offset_y;

  x = CLAMP (x,
             gtk_adjustment_get_lower (hadj),
             gtk_adjustment_get_upper (hadj) -
             gtk_adjustment_get_page_size (hadj));
  y = CLAMP (y,
             gtk_adjustment_get_lower (vadj),
             gtk_adjustment_get_upper (vadj) -
             gtk_adjustment_get_page_size (vadj));

  gimp_preview_get_offsets (GIMP_PREVIEW (preview), &xoff, &yoff);
  if (xoff == x && yoff == y)
    return;

  gtk_adjustment_set_value (hadj, x);
  gtk_adjustment_set_value (vadj, y);

  gimp_preview_draw (GIMP_PREVIEW (preview));
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
gimp_scrolled_preview_drag_end (GtkGestureDrag *gesture,
                                gdouble         offset_x,
                                gdouble         offset_y,
                                gpointer        user_data)
{
  GimpScrolledPreview        *preview = GIMP_SCROLLED_PREVIEW (user_data);
  GimpScrolledPreviewPrivate *priv    = GET_PRIVATE (preview);

  priv->in_drag = FALSE;
}

static void
gimp_scrolled_preview_drag_cancel (GtkGesture       *gesture,
                                   GdkEventSequence *sequence,
                                   gpointer          user_data)
{
  GimpScrolledPreview        *preview = GIMP_SCROLLED_PREVIEW (user_data);
  GimpScrolledPreviewPrivate *priv    = GET_PRIVATE (preview);

  priv->in_drag = FALSE;
}

static void
gimp_scrolled_preview_area_realize (GtkWidget           *widget,
                                    GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv    = GET_PRIVATE (preview);
  GdkDisplay                 *display = gtk_widget_get_display (widget);

  g_return_if_fail (priv->cursor_move == NULL);

  priv->cursor_move = gdk_cursor_new_for_display (display, GDK_HAND1);
}

static void
gimp_scrolled_preview_area_unrealize (GtkWidget           *widget,
                                      GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  g_clear_object (&priv->cursor_move);
}

static void
gimp_scrolled_preview_hscr_update (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkAdjustment              *adj;
  gint                        xmin, xmax;
  gint                        width;

  adj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));

  gimp_preview_get_bounds (GIMP_PREVIEW (preview), &xmin, NULL, &xmax, NULL);
  gimp_preview_get_size (GIMP_PREVIEW (preview), &width, NULL);

  gtk_adjustment_configure (adj,
                            gtk_adjustment_get_value (adj),
                            0, xmax - xmin,
                            1.0,
                            MAX (width / 2.0, 1.0),
                            width);
}

static void
gimp_scrolled_preview_vscr_update (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkAdjustment              *adj;
  gint                        ymin, ymax;
  gint                        height;

  adj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));

  gimp_preview_get_bounds (GIMP_PREVIEW (preview), NULL, &ymin, NULL, &ymax);
  gimp_preview_get_size (GIMP_PREVIEW (preview), NULL, &height);

  gtk_adjustment_configure (adj,
                            gtk_adjustment_get_value (adj),
                            0, ymax - ymin,
                            1.0,
                            MAX (height / 2.0, 1.0),
                            height);
}

static void
gimp_scrolled_preview_area_size_allocate (GtkWidget           *widget,
                                          GtkAllocation       *allocation,
                                          GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                        xmin, ymin;
  gint                        xmax, ymax;
  gint                        width;
  gint                        height;

  gimp_preview_get_bounds (GIMP_PREVIEW (preview), &xmin, &ymin, &xmax, &ymax);
  gimp_preview_get_size (GIMP_PREVIEW (preview), &width, &height);

  gimp_scrolled_preview_freeze (preview);

#if 0
  GIMP_PREVIEW (preview)->width  = MIN (xmax - xmin, allocation->width);
  GIMP_PREVIEW (preview)->height = MIN (ymax - ymin, allocation->height);
#endif

  gimp_scrolled_preview_hscr_update (preview);

  switch (priv->hscr_policy)
    {
    case GTK_POLICY_AUTOMATIC:
      gtk_widget_set_visible (priv->hscr, xmax - xmin > width);
      break;

    case GTK_POLICY_ALWAYS:
      gtk_widget_show (priv->hscr);
      break;

    case GTK_POLICY_NEVER:
    case GTK_POLICY_EXTERNAL:
      gtk_widget_hide (priv->hscr);
      break;
    }

  gimp_scrolled_preview_vscr_update (preview);

  switch (priv->vscr_policy)
    {
    case GTK_POLICY_AUTOMATIC:
      gtk_widget_set_visible (priv->vscr, ymax - ymin > height);
      break;

    case GTK_POLICY_ALWAYS:
      gtk_widget_show (priv->vscr);
      break;

    case GTK_POLICY_NEVER:
    case GTK_POLICY_EXTERNAL:
      gtk_widget_hide (priv->vscr);
      break;
    }

  gtk_widget_set_visible (priv->nav_icon,
                          gtk_widget_get_visible (priv->vscr) &&
                          gtk_widget_get_visible (priv->hscr) &&
                          GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb);

  gimp_scrolled_preview_thaw (preview);
}

static gboolean
gimp_scrolled_preview_area_event (GtkWidget           *area,
                                  GdkEvent            *event,
                                  GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  if (event->type == GDK_SCROLL)
    {
      GdkEventScroll *sevent = (GdkEventScroll *) event;
      GtkAdjustment  *adj_x;
      GtkAdjustment  *adj_y;
      gdouble         value_x;
      gdouble         value_y;

      /*  Ctrl-Scroll is reserved for zooming  */
      if (sevent->state & GDK_CONTROL_MASK)
        return FALSE;

      adj_x = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));
      adj_y = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));

      gimp_scroll_adjustment_values (sevent,
                                     adj_x, adj_y,
                                     &value_x, &value_y);

      gtk_adjustment_set_value (adj_x, value_x);
      gtk_adjustment_set_value (adj_y, value_y);
    }

  return FALSE;
}

static void
gimp_scrolled_preview_h_scroll (GtkAdjustment *hadj,
                                GimpPreview   *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget                  *area;
  gint                        xoff, yoff;

  gimp_preview_get_offsets (preview, NULL, &yoff);

  xoff = gtk_adjustment_get_value (hadj);

  gimp_preview_set_offsets (preview, xoff, yoff);

  area = gimp_preview_get_area (preview);

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (area), xoff, yoff);

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
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget                  *area;
  gint                        xoff, yoff;

  gimp_preview_get_offsets (preview, &xoff, NULL);

  yoff = gtk_adjustment_get_value (vadj);

  gimp_preview_set_offsets (preview, xoff, yoff);

  area = gimp_preview_get_area (preview);

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (area), xoff, yoff);

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
  GimpScrolledPreviewPrivate *priv         = GET_PRIVATE (preview);
  GimpPreview                *gimp_preview = GIMP_PREVIEW (preview);
  GtkAdjustment              *adj;

  if (priv->nav_popup)
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      GtkStyleContext *context = gtk_widget_get_style_context (widget);
      GtkWidget       *toplevel;
      GtkWidget       *outer;
      GtkWidget       *inner;
      GtkWidget       *area;
      GdkCursor       *cursor;
      GtkBorder        border;
      GimpCheckType    check_type;
      GeglColor       *check_custom_color1;
      GeglColor       *check_custom_color2;
      gint             area_width;
      gint             area_height;
      gint             x, y;
      gdouble          h, v;

      priv->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (preview));
      if (GTK_IS_WINDOW (toplevel))
        {
          gtk_window_set_transient_for (GTK_WINDOW (priv->nav_popup),
                                        GTK_WINDOW (toplevel));
        }

      gtk_window_set_screen (GTK_WINDOW (priv->nav_popup),
                             gtk_widget_get_screen (widget));

      outer = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (outer), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (priv->nav_popup), outer);
      gtk_widget_show (outer);

      inner = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (inner), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (outer), inner);
      gtk_widget_show (inner);

      g_object_get (gimp_preview_get_area (gimp_preview),
                    "check-type", &check_type,
                    "check-custom-color1", &check_custom_color1,
                    "check-custom-color2", &check_custom_color2,
                    NULL);

      area = g_object_new (GIMP_TYPE_PREVIEW_AREA,
                           "check-size", GIMP_CHECK_SIZE_SMALL_CHECKS,
                           "check-type", check_type,
                           "check-custom-color1", check_custom_color1,
                           "check-custom-color2", check_custom_color2,
                           NULL);

      gtk_container_add (GTK_CONTAINER (inner), area);

      g_signal_connect (area, "event",
                        G_CALLBACK (gimp_scrolled_preview_nav_popup_event),
                        preview);
      g_signal_connect_after (area, "draw",
                              G_CALLBACK (gimp_scrolled_preview_nav_popup_draw),
                              preview);

      GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb (gimp_preview,
                                                    GIMP_PREVIEW_AREA (area),
                                                    POPUP_SIZE, POPUP_SIZE);
      gtk_widget_realize (area);
      gtk_widget_show (area);

      gdk_window_get_origin (gtk_widget_get_window (widget), &x, &y);

      adj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));
      h = ((gtk_adjustment_get_value (adj) /
            gtk_adjustment_get_upper (adj)) +
           (gtk_adjustment_get_page_size (adj) /
            gtk_adjustment_get_upper (adj)) / 2.0);

      adj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));
      v = ((gtk_adjustment_get_value (adj) /
            gtk_adjustment_get_upper (adj)) +
           (gtk_adjustment_get_page_size (adj) /
            gtk_adjustment_get_upper (adj)) / 2.0);

      gimp_preview_area_get_size (GIMP_PREVIEW_AREA (area),
                                  &area_width, &area_height);

      x += event->x - h * (gdouble) area_width;
      y += event->y - v * (gdouble) area_height;

      gtk_style_context_get_border (context,
                                    gtk_style_context_get_state (context),
                                    &border);

      gtk_window_move (GTK_WINDOW (priv->nav_popup),
                       x - (border.left + border.right),
                       y - (border.top + border.bottom));

      gtk_widget_show (priv->nav_popup);

      gtk_grab_add (area);

      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                           GDK_FLEUR);

      gdk_seat_grab (gdk_display_get_default_seat (gtk_widget_get_display (widget)),
                     gtk_widget_get_window (area),
                     GDK_SEAT_CAPABILITY_ALL_POINTING, TRUE,
                     cursor, (GdkEvent *) event,
                     NULL, NULL);

      g_object_unref (cursor);
    }

  return TRUE;
}

static gboolean
gimp_scrolled_preview_nav_popup_event (GtkWidget           *widget,
                                       GdkEvent            *event,
                                       GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  switch (event->type)
    {
    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *button_event = (GdkEventButton *) event;

        if (button_event->button == 1)
          {
            GdkSeat *seat = gdk_display_get_default_seat (gtk_widget_get_display (widget));

            gtk_grab_remove (widget);
            gdk_seat_ungrab (seat);

            gtk_widget_destroy (priv->nav_popup);
            priv->nav_popup = NULL;
          }
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;
        GtkAdjustment  *hadj;
        GtkAdjustment  *vadj;
        GtkAllocation   allocation;
        gint            cx, cy;
        gdouble         x, y;

        hadj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));
        vadj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));

        gtk_widget_get_allocation (widget, &allocation);

        gdk_window_get_device_position (gtk_widget_get_window (widget),
                                        gdk_event_get_device (event),
                                        &cx, &cy, NULL);

        x = cx * (gtk_adjustment_get_upper (hadj) -
                  gtk_adjustment_get_lower (hadj)) / allocation.width;
        y = cy * (gtk_adjustment_get_upper (vadj) -
                  gtk_adjustment_get_lower (vadj)) / allocation.height;

        x += (gtk_adjustment_get_lower (hadj) -
              gtk_adjustment_get_page_size (hadj) / 2);
        y += (gtk_adjustment_get_lower (vadj) -
              gtk_adjustment_get_page_size (vadj) / 2);

        gtk_adjustment_set_value (hadj,
                                  CLAMP (x,
                                         gtk_adjustment_get_lower (hadj),
                                         gtk_adjustment_get_upper (hadj) -
                                         gtk_adjustment_get_page_size (hadj)));
        gtk_adjustment_set_value (vadj,
                                  CLAMP (y,
                                         gtk_adjustment_get_lower (vadj),
                                         gtk_adjustment_get_upper (vadj) -
                                         gtk_adjustment_get_page_size (vadj)));

        gtk_widget_queue_draw (widget);

        gdk_event_request_motions (mevent);
      }
      break;

  default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_scrolled_preview_nav_popup_draw (GtkWidget           *widget,
                                      cairo_t             *cr,
                                      GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkAdjustment              *adj;
  GtkAllocation               allocation;
  gdouble                     x, y;
  gdouble                     w, h;

  adj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));

  gtk_widget_get_allocation (widget, &allocation);

  x = (gtk_adjustment_get_value (adj) /
       (gtk_adjustment_get_upper (adj) -
        gtk_adjustment_get_lower (adj)));
  w = (gtk_adjustment_get_page_size (adj) /
       (gtk_adjustment_get_upper (adj) -
        gtk_adjustment_get_lower (adj)));

  adj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));

  y = (gtk_adjustment_get_value (adj) /
       (gtk_adjustment_get_upper (adj) -
        gtk_adjustment_get_lower (adj)));
  h = (gtk_adjustment_get_page_size (adj) /
       (gtk_adjustment_get_upper (adj) -
        gtk_adjustment_get_lower (adj)));

  if (w >= 1.0 && h >= 1.0)
    return FALSE;

  x = floor (x * (gdouble) allocation.width);
  y = floor (y * (gdouble) allocation.height);
  w = MAX (1, ceil (w * (gdouble) allocation.width));
  h = MAX (1, ceil (h * (gdouble) allocation.height));

  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  cairo_rectangle (cr, x, y, w, h);

  cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_fill (cr);

  cairo_rectangle (cr, x, y, w, h);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_set_line_width (cr, 2);
  cairo_stroke (cr);

  return FALSE;
}

static void
gimp_scrolled_preview_set_cursor (GimpPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget                  *area;
  gint                        width, height;
  gint                        xmin, ymin;
  gint                        xmax, ymax;

  area = gimp_preview_get_area (preview);

  if (! gtk_widget_get_realized (area))
    return;

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

  if (xmax - xmin > width  ||
      ymax - ymin > height)
    {
      gdk_window_set_cursor (gtk_widget_get_window (area),
                             priv->cursor_move);
    }
  else
    {
      gdk_window_set_cursor (gtk_widget_get_window (area),
                             gimp_preview_get_default_cursor (preview));
    }
}

/**
 * gimp_scrolled_preview_set_position:
 * @preview: a #GimpScrolledPreview
 * @x:       horizontal scroll offset
 * @y:       vertical scroll offset
 *
 * Since: 2.4
 **/
void
gimp_scrolled_preview_set_position (GimpScrolledPreview *preview,
                                    gint                 x,
                                    gint                 y)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkAdjustment              *adj;
  gint                        xmin, ymin;

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));


  gimp_scrolled_preview_freeze (preview);

  gimp_scrolled_preview_hscr_update (preview);
  gimp_scrolled_preview_vscr_update (preview);

  gimp_preview_get_bounds (GIMP_PREVIEW (preview), &xmin, &ymin, NULL, NULL);

  adj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));
  gtk_adjustment_set_value (adj, x - xmin);

  adj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));
  gtk_adjustment_set_value (adj, y - ymin);

  gimp_scrolled_preview_thaw (preview);
}

/**
 * gimp_scrolled_preview_set_policy
 * @preview:           a #GimpScrolledPreview
 * @hscrollbar_policy: policy for horizontal scrollbar
 * @vscrollbar_policy: policy for vertical scrollbar
 *
 * Since: 2.4
 **/
void
gimp_scrolled_preview_set_policy (GimpScrolledPreview *preview,
                                  GtkPolicyType        hscrollbar_policy,
                                  GtkPolicyType        vscrollbar_policy)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

  priv->hscr_policy = hscrollbar_policy;
  priv->vscr_policy = vscrollbar_policy;

  gtk_widget_queue_resize (gimp_preview_get_area (GIMP_PREVIEW (preview)));
}

/**
 * gimp_scrolled_preview_get_adjustments:
 * @preview: a #GimpScrolledPreview
 * @hadj: (out) (transfer none): Horizontal adjustment
 * @vadj: (out) (transfer none): Vertical adjustment
 */
void
gimp_scrolled_preview_get_adjustments (GimpScrolledPreview  *preview,
                                       GtkAdjustment       **hadj,
                                       GtkAdjustment       **vadj)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

  if (hadj) *hadj = gtk_range_get_adjustment (GTK_RANGE (priv->hscr));
  if (vadj) *vadj = gtk_range_get_adjustment (GTK_RANGE (priv->vscr));
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
 * Since: 2.4
 **/
void
gimp_scrolled_preview_freeze (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));

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
 * Since: 2.4
 **/
void
gimp_scrolled_preview_thaw (GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GET_PRIVATE (preview);

  g_return_if_fail (GIMP_IS_SCROLLED_PREVIEW (preview));
  g_return_if_fail (priv->frozen > 0);

  priv->frozen--;
  if (! priv->frozen)
    {
      gimp_preview_draw (GIMP_PREVIEW (preview));
      gimp_preview_invalidate (GIMP_PREVIEW (preview));
    }
}

/**
 * gimp_scroll_adjustment_values:
 * @sevent: A #GdkEventScroll
 * @hadj: (nullable): Horizontal adjustment
 * @vadj: (nullable): Vertical adjustment
 * @hvalue: (out) (optional): Return location for horizontal value, or %NULL
 * @vvalue: (out) (optional): Return location for vertical value, or %NULL
 */
void
gimp_scroll_adjustment_values (GdkEventScroll *sevent,
                               GtkAdjustment  *hadj,
                               GtkAdjustment  *vadj,
                               gdouble        *hvalue,
                               gdouble        *vvalue)
{
  GtkAdjustment *adj_x;
  GtkAdjustment *adj_y;
  gdouble        scroll_unit_x;
  gdouble        scroll_unit_y;
  gdouble        value_x = 0.0;
  gdouble        value_y = 0.0;

  g_return_if_fail (sevent != NULL);
  g_return_if_fail (hadj == NULL || GTK_IS_ADJUSTMENT (hadj));
  g_return_if_fail (vadj == NULL || GTK_IS_ADJUSTMENT (vadj));

  if (sevent->state & GDK_SHIFT_MASK)
    {
      adj_x = vadj;
      adj_y = hadj;
    }
  else
    {
      adj_x = hadj;
      adj_y = vadj;
    }

  if (adj_x)
    {
      gdouble page_size = gtk_adjustment_get_page_size (adj_x);
      gdouble pow_unit  = pow (page_size, 2.0 / 3.0);

      scroll_unit_x = MIN (page_size / 2.0, pow_unit);
    }
  else
    scroll_unit_x = 1.0;

  if (adj_y)
    {
      gdouble page_size = gtk_adjustment_get_page_size (adj_y);
      gdouble pow_unit  = pow (page_size, 2.0 / 3.0);

      scroll_unit_y = MIN (page_size / 2.0, pow_unit);
    }
  else
    scroll_unit_y = 1.0;

  switch (sevent->direction)
    {
    case GDK_SCROLL_LEFT:
      value_x = -scroll_unit_x;
      break;

    case GDK_SCROLL_RIGHT:
      value_x = scroll_unit_x;
      break;

    case GDK_SCROLL_UP:
      value_y = -scroll_unit_y;
      break;

    case GDK_SCROLL_DOWN:
      value_y = scroll_unit_y;
      break;

    case GDK_SCROLL_SMOOTH:
      gdk_event_get_scroll_deltas ((GdkEvent *) sevent, &value_x, &value_y);
      value_x *= scroll_unit_x;
      value_y *= scroll_unit_y;
    }

  if (adj_x)
    value_x = CLAMP (value_x +
                     gtk_adjustment_get_value (adj_x),
                     gtk_adjustment_get_lower (adj_x),
                     gtk_adjustment_get_upper (adj_x) -
                     gtk_adjustment_get_page_size (adj_x));

  if (adj_y)
    value_y = CLAMP (value_y +
                     gtk_adjustment_get_value (adj_y),
                     gtk_adjustment_get_lower (adj_y),
                     gtk_adjustment_get_upper (adj_y) -
                     gtk_adjustment_get_page_size (adj_y));

  if (sevent->state & GDK_SHIFT_MASK)
    {
      if (hvalue) *hvalue = value_y;
      if (vvalue) *vvalue = value_x;
    }
  else
    {
      if (hvalue) *hvalue = value_x;
      if (vvalue) *vvalue = value_y;
    }
}
