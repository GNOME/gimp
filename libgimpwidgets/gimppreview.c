/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppreview.c
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

#include "gimppreview.h"

#include "libgimp/libgimp-intl.h"


#define DEFAULT_SIZE     150
#define POPUP_SIZE       100
#define PEN_WIDTH          3
#define PREVIEW_TIMEOUT  200


enum
{
  INVALIDATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_UPDATE
};


static void      gimp_preview_class_init          (GimpPreviewClass *klass);
static void      gimp_preview_init                (GimpPreview      *preview);
static void      gimp_preview_dispose             (GObject          *object);
static void      gimp_preview_get_property        (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);
static void      gimp_preview_set_property        (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);

static void      gimp_preview_direction_changed   (GtkWidget        *widget,
                                                   GtkTextDirection  prev_dir);
static gboolean  gimp_preview_popup_menu          (GtkWidget        *widget);

static void      gimp_preview_area_realize        (GtkWidget        *widget,
                                                   GimpPreview      *preview);
static void      gimp_preview_area_unrealize      (GtkWidget        *widget,
                                                   GimpPreview      *preview);
static void      gimp_preview_area_size_allocate  (GtkWidget        *widget,
                                                   GtkAllocation    *allocation,
                                                   GimpPreview      *preview);
static gboolean  gimp_preview_area_event          (GtkWidget        *area,
                                                   GdkEvent         *event,
                                                   GimpPreview      *preview);

static void      gimp_preview_h_scroll            (GtkAdjustment    *hadj,
                                                   GimpPreview      *preview);
static void      gimp_preview_v_scroll            (GtkAdjustment    *vadj,
                                                   GimpPreview      *preview);

static gboolean  gimp_preview_nav_button_press    (GtkWidget        *widget,
                                                   GdkEventButton   *event,
                                                   GimpPreview      *preview);

static void      gimp_preview_nav_popup_realize   (GtkWidget        *widget,
                                                   GimpPreview      *preview);
static void      gimp_preview_nav_popup_unrealize (GtkWidget        *widget,
                                                   GimpPreview      *preview);
static gboolean  gimp_preview_nav_popup_event     (GtkWidget        *widget,
                                                   GdkEvent         *event,
                                                   GimpPreview      *preview);
static gboolean  gimp_preview_nav_popup_expose    (GtkWidget        *widget,
                                                   GdkEventExpose   *event,
                                                   GimpPreview      *preview);

static void      gimp_preview_toggle_callback     (GtkWidget        *toggle,
                                                   GimpPreview      *preview);

static void      gimp_preview_notify_checks       (GimpPreview      *preview);

static gboolean  gimp_preview_invalidate_now      (GimpPreview      *preview);


static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;


