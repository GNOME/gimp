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

#include "gimpwidgets.h"

#include "gimppreview.h"

#include "libgimp/libgimp-intl.h"


#define PREVIEW_TIMEOUT  200


enum
{
  INVALIDATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_UPDATE,
  PROP_SHOW_UPDATE_TOGGLE
};


static void     gimp_preview_class_init         (GimpPreviewClass *klass);
static void     gimp_preview_init               (GimpPreview      *preview);
static void     gimp_preview_dispose            (GObject          *object);
static void     gimp_preview_get_property       (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     gimp_preview_set_property       (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);

static void     gimp_preview_draw               (GimpPreview      *preview);
static void     gimp_preview_area_realize       (GtkWidget        *widget);
static void     gimp_preview_area_size_allocate (GtkWidget        *widget,
                                                 GtkAllocation    *allocation,
                                                 GimpPreview      *preview);
static void     gimp_preview_h_scroll           (GtkAdjustment    *hadj,
                                                 GimpPreview      *preview);
static void     gimp_preview_v_scroll           (GtkAdjustment    *vadj,
                                                 GimpPreview      *preview);
static gboolean gimp_preview_area_event         (GtkWidget        *area,
                                                 GdkEvent         *event,
                                                 GimpPreview      *preview);
static void     gimp_preview_toggle_callback    (GtkWidget        *toggle,
                                                 GimpPreview      *preview);

static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;


GType
gimp_preview_get_type (void)
{
  static GType preview_type = 0;

  if (!preview_type)
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

      preview_type = g_type_register_static (GTK_TYPE_TABLE,
                                             "GimpPreview",
                                             &preview_info,
                                             G_TYPE_FLAG_ABSTRACT);
    }

  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[INVALIDATED] =
    g_signal_new ("invalidated",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, invalidated),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->dispose      = gimp_preview_dispose;
  object_class->get_property = gimp_preview_get_property;
  object_class->set_property = gimp_preview_set_property;

  g_object_class_install_property (object_class,
                                   PROP_UPDATE,
                                   g_param_spec_boolean ("update",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_SHOW_UPDATE_TOGGLE,
                                   g_param_spec_boolean ("show_update_toggle",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));


  klass->draw = NULL;
}

static void
gimp_preview_init (GimpPreview *preview)
{
  GtkWidget *frame;

  gtk_table_resize (GTK_TABLE (preview), 3, 2);
  gtk_table_set_homogeneous (GTK_TABLE (preview), FALSE);

  preview->xoff       = 0;
  preview->yoff       = 0;
  preview->in_drag    = FALSE;
  preview->timeout_id = 0;

  preview->xmin   = preview->ymin = 0;
  preview->xmax   = preview->ymax = 1;

  preview->width  = preview->xmax - preview->xmin;
  preview->height = preview->ymax - preview->ymin;

  preview->hadj = gtk_adjustment_new (0, 0, preview->width - 1, 1.0,
                                      preview->width, preview->width);

  g_signal_connect (preview->hadj, "value_changed",
                    G_CALLBACK (gimp_preview_h_scroll),
                    preview);

  preview->hscr = gtk_hscrollbar_new (GTK_ADJUSTMENT (preview->hadj));
  gtk_range_set_update_policy (GTK_RANGE (preview->hscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (preview), preview->hscr, 0,1, 1,2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);

  preview->vadj = gtk_adjustment_new (0, 0, preview->height - 1, 1.0,
                                      preview->height, preview->height);

  g_signal_connect (preview->vadj, "value_changed",
                    G_CALLBACK (gimp_preview_v_scroll),
                    preview);

  preview->vscr = gtk_vscrollbar_new (GTK_ADJUSTMENT (preview->vadj));
  gtk_range_set_update_policy (GTK_RANGE (preview->vscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (preview), preview->vscr, 1,2, 0,1,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /* the area itself */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (preview), frame,
                    0,1, 0,1,
                    GTK_FILL, GTK_FILL, 0,0);
  gtk_widget_show (frame);

  preview->area = gimp_preview_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), preview->area);
  gtk_widget_show (preview->area);

  gtk_widget_set_events (preview->area,
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_BUTTON_RELEASE_MASK      |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_MOTION_MASK);

  g_signal_connect (preview->area, "event",
                    G_CALLBACK (gimp_preview_area_event),
                    preview);

  g_signal_connect (preview->area, "realize",
                    G_CALLBACK (gimp_preview_area_realize),
                    NULL);

  g_signal_connect (preview->area, "size_allocate",
                    G_CALLBACK (gimp_preview_area_size_allocate),
                    preview);

  /* a toggle button to (des)activate the instant preview */

  preview->toggle_update = gtk_check_button_new_with_mnemonic (_("_Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->toggle_update),
                                preview->update_preview);
  gtk_table_set_row_spacing (GTK_TABLE (preview), 1, 6);
  gtk_table_attach (GTK_TABLE (preview), preview->toggle_update,
                    0, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect_after (preview->toggle_update, "toggled",
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

    case PROP_SHOW_UPDATE_TOGGLE:
      g_value_set_boolean (value, GTK_WIDGET_VISIBLE (preview->toggle_update));
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
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->toggle_update),
                                    g_value_get_boolean (value));
      break;

    case PROP_SHOW_UPDATE_TOGGLE:
      if (g_value_get_boolean (value))
        gtk_widget_show (preview->toggle_update);
      else
        gtk_widget_hide (preview->toggle_update);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_preview_draw (GimpPreview *preview)
{
  GimpPreviewClass *class = GIMP_PREVIEW_GET_CLASS (preview);

  if (class->draw)
    class->draw (preview);
}

static void
gimp_preview_area_realize (GtkWidget *widget)
{
  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkCursor  *cursor  = gdk_cursor_new_for_display (display, GDK_FLEUR);

  gdk_window_set_cursor (widget->window, cursor);

  gdk_cursor_unref (cursor);
}

static void
gimp_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation,
                                 GimpPreview   *preview)
{
  gint  width  = preview->xmax - preview->xmin;
  gint  height = preview->ymax - preview->ymin;

  preview->width  = allocation->width;
  preview->height = allocation->height;

  if (width > preview->width)
    {
      gtk_widget_show (preview->hscr);

      gtk_range_set_increments (GTK_RANGE (preview->hscr),
                                1.0, MIN (preview->width, width));
      gtk_range_set_range (GTK_RANGE (preview->hscr),
                           0, width - preview->width - 1);
    }
  else
    {
      gtk_widget_hide (preview->hscr);
    }

  if (height > preview->height)
    {
      gtk_widget_show (preview->vscr);

      gtk_range_set_increments (GTK_RANGE (preview->vscr),
                                1.0, MIN (preview->height, height));
      gtk_range_set_range (GTK_RANGE (preview->vscr),
                           0, height - preview->height - 1);
    }
  else
    {
      gtk_widget_hide (preview->vscr);
    }

  gimp_preview_draw (preview);
  gimp_preview_invalidate (preview);
}

static void
gimp_preview_h_scroll (GtkAdjustment *hadj,
                       GimpPreview   *preview)
{
  preview->xoff = hadj->value;

  if (! preview->in_drag)
    gimp_preview_draw (preview);

  gimp_preview_invalidate (preview);
}

static void
gimp_preview_v_scroll (GtkAdjustment *vadj,
                       GimpPreview   *preview)
{
  preview->yoff = vadj->value;

  if (! preview->in_drag)
    gimp_preview_draw (preview);

  gimp_preview_invalidate (preview);
}

static gboolean
gimp_preview_area_event (GtkWidget   *area,
                         GdkEvent    *event,
                         GimpPreview *preview)
{
  gint  x, y;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      gtk_widget_get_pointer (area, &x, &y);

      preview->in_drag = TRUE;
      preview->drag_x = x;
      preview->drag_y = y;
      preview->drag_xoff = preview->xoff;
      preview->drag_yoff = preview->yoff;

      gtk_grab_add (area);
      break;

    case GDK_BUTTON_RELEASE:
      if (preview->in_drag)
        {
          gtk_grab_remove (area);
          preview->in_drag = FALSE;
        }
      break;

    case GDK_MOTION_NOTIFY:
      if (preview->in_drag)
        {
          gint dx, dy;
          gint xoff, yoff;

          gtk_widget_get_pointer (area, &x, &y);

          dx = x - preview->drag_x;
          dy = y - preview->drag_y;

          xoff = CLAMP (preview->drag_xoff - dx,
                        0, preview->xmax - preview->xmin - preview->width);
          yoff = CLAMP (preview->drag_yoff - dy,
                        0, preview->ymax - preview->ymin - preview->height);

          gtk_adjustment_set_value (GTK_ADJUSTMENT (preview->hadj), xoff);
          gtk_adjustment_set_value (GTK_ADJUSTMENT (preview->vadj), yoff);

          gimp_preview_draw (preview);
          gimp_preview_invalidate (preview);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_preview_toggle_callback (GtkWidget   *toggle,
                              GimpPreview *preview)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      preview->update_preview = TRUE;
      gimp_preview_invalidate (preview);
    }
  else
    {
      preview->update_preview = FALSE;
    }

  g_object_notify (G_OBJECT (preview), "update");
}

static gboolean
gimp_preview_invalidate_now (GimpPreview *preview)
{
  preview->timeout_id = 0;

  g_signal_emit (preview, preview_signals[INVALIDATED], 0);

  return FALSE;
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
 * gimp_preview_get_posistion:
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

/**
 * gimp_preview_show_update_toggle:
 * @preview:     a #GimpPreview widget
 * @show_update: whether to show the "Update Preview" toggle button
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_show_update_toggle (GimpPreview *preview,
                                 gboolean     show_update)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  g_object_set (preview,
                "show_update_toggle", show_update,
                NULL);
}

/*
 * gimp_preview_invalidate:
 * @preview: a #GimpPreview widget
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
