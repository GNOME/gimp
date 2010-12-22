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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpicons.h"
#include "gimppreviewarea.h"
#include "gimpscrolledpreview.h"
#include "gimp3migration.h"

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

static gboolean  gimp_scrolled_preview_nav_popup_event     (GtkWidget                *widget,
                                                            GdkEvent                 *event,
                                                            GimpScrolledPreview      *preview);
static gboolean  gimp_scrolled_preview_nav_popup_draw      (GtkWidget                *widget,
                                                            cairo_t                  *cr,
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
  GtkAdjustment              *adj;

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
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, GIMP_PREVIEW (preview)->width - 1, 1.0,
                                            GIMP_PREVIEW (preview)->width,
                                            GIMP_PREVIEW (preview)->width));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_scrolled_preview_h_scroll),
                    preview);

  preview->hscr = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, adj);
  gtk_table_attach (GTK_TABLE (GIMP_PREVIEW (preview)->table),
                    preview->hscr, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, GIMP_PREVIEW (preview)->height - 1, 1.0,
                                            GIMP_PREVIEW (preview)->height,
                                            GIMP_PREVIEW (preview)->height));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_scrolled_preview_v_scroll),
                    preview);

  preview->vscr = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adj);
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

  image = gtk_image_new_from_icon_name (GIMP_ICON_DIALOG_NAVIGATION,
                                        GTK_ICON_SIZE_MENU);
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

  preview->cursor_move = gdk_cursor_new_for_display (display, GDK_HAND1);
}

static void
gimp_scrolled_preview_area_unrealize (GtkWidget           *widget,
                                      GimpScrolledPreview *preview)
{
  if (preview->cursor_move)
    {
      g_object_unref (preview->cursor_move);
      preview->cursor_move = NULL;
    }
}

static void
gimp_scrolled_preview_hscr_update (GimpScrolledPreview *preview)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  gint           width;

  width = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;

  gtk_adjustment_configure (adj,
                            gtk_adjustment_get_value (adj),
                            0, width,
                            1.0,
                            MAX (GIMP_PREVIEW (preview)->width / 2.0, 1.0),
                            GIMP_PREVIEW (preview)->width);
}

static void
gimp_scrolled_preview_vscr_update (GimpScrolledPreview *preview)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  gint           height;

  height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  gtk_adjustment_configure (adj,
                            gtk_adjustment_get_value (adj),
                            0, height,
                            1.0,
                            MAX (GIMP_PREVIEW (preview)->height / 2.0, 1.0),
                            GIMP_PREVIEW (preview)->height);
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
      gtk_widget_set_visible (preview->hscr,
                              width > GIMP_PREVIEW (preview)->width);
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
      gtk_widget_set_visible (preview->vscr,
                              height > GIMP_PREVIEW (preview)->height);
      break;

    case GTK_POLICY_ALWAYS:
      gtk_widget_show (preview->vscr);
      break;

    case GTK_POLICY_NEVER:
      gtk_widget_hide (preview->vscr);
      break;
    }

  gtk_widget_set_visible (preview->nav_icon,
                          gtk_widget_get_visible (preview->vscr) &&
                          gtk_widget_get_visible (preview->hscr) &&
                          GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb);

  gimp_scrolled_preview_thaw (preview);
}