GType
gimp_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpPreviewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPreview),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_preview_init,
      };

      preview_type = g_type_register_static (GTK_TYPE_VBOX,
                                             "GimpPreview",
                                             &preview_info,
                                             G_TYPE_FLAG_ABSTRACT);
    }

  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[INVALIDATED] =
    g_signal_new ("invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPreviewClass, invalidated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose           = gimp_preview_dispose;
  object_class->get_property      = gimp_preview_get_property;
  object_class->set_property      = gimp_preview_set_property;

  widget_class->direction_changed = gimp_preview_direction_changed;
  widget_class->popup_menu        = gimp_preview_popup_menu;

  klass->draw                     = NULL;
  klass->draw_thumb               = NULL;

  g_object_class_install_property (object_class,
                                   PROP_UPDATE,
                                   g_param_spec_boolean ("update",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("size",
                                                             NULL, NULL,
                                                             1, 1024,
                                                             DEFAULT_SIZE,
                                                             G_PARAM_READABLE));
}

static void
gimp_preview_init (GimpPreview *preview)
{
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *image;
  GtkObject *adj;
  gdouble    xalign = 0.0;

  gtk_box_set_homogeneous (GTK_BOX (preview), FALSE);
  gtk_box_set_spacing (GTK_BOX (preview), 6);

  if (gtk_widget_get_direction (GTK_WIDGET (preview)) == GTK_TEXT_DIR_RTL)
    xalign = 1.0;

  preview->frame = gtk_aspect_frame_new (NULL, xalign, 0.0, 1.0, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (preview->frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (preview), preview->frame, TRUE, TRUE, 0);
  gtk_widget_show (preview->frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (preview->frame), table);
  gtk_widget_show (table);

  preview->xoff       = 0;
  preview->yoff       = 0;
  preview->in_drag    = FALSE;
  preview->timeout_id = 0;

  preview->xmin       = preview->ymin = 0;
  preview->xmax       = preview->ymax = 1;
  preview->width      = preview->xmax - preview->xmin;
  preview->height     = preview->ymax - preview->ymin;

  preview->nav_popup  = NULL;

  /*  scrollbars  */
  adj = gtk_adjustment_new (0, 0, preview->width - 1, 1.0,
                            preview->width, preview->width);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_preview_h_scroll),
                    preview);

  preview->hscr = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->hscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (table), preview->hscr, 0,1, 1,2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);

  adj = gtk_adjustment_new (0, 0, preview->height - 1, 1.0,
                            preview->height, preview->height);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_preview_v_scroll),
                    preview);

  preview->vscr = gtk_vscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->vscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (table), preview->vscr, 1,2, 0,1,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /*  preview area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 0,1, 0,1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  preview->area = gimp_preview_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), preview->area);
  gtk_widget_show (preview->area);

  g_signal_connect_swapped (preview->area, "notify::check-size",
                            G_CALLBACK (gimp_preview_notify_checks),
                            preview);
  g_signal_connect_swapped (preview->area, "notify::check-type",
                            G_CALLBACK (gimp_preview_notify_checks),
                            preview);

  gtk_widget_add_events (preview->area,
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_BUTTON_RELEASE_MASK      |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_MOTION_MASK);

  g_signal_connect (preview->area, "event",
                    G_CALLBACK (gimp_preview_area_event),
                    preview);

  g_signal_connect (preview->area, "realize",
                    G_CALLBACK (gimp_preview_area_realize),
                    preview);
  g_signal_connect (preview->area, "unrealize",
                    G_CALLBACK (gimp_preview_area_unrealize),
                    preview);

  g_signal_connect (preview->area, "size_allocate",
                    G_CALLBACK (gimp_preview_area_size_allocate),
                    preview);

  /*  navigation icon  */
  preview->nav_icon = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (table), preview->nav_icon, 1,2, 1,2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);

  image = gtk_image_new_from_stock (GIMP_STOCK_NAVIGATION, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (preview->nav_icon), image);
  gtk_widget_show (image);

  g_signal_connect (preview->nav_icon, "button_press_event",
                    G_CALLBACK (gimp_preview_nav_button_press),
                    preview);

  /*  toggle button to (des)activate the instant preview  */
  preview->toggle = gtk_check_button_new_with_mnemonic (_("_Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->toggle),
                                preview->update_preview);
  gtk_box_pack_start (GTK_BOX (preview), preview->toggle, FALSE, FALSE, 0);
  gtk_widget_show (preview->toggle);

  g_signal_connect (preview->toggle, "toggled",
                    G_CALLBACK (gimp_preview_toggle_callback),
                    preview);
}

