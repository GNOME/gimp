/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpink-blob.h"
#include "paint/gimpinkoptions.h"

#include "widgets/gimppropwidgets.h"

#include "gimpinkoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


static GtkWidget * brush_widget_new         (GimpInkOptions  *options);
static GtkWidget * blob_button_new          (GimpInkBlobType  blob_type);


GtkWidget *
gimp_ink_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *brush_vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *brush;

  config = G_OBJECT (tool_options);

  vbox = gimp_paint_options_gui (tool_options);

  /* adjust sliders */
  frame = gimp_frame_new (_("Adjustment"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*  size slider  */
  gimp_prop_scale_entry_new (config, "size",
                             GTK_TABLE (table), 0, 0,
                             _("Size:"),
                             1.0, 2.0, 1,
                             FALSE, 0.0, 0.0);

  /* angle adjust slider */
  gimp_prop_scale_entry_new (config, "tilt-angle",
                             GTK_TABLE (table), 0, 1,
                             _("Angle:"),
                             1.0, 10.0, 1,
                             FALSE, 0.0, 0.0);

  /* sens sliders */
  frame = gimp_frame_new (_("Sensitivity"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /* size sens slider */
  gimp_prop_scale_entry_new (config, "size-sensitivity",
                             GTK_TABLE (table), 0, 0,
                             _("Size:"),
                             0.01, 0.1, 1,
                             FALSE, 0.0, 0.0);

  /* tilt sens slider */
  gimp_prop_scale_entry_new (config, "tilt-sensitivity",
                             GTK_TABLE (table), 0, 1,
                             _("Tilt:"),
                             0.01, 0.1, 1,
                             FALSE, 0.0, 0.0);

  /* velocity sens slider */
  gimp_prop_scale_entry_new (config, "vel-sensitivity",
                             GTK_TABLE (table), 0, 2,
                             _("Speed:"),
                             0.01, 0.1, 1,
                             FALSE, 0.0, 0.0);

  /*  bottom hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Brush type radiobuttons */
  frame = gimp_prop_enum_radio_frame_new (config, "blob-type",
                                          _("Type"), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  {
    GList           *children;
    GList           *list;
    GimpInkBlobType  blob_type;

    children =
      gtk_container_get_children (GTK_CONTAINER (GTK_BIN (frame)->child));

    for (list = children, blob_type = GIMP_INK_BLOB_TYPE_ELLIPSE;
         list;
         list = g_list_next (list), blob_type++)
      {
        GtkWidget *radio;
        GtkWidget *blob;

        radio = GTK_WIDGET (list->data);

        gtk_container_remove (GTK_CONTAINER (radio), GTK_BIN (radio)->child);

        blob = blob_button_new (blob_type);
        gtk_container_add (GTK_CONTAINER (radio), blob);
        gtk_widget_show (blob);
      }

    g_list_free (children);
  }

  /* Brush shape widget */
  frame = gimp_frame_new (_("Shape"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  brush_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (brush_vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), brush_vbox);
  gtk_widget_show (brush_vbox);

  frame = gtk_aspect_frame_new (NULL, 0.0, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (brush_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  brush = brush_widget_new (GIMP_INK_OPTIONS (tool_options));
  gtk_container_add (GTK_CONTAINER (frame), brush);
  gtk_widget_show (brush);

  return vbox;
}


/*  BrushWidget functions  */

typedef struct _BrushWidget BrushWidget;

struct _BrushWidget
{
  GtkWidget      *widget;
  gboolean        active;

  /* EEK */
  GimpInkOptions *ink_options;
};

static void     brush_widget_notify         (GimpInkOptions *options,
                                             GParamSpec     *pspec,
                                             BrushWidget    *brush_widget);
static void     brush_widget_active_rect    (BrushWidget    *brush_widget,
                                             GtkWidget      *widget,
                                             GdkRectangle   *rect);
static void     brush_widget_realize        (GtkWidget      *widget);
static gboolean brush_widget_expose         (GtkWidget      *widget,
                                             GdkEventExpose *event,
                                             BrushWidget    *brush_widget);
static gboolean brush_widget_button_press   (GtkWidget      *widget,
                                             GdkEventButton *event,
                                             BrushWidget    *brush_widget);
static gboolean brush_widget_button_release (GtkWidget      *widget,
                                             GdkEventButton *event,
                                             BrushWidget    *brush_widget);
static gboolean brush_widget_motion_notify  (GtkWidget      *widget,
                                             GdkEventMotion *event,
                                             BrushWidget    *brush_widget);

static void     paint_blob                  (GdkDrawable    *drawable,
                                             GdkGC          *gc,
                                             Blob           *blob);

static GtkWidget *
brush_widget_new (GimpInkOptions *options)
{
  BrushWidget *brush_w;

  brush_w = g_new (BrushWidget, 1);
  brush_w->widget      = gtk_drawing_area_new ();
  brush_w->active      = FALSE;
  brush_w->ink_options = options;

  gtk_widget_set_size_request (brush_w->widget, 60, 60);
  gtk_widget_set_events (brush_w->widget,
			 GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
			 GDK_POINTER_MOTION_MASK |
                         GDK_EXPOSURE_MASK);

  g_signal_connect (options, "notify",
                    G_CALLBACK (brush_widget_notify),
                    brush_w);

  g_signal_connect (brush_w->widget, "button_press_event",
		    G_CALLBACK (brush_widget_button_press),
		    brush_w);
  g_signal_connect (brush_w->widget, "button_release_event",
		    G_CALLBACK (brush_widget_button_release),
		    brush_w);
  g_signal_connect (brush_w->widget, "motion_notify_event",
		    G_CALLBACK (brush_widget_motion_notify),
		    brush_w);
  g_signal_connect (brush_w->widget, "expose_event",
		    G_CALLBACK (brush_widget_expose),
		    brush_w);
  g_signal_connect (brush_w->widget, "realize",
		    G_CALLBACK (brush_widget_realize),
		    brush_w);

  g_object_weak_ref (G_OBJECT (brush_w->widget),
                     (GWeakNotify) g_free, brush_w);

  return brush_w->widget;
}

static void
brush_widget_notify (GimpInkOptions *options,
                     GParamSpec     *pspec,
                     BrushWidget    *brush_w)
{
  if (! strncmp (pspec->name, "blob", 4))
    gtk_widget_queue_draw (brush_w->widget);
}

static void
brush_widget_active_rect (BrushWidget  *brush_widget,
			  GtkWidget    *widget,
			  GdkRectangle *rect)
{
  gint x,y;
  gint r;

  r = MIN (widget->allocation.width, widget->allocation.height) / 2;

  x = (widget->allocation.width / 2 +
       0.85 * r * brush_widget->ink_options->blob_aspect / 10.0 *
       cos (brush_widget->ink_options->blob_angle));

  y = (widget->allocation.height / 2 +
       0.85 * r * brush_widget->ink_options->blob_aspect / 10.0 *
       sin (brush_widget->ink_options->blob_angle));

  rect->x      = x - 5;
  rect->y      = y - 5;
  rect->width  = 10;
  rect->height = 10;
}

static void
brush_widget_realize (GtkWidget *widget)
{
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}

static void
brush_widget_draw_brush (BrushWidget *brush_widget,
			 GtkWidget   *widget,
			 gdouble      xc,
			 gdouble      yc,
			 gdouble      radius)
{
  BlobFunc  function = blob_ellipse;
  Blob     *blob;

  switch (brush_widget->ink_options->blob_type)
    {
    case GIMP_INK_BLOB_TYPE_ELLIPSE:
      function = blob_ellipse;
      break;

    case GIMP_INK_BLOB_TYPE_SQUARE:
      function = blob_square;
      break;

    case GIMP_INK_BLOB_TYPE_DIAMOND:
      function = blob_diamond;
      break;
    }

  blob = (* function) (xc, yc,
                       radius * cos (brush_widget->ink_options->blob_angle),
                       radius * sin (brush_widget->ink_options->blob_angle),
                       (- (radius / brush_widget->ink_options->blob_aspect) *
                        sin (brush_widget->ink_options->blob_angle)),
                       ((radius / brush_widget->ink_options->blob_aspect) *
                        cos (brush_widget->ink_options->blob_angle)));
  paint_blob (widget->window, widget->style->fg_gc[widget->state], blob);
  g_free (blob);
}

static gboolean
brush_widget_expose (GtkWidget      *widget,
		     GdkEventExpose *event,
		     BrushWidget    *brush_widget)
{
  GdkRectangle rect;
  gint         r0;

  r0 = MIN (widget->allocation.width, widget->allocation.height) / 2;

  if (r0 < 2)
    return TRUE;

  brush_widget_draw_brush (brush_widget, widget,
			   widget->allocation.width  / 2,
			   widget->allocation.height / 2,
			   0.9 * r0);

  brush_widget_active_rect (brush_widget, widget, &rect);
  gdk_draw_rectangle (widget->window, widget->style->bg_gc[GTK_STATE_NORMAL],
		      TRUE,	/* filled */
		      rect.x, rect.y,
		      rect.width, rect.height);
  gtk_paint_shadow (widget->style, widget->window, widget->state,
                    GTK_SHADOW_OUT,
                    NULL, widget, NULL,
                    rect.x, rect.y,
                    rect.width, rect.height);

  return TRUE;
}

static gboolean
brush_widget_button_press (GtkWidget      *widget,
			   GdkEventButton *event,
			   BrushWidget    *brush_widget)
{
  GdkRectangle rect;

  brush_widget_active_rect (brush_widget, widget, &rect);

  if ((event->x >= rect.x) && (event->x-rect.x < rect.width) &&
      (event->y >= rect.y) && (event->y-rect.y < rect.height))
    {
      brush_widget->active = TRUE;
    }

  return TRUE;
}

static gboolean
brush_widget_button_release (GtkWidget      *widget,
			     GdkEventButton *event,
			     BrushWidget    *brush_widget)
{
  brush_widget->active = FALSE;

  return TRUE;
}

static gboolean
brush_widget_motion_notify (GtkWidget      *widget,
			    GdkEventMotion *event,
			    BrushWidget    *brush_widget)
{
  if (brush_widget->active)
    {
      gint x;
      gint y;
      gint rsquare;

      x = event->x - widget->allocation.width  / 2;
      y = event->y - widget->allocation.height / 2;

      rsquare = x*x + y*y;

      if (rsquare != 0)
	{
          gint    r0;
          gdouble angle;
          gdouble aspect;

	  r0 = MIN (widget->allocation.width, widget->allocation.height) / 2;

          angle  = atan2 (y, x);
          aspect = 10.0 * sqrt ((gdouble) rsquare / (r0 * r0)) / 0.85;

          aspect = CLAMP (aspect, 1.0, 10.0);

          g_object_set (brush_widget->ink_options,
                        "blob-angle",  angle,
                        "blob-aspect", aspect,
                        NULL);

	  gtk_widget_queue_draw (widget);
	}
    }

  return TRUE;
}


/*  blob button functions  */

static gboolean   blob_button_expose (GtkWidget      *widget,
                                      GdkEventExpose *event,
                                      BlobFunc        function);

static GtkWidget *
blob_button_new (GimpInkBlobType blob_type)
{
  GtkWidget *blob;
  BlobFunc   function = blob_ellipse;

  switch (blob_type)
    {
    case GIMP_INK_BLOB_TYPE_ELLIPSE:
      function = blob_ellipse;
      break;

    case GIMP_INK_BLOB_TYPE_SQUARE:
      function = blob_square;
      break;

    case GIMP_INK_BLOB_TYPE_DIAMOND:
      function = blob_diamond;
      break;
    }

  blob = gtk_drawing_area_new ();
  gtk_widget_set_size_request (blob, 21, 21);

  g_signal_connect (blob, "expose_event",
                    G_CALLBACK (blob_button_expose),
                    function);

  return blob;
}

static gboolean
blob_button_expose (GtkWidget      *widget,
                    GdkEventExpose *event,
                    BlobFunc        function)
{
  Blob *blob;

  blob = (* function) (widget->allocation.width  / 2,
                       widget->allocation.height / 2,
                       8, 0, 0, 8);
  paint_blob (widget->window, widget->style->fg_gc[widget->state], blob);
  g_free (blob);

  return TRUE;
}


/*  Draw a blob onto a drawable with the specified graphics context  */

static void
paint_blob (GdkDrawable *drawable,
	    GdkGC       *gc,
	    Blob        *blob)
{
  gint i;

  for (i = 0; i < blob->height; i++)
    if (blob->data[i].left <= blob->data[i].right)
      gdk_draw_line (drawable, gc,
                     blob->data[i].left,      i + blob->y,
                     blob->data[i].right + 1, i + blob->y);
}