static gboolean
gimp_scrolled_preview_area_event (GtkWidget           *area,
                                  GdkEvent            *event,
                                  GimpScrolledPreview *preview)
{
  GimpScrolledPreviewPrivate *priv = GIMP_SCROLLED_PREVIEW_GET_PRIVATE (preview);
  GdkEventButton             *button_event = (GdkEventButton *) event;
  GdkCursor                  *cursor;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (button_event->button)
        {
        case 1:
        case 2:
          cursor = gdk_cursor_new_for_display (gtk_widget_get_display (area),
                                               GDK_FLEUR);

          if (gdk_pointer_grab (gtk_widget_get_window (area), TRUE,
                                GDK_BUTTON_RELEASE_MASK |
                                GDK_BUTTON_MOTION_MASK  |
                                GDK_POINTER_MOTION_HINT_MASK,
                                NULL, cursor,
                                gdk_event_get_time (event)) == GDK_GRAB_SUCCESS)
            {
              gtk_widget_get_pointer (area, &priv->drag_x, &priv->drag_y);

              priv->drag_xoff = GIMP_PREVIEW (preview)->xoff;
              priv->drag_yoff = GIMP_PREVIEW (preview)->yoff;
              priv->in_drag   = TRUE;
              gtk_grab_add (area);
            }

          g_object_unref (cursor);

          break;

        case 3:
          return TRUE;
        }
      break;

    case GDK_BUTTON_RELEASE:
      if (priv->in_drag &&
          (button_event->button == 1 || button_event->button == 2))
        {
          gdk_display_pointer_ungrab (gtk_widget_get_display (area),
                                      gdk_event_get_time (event));

          gtk_grab_remove (area);
          priv->in_drag = FALSE;
        }
      break;

    case GDK_MOTION_NOTIFY:
      if (priv->in_drag)
        {
          GdkEventMotion *mevent = (GdkEventMotion *) event;
          GtkAdjustment  *hadj;
          GtkAdjustment  *vadj;
          gint            x, y;

          hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
          vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

          gtk_widget_get_pointer (area, &x, &y);

          x = priv->drag_xoff - (x - priv->drag_x);
          y = priv->drag_yoff - (y - priv->drag_y);

          x = CLAMP (x,
                     gtk_adjustment_get_lower (hadj),
                     gtk_adjustment_get_upper (hadj) -
                     gtk_adjustment_get_page_size (hadj));
          y = CLAMP (y,
                     gtk_adjustment_get_lower (vadj),
                     gtk_adjustment_get_upper (vadj) -
                     gtk_adjustment_get_page_size (vadj));

          if (GIMP_PREVIEW (preview)->xoff != x ||
              GIMP_PREVIEW (preview)->yoff != y)
            {
              gtk_adjustment_set_value (hadj, x);
              gtk_adjustment_set_value (vadj, y);

              gimp_preview_draw (GIMP_PREVIEW (preview));
              gimp_preview_invalidate (GIMP_PREVIEW (preview));
            }

          gdk_event_request_motions (mevent);
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
            value -= gtk_adjustment_get_page_increment (adj) / 2;
            break;

          case GDK_SCROLL_DOWN:
          case GDK_SCROLL_RIGHT:
            value += gtk_adjustment_get_page_increment (adj) / 2;
            break;
          }

        gtk_adjustment_set_value (adj,
                                  CLAMP (value,
                                         gtk_adjustment_get_lower (adj),
                                         gtk_adjustment_get_upper (adj) -
                                         gtk_adjustment_get_page_size (adj)));
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

  preview->xoff = gtk_adjustment_get_value (hadj);

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

  preview->yoff = gtk_adjustment_get_value (vadj);

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
      GtkStyleContext *context = gtk_widget_get_style_context (widget);
      GtkWidget       *outer;
      GtkWidget       *inner;
      GtkWidget       *area;
      GdkCursor       *cursor;
      GtkBorder        border;
      gint             x, y;
      gdouble          h, v;

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

      adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
      h = ((gtk_adjustment_get_value (adj) /
            gtk_adjustment_get_upper (adj)) +
           (gtk_adjustment_get_page_size (adj) /
            gtk_adjustment_get_upper (adj)) / 2.0);

      adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
      v = ((gtk_adjustment_get_value (adj) /
            gtk_adjustment_get_upper (adj)) +
           (gtk_adjustment_get_page_size (adj) /
            gtk_adjustment_get_upper (adj)) / 2.0);

      x += event->x - h * (gdouble) GIMP_PREVIEW_AREA (area)->width;
      y += event->y - v * (gdouble) GIMP_PREVIEW_AREA (area)->height;

      gtk_style_context_get_border (context, 0, &border);

      gtk_window_move (GTK_WINDOW (preview->nav_popup),
                       x - (border.left + border.right),
                       y - (border.top + border.bottom));

      gtk_widget_show (preview->nav_popup);

      gtk_grab_add (area);

      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                           GDK_FLEUR);

      gdk_pointer_grab (gtk_widget_get_window (area), TRUE,
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_BUTTON_MOTION_MASK  |
                        GDK_POINTER_MOTION_HINT_MASK,
                        gtk_widget_get_window (area), cursor,
                        event->time);

      g_object_unref (cursor);
    }

  return TRUE;
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
        GdkEventMotion *mevent = (GdkEventMotion *) event;
        GtkAdjustment  *hadj;
        GtkAdjustment  *vadj;
        GtkAllocation   allocation;
        gint            cx, cy;
        gdouble         x, y;

        hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
        vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

        gtk_widget_get_allocation (widget, &allocation);

        gtk_widget_get_pointer (widget, &cx, &cy);

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
        gdk_window_process_updates (gtk_widget_get_window (widget), FALSE);

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
  GtkAdjustment *adj;
  GtkAllocation  allocation;
  gdouble        x, y;
  gdouble        w, h;

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));

  gtk_widget_get_allocation (widget, &allocation);

  x = (gtk_adjustment_get_value (adj) /
       (gtk_adjustment_get_upper (adj) -
        gtk_adjustment_get_lower (adj)));
  w = (gtk_adjustment_get_page_size (adj) /
       (gtk_adjustment_get_upper (adj) -
        gtk_adjustment_get_lower (adj)));

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

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
  if (! gtk_widget_get_realized (preview->area))
    return;

  if (preview->xmax - preview->xmin > preview->width  ||
      preview->ymax - preview->ymin > preview->height)
    {
      gdk_window_set_cursor (gtk_widget_get_window (preview->area),
                             GIMP_SCROLLED_PREVIEW (preview)->cursor_move);
    }
  else
    {
      gdk_window_set_cursor (gtk_widget_get_window (preview->area),
                             preview->default_cursor);
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
 * Since: 2.4
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
 * Since: 2.4
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
 * Since: 2.4
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