static void
gimp_preview_dispose (GObject *object)
{
  GimpPreview *preview = GIMP_PREVIEW (object);

  if (preview->timeout_id)
    {
      g_source_remove (preview->timeout_id);
      preview->timeout_id = 0;
    }

  if (preview->nav_popup)
    {
      gtk_widget_destroy (preview->nav_popup);
      preview->nav_popup = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_preview_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpPreview *preview = GIMP_PREVIEW (object);

  switch (property_id)
    {
    case PROP_UPDATE:
      g_value_set_boolean (value, preview->update_preview);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_preview_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpPreview *preview = GIMP_PREVIEW (object);

  switch (property_id)
    {
    case PROP_UPDATE:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->toggle),
                                    g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_preview_direction_changed (GtkWidget        *widget,
                                GtkTextDirection  prev_dir)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);
  gdouble      xalign  = 0.0;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    xalign = 1.0;

  gtk_aspect_frame_set (GTK_ASPECT_FRAME (preview->frame),
                        xalign, 0.0, 1.0, TRUE);
}

static gboolean
gimp_preview_popup_menu (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);

  gimp_preview_area_menu_popup (GIMP_PREVIEW_AREA (preview->area), NULL);

  return TRUE;
}

static void
gimp_preview_area_realize (GtkWidget   *widget,
                           GimpPreview *preview)
{
  GdkDisplay *display = gtk_widget_get_display (widget);

  g_return_if_fail (preview->cursor_move == NULL);
  g_return_if_fail (preview->cursor_busy == NULL);

  preview->cursor_move = gdk_cursor_new_for_display (display, GDK_FLEUR);
  preview->cursor_busy = gdk_cursor_new_for_display (display, GDK_WATCH);

  if (preview->xmax - preview->xmin > preview->width  ||
      preview->ymax - preview->ymin > preview->height)
    {
      gdk_window_set_cursor (widget->window, preview->cursor_move);
    }
  else
    {
      gdk_window_set_cursor (widget->window, NULL);
    }
}

static void
gimp_preview_area_unrealize (GtkWidget   *widget,
                             GimpPreview *preview)
{
  if (preview->cursor_move)
    {
      gdk_cursor_unref (preview->cursor_move);
      preview->cursor_move = NULL;
    }
  if (preview->cursor_busy)
    {
      gdk_cursor_unref (preview->cursor_busy);
      preview->cursor_busy = NULL;
    }
}

static void
gimp_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation,
                                 GimpPreview   *preview)
{
  GdkCursor *cursor = NULL;
  gint       width  = preview->xmax - preview->xmin;
  gint       height = preview->ymax - preview->ymin;

  preview->width  = MIN (width,  allocation->width);
  preview->height = MIN (height, allocation->height);

  if (width > preview->width)
    {
      GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));

      adj->lower          = 0;
      adj->upper          = width;
      adj->page_size      = preview->width;
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

  if (height > preview->height)
    {
      GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

      adj->lower          = 0;
      adj->upper          = height;
      adj->page_size      = preview->height;
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

  gimp_preview_draw (preview);
  gimp_preview_invalidate (preview);
}


static gboolean
gimp_preview_area_event (GtkWidget   *area,
                         GdkEvent    *event,
                         GimpPreview *preview)
{
  GdkEventButton *button_event = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (button_event->button)
        {
        case 1:
          gtk_widget_get_pointer (area, &preview->drag_x, &preview->drag_y);

          preview->drag_xoff = preview->xoff;
          preview->drag_yoff = preview->yoff;

          preview->in_drag = TRUE;
          gtk_grab_add (area);
          break;

        case 2:
          break;

        case 3:
          gimp_preview_area_menu_popup (GIMP_PREVIEW_AREA (area), button_event);
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
                        0, preview->xmax - preview->xmin - preview->width);
          yoff = CLAMP (preview->drag_yoff - dy,
                        0, preview->ymax - preview->ymin - preview->height);

          if (preview->xoff != xoff || preview->yoff != yoff)
            {
              gtk_range_set_value (GTK_RANGE (preview->hscr), xoff);
              gtk_range_set_value (GTK_RANGE (preview->vscr), yoff);

              gimp_preview_draw (preview);
              gimp_preview_invalidate (preview);
            }
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_preview_h_scroll (GtkAdjustment *hadj,
                       GimpPreview   *preview)
{
  preview->xoff = hadj->value;

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (preview->area),
                                 preview->xoff, preview->yoff);

  if (! preview->in_drag)
    {
      gimp_preview_draw (preview);
      gimp_preview_invalidate (preview);
    }
}

static void
gimp_preview_v_scroll (GtkAdjustment *vadj,
                       GimpPreview   *preview)
{
  preview->yoff = vadj->value;

  gimp_preview_area_set_offsets (GIMP_PREVIEW_AREA (preview->area),
                                 preview->xoff, preview->yoff);

  if (! preview->in_drag)
    {
      gimp_preview_draw (preview);
      gimp_preview_invalidate (preview);
    }
}

static gboolean
gimp_preview_nav_button_press (GtkWidget      *widget,
                               GdkEventButton *event,
                               GimpPreview    *preview)
{
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
                        G_CALLBACK (gimp_preview_nav_popup_realize),
                        preview);
      g_signal_connect (area, "unrealize",
                        G_CALLBACK (gimp_preview_nav_popup_unrealize),
                        preview);
      g_signal_connect (area, "event",
                        G_CALLBACK (gimp_preview_nav_popup_event),
                        preview);
      g_signal_connect_after (area, "expose_event",
                              G_CALLBACK (gimp_preview_nav_popup_expose),
                              preview);

      GIMP_PREVIEW_GET_CLASS (preview)->draw_thumb (preview,
                                                    GIMP_PREVIEW_AREA (area),
                                                    POPUP_SIZE, POPUP_SIZE);
      gtk_widget_realize (area);
      gtk_widget_show (area);

      gdk_window_get_origin (widget->window, &x, &y);

      h = ((gdouble) (preview->xoff + preview->width  / 2) /
           (gdouble) (preview->xmax - preview->xmin));
      v = ((gdouble) (preview->yoff + preview->height / 2) /
           (gdouble) (preview->ymax - preview->ymin));

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
gimp_preview_nav_popup_realize (GtkWidget   *widget,
                                GimpPreview *preview)
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
gimp_preview_nav_popup_unrealize (GtkWidget   *widget,
                                  GimpPreview *preview)
{
  if (preview->nav_gc)
    {
      g_object_unref (preview->nav_gc);
      preview->nav_gc = NULL;
    }
}

static gboolean
gimp_preview_nav_popup_event (GtkWidget   *widget,
                              GdkEvent    *event,
                              GimpPreview *preview)
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
gimp_preview_nav_popup_expose (GtkWidget      *widget,
                               GdkEventExpose *event,
                               GimpPreview    *preview)
{
  gdouble  x, y;
  gdouble  w, h;

  x = (gdouble) preview->xoff / (gdouble) (preview->xmax - preview->xmin);
  y = (gdouble) preview->yoff / (gdouble) (preview->ymax - preview->ymin);

  w = (gdouble) preview->width  / (gdouble) (preview->xmax - preview->xmin);
  h = (gdouble) preview->height / (gdouble) (preview->ymax - preview->ymin);

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
gimp_preview_toggle_callback (GtkWidget   *toggle,
                              GimpPreview *preview)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      preview->update_preview = TRUE;

      if (preview->timeout_id)
        g_source_remove (preview->timeout_id);

      gimp_preview_invalidate_now (preview);
    }
  else
    {
      preview->update_preview = FALSE;

      gimp_preview_draw (preview);
    }

  g_object_notify (G_OBJECT (preview), "update");
}

static void
gimp_preview_notify_checks (GimpPreview *preview)
{
  gimp_preview_draw (preview);
  gimp_preview_invalidate (preview);
}

static gboolean
gimp_preview_invalidate_now (GimpPreview *preview)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (preview));

  preview->timeout_id = 0;

  if (toplevel && GTK_WIDGET_REALIZED (toplevel))
    {
      gdk_window_set_cursor (toplevel->window, preview->cursor_busy);
      gdk_window_set_cursor (preview->area->window, preview->cursor_busy);

      gdk_flush ();

      g_signal_emit (preview, preview_signals[INVALIDATED], 0);

      if (preview->xmax - preview->xmin > preview->width  ||
          preview->ymax - preview->ymin > preview->height)
        {
          gdk_window_set_cursor (preview->area->window, preview->cursor_move);
        }
      else
        {
          gdk_window_set_cursor (preview->area->window, NULL);
        }

      gdk_window_set_cursor (toplevel->window, NULL);
    }
  else
    {
      g_signal_emit (preview, preview_signals[INVALIDATED], 0);
    }

  return FALSE;
}

/**
 * gimp_preview_set_update:
 * @preview: a #GimpPreview widget
 * @update: %TRUE if the preview should invalidate itself when being
 *          scrolled or when gimp_preview_invalidate() is being called
 *
 * Sets the state of the "Preview" check button.
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_set_update (GimpPreview *preview,
                         gboolean     update)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  g_object_set (preview,
                "update", update,
                NULL);
}

/**
 * gimp_preview_get_update:
 * @preview: a #GimpPreview widget
 *
 * Return value: the state of the "Preview" check button.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_preview_get_update (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), FALSE);

  return preview->update_preview;
}

/**
 * gimp_preview_set_bounds:
 * @preview: a #GimpPreview widget
 * @xmin:
 * @ymin:
 * @xmax:
 * @ymax:
 *
 * Sets the lower and upper limits for the previewed area. The
 * difference between the upper and lower value is used to set the
 * maximum size of the #GimpPreviewArea used in the @preview.
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_set_bounds (GimpPreview *preview,
                         gint         xmin,
                         gint         ymin,
                         gint         xmax,
                         gint         ymax)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (xmax > xmin);
  g_return_if_fail (ymax > ymin);

  preview->xmin = xmin;
  preview->ymin = ymin;
  preview->xmax = xmax;
  preview->ymax = ymax;

  gimp_preview_area_set_max_size (GIMP_PREVIEW_AREA (preview->area),
                                  xmax - xmin,
                                  ymax - ymin);
}

/**
 * gimp_preview_get_size:
 * @preview: a #GimpPreview widget
 * @width:   return location for the preview area width
 * @height:  return location for the preview area height
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_get_size (GimpPreview *preview,
                       gint        *width,
                       gint        *height)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (width)
    *width = preview->width;

  if (height)
    *height = preview->height;
}

/**
 * gimp_preview_get_position:
 * @preview: a #GimpPreview widget
 * @x:       return location for the horizontal offset
 * @y:       return location for the vertical offset
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_get_position (GimpPreview  *preview,
                           gint         *x,
                           gint         *y)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (x)
    *x = preview->xoff + preview->xmin;

  if (y)
    *y = preview->yoff + preview->ymin;
}

/*
 * gimp_preview_draw:
 * @preview: a #GimpPreview widget
 *
 * Calls the GimpPreview::draw method. GimpPreview itself doesn't
 * implement a default draw method so the behaviour is determined by
 * the derived class implementing this method.
 *
 * #GimpDrawablePreview implements gimp_preview_draw() by drawing the
 * original, unmodified drawable to the @preview.
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_draw (GimpPreview *preview)
{
  GimpPreviewClass *class = GIMP_PREVIEW_GET_CLASS (preview);

  if (class->draw)
    class->draw (preview);
}

/*
 * gimp_preview_invalidate:
 * @preview: a #GimpPreview widget
 *
 * This function starts or renews a short low-priority timeout. When
 * the timeout expires, the GimpPreview::invalidated signal is emitted
 * which will usually cause the @preview to be updated.
 *
 * This function does nothing unless the "Preview" button is checked.
 *
 * During the emission of the signal a busy cursor is set on the
 * toplevel window containing the @preview and on the preview area
 * itself.
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_invalidate (GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (preview->update_preview)
    {
      if (preview->timeout_id)
        g_source_remove (preview->timeout_id);

      preview->timeout_id =
        g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, PREVIEW_TIMEOUT,
                            (GSourceFunc) gimp_preview_invalidate_now,
                            preview, NULL);
    }
}
