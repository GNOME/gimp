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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "gimpinkoptions.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_ink_options_init       (GimpInkOptions      *options);
static void   gimp_ink_options_class_init (GimpInkOptionsClass *options_class);

static void   gimp_ink_options_set_property (GObject         *object,
                                             guint            property_id,
                                             const GValue    *value,
                                             GParamSpec      *pspec);
static void   gimp_ink_options_get_property (GObject         *object,
                                             guint            property_id,
                                             GValue          *value,
                                             GParamSpec      *pspec);

static void   gimp_ink_options_reset        (GimpToolOptions *tool_options);

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

static void     ink_type_update        (GtkWidget       *radio_button,
                                        GimpInkOptions  *options);

static void
brush_widget_active_rect (BrushWidget  *brush_widget,
			  GtkWidget    *widget,
			  GdkRectangle *rect)
{
  gint x,y;
  gint r;

  r = MIN (widget->allocation.width, widget->allocation.height) / 2;

  x = widget->allocation.width / 2 + 0.85 * r * brush_widget->ink_options->aspect / 10.0 *
    cos (brush_widget->ink_options->angle);
  y = widget->allocation.height / 2 + 0.85 * r * brush_widget->ink_options->aspect / 10.0 *
    sin (brush_widget->ink_options->angle);

  rect->x = x - 5;
  rect->y = y - 5;
  rect->width = 10;
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
  Blob *blob;

  blob = brush_widget->ink_options->function (xc, yc,
                                              radius * cos (brush_widget->ink_options->angle),
                                              radius * sin (brush_widget->ink_options->angle),
                                              (- (radius / brush_widget->ink_options->aspect) *
                                               sin (brush_widget->ink_options->angle)),
                                              ((radius / brush_widget->ink_options->aspect) *
                                               cos (brush_widget->ink_options->angle)));

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
      brush_widget->state = TRUE;

      gtk_grab_add (brush_widget->widget);
    }

  return TRUE;
}

static gboolean
brush_widget_button_release (GtkWidget      *widget,
			     GdkEventButton *event,
			     BrushWidget    *brush_widget)
{
  brush_widget->state = FALSE;

  gtk_grab_remove (brush_widget->widget);

  return TRUE;
}

static gboolean
brush_widget_motion_notify (GtkWidget      *widget,
			    GdkEventMotion *event,
			    BrushWidget    *brush_widget)
{
  int x;
  int y;
  int r0;
  int rsquare;

  if (brush_widget->state)
    {
      x = event->x - widget->allocation.width / 2;
      y = event->y - widget->allocation.height / 2;
      rsquare = x*x + y*y;

      if (rsquare != 0)
	{
	  brush_widget->ink_options->angle = atan2 (y, x);

	  r0 = MIN (widget->allocation.width, widget->allocation.height) / 2;
	  brush_widget->ink_options->aspect =
	    10.0 * sqrt ((gdouble) rsquare / (r0 * r0)) / 0.85;

	  if (brush_widget->ink_options->aspect < 1.0)
	    brush_widget->ink_options->aspect = 1.0;
	  if (brush_widget->ink_options->aspect > 10.0)
	    brush_widget->ink_options->aspect = 10.0;

	  gtk_widget_queue_draw (widget);
	}
    }

  return TRUE;
}

static gboolean
blob_button_expose (GtkWidget      *widget,
                    GdkEventExpose *event,
                    BlobFunc        function)
{
  Blob *blob = (*function) (widget->allocation.width  / 2,
                            widget->allocation.height / 2, 8, 0, 0, 8);
  paint_blob (widget->window, widget->style->fg_gc[widget->state], blob);
  g_free (blob);

  return TRUE;
}

/*
 * Draw a blob onto a drawable with the specified graphics context
 */
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


static GimpPaintOptionsClass *parent_class = NULL;


GType
gimp_ink_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpInkOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_ink_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpInkOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_ink_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpInkOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_ink_options_class_init (GimpInkOptionsClass *klass)
{
  GObjectClass         *object_class;
  GimpToolOptionsClass *options_class;

  object_class  = G_OBJECT_CLASS (klass);
  options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_ink_options_set_property;
  object_class->get_property = gimp_ink_options_get_property;

  options_class->reset       = gimp_ink_options_reset;
}

static void
gimp_ink_options_init (GimpInkOptions *options)
{
  options->size             = options->size_d             = 4.4;
  options->sensitivity      = options->sensitivity_d      = 1.0;
  options->vel_sensitivity  = options->vel_sensitivity_d  = 0.8;
  options->tilt_sensitivity = options->tilt_sensitivity_d = 0.4;
  options->tilt_angle       = options->tilt_angle_d       = 0.0;
  options->function         = options->function_d         = blob_ellipse;
  options->aspect           = options->aspect_d           = 1.0;
  options->angle            = options->angle_d            = 0.0;
}

static void
gimp_ink_options_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
}

static void
gimp_ink_options_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
}

void
gimp_ink_options_gui (GimpToolOptions *tool_options)
{
  GimpInkOptions *options;
  GtkWidget      *table;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *hbox2;
  GtkWidget      *radio_button;
  GtkWidget      *blob;
  GtkWidget      *frame;
  GtkWidget      *darea;

  options = GIMP_INK_OPTIONS (tool_options);

  gimp_paint_options_gui (tool_options);

  vbox = tool_options->main_vbox;

  /* adjust sliders */
  frame = gtk_frame_new (_("Adjustment"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*  size slider  */
  options->size_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					  _("Size:"), -1, 5,
					  options->size,
					  0.0, 20.0, 1.0, 2.0, 1,
					  TRUE, 0.0, 0.0,
					  NULL, NULL);

  g_signal_connect (options->size_w, "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &options->size);

  /* angle adjust slider */
  options->tilt_angle_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
						_("Angle:"), -1, 5,
						options->tilt_angle,
						-90.0, 90.0, 1, 10.0, 1,
						TRUE, 0.0, 0.0,
						NULL, NULL);

  g_signal_connect (options->tilt_angle_w, "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &options->tilt_angle);

  /* sens sliders */
  frame = gtk_frame_new (_("Sensitivity"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /* size sens slider */
  options->sensitivity_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
						 _("Size:"), -1, -1,
						 options->sensitivity,
						 0.0, 1.0, 0.01, 0.1, 1,
						 TRUE, 0.0, 0.0,
						 NULL, NULL);

  g_signal_connect (options->sensitivity_w, "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &options->sensitivity);
  
  /* tilt sens slider */
  options->tilt_sensitivity_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
						      _("Tilt:"), -1, -1,
						      options->tilt_sensitivity,
						      0.0, 1.0, 0.01, 0.1, 1,
						      TRUE, 0.0, 0.0,
						      NULL, NULL);

  g_signal_connect (options->tilt_sensitivity_w, "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &options->tilt_sensitivity);

  /* velocity sens slider */
  options->vel_sensitivity_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
						     _("Speed:"), -1, -1,
						     options->vel_sensitivity,
						     0.0, 1.0, 0.01, 0.1, 1,
						     TRUE, 0.0, 0.0,
						     NULL, NULL);

  g_signal_connect (options->vel_sensitivity_w, "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &options->vel_sensitivity);

  /*  bottom hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* Brush type radiobuttons */
  frame = gtk_frame_new (_("Type"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);
  
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  blob = gtk_drawing_area_new ();
  gtk_widget_set_size_request (blob, 21, 21);
  g_signal_connect (blob, "expose_event",
                    G_CALLBACK (blob_button_expose),
                    blob_ellipse);

  radio_button = gtk_radio_button_new (NULL);
  gtk_container_add (GTK_CONTAINER (radio_button), blob);
  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (radio_button), "gimp-item-data", blob_ellipse);

  g_signal_connect (radio_button, "toggled",
		    G_CALLBACK (ink_type_update),
		    options);

  options->function_w[0] = radio_button;

  blob = gtk_drawing_area_new ();
  gtk_widget_set_size_request (blob, 21, 21);
  g_signal_connect (blob, "expose_event",
                    G_CALLBACK (blob_button_expose),
                    blob_square);
  radio_button =
    gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_button));
  gtk_container_add (GTK_CONTAINER (radio_button), blob);
  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (radio_button), "gimp-item-data", blob_square);

  g_signal_connect (radio_button, "toggled",
		    G_CALLBACK (ink_type_update), 
		    options);
  

  options->function_w[1] = radio_button;

  blob = gtk_drawing_area_new ();
  gtk_widget_set_size_request (blob, 21, 21);
  g_signal_connect (blob, "expose_event",
                    G_CALLBACK (blob_button_expose),
                    blob_diamond);
  radio_button =
    gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_button));
  gtk_container_add (GTK_CONTAINER (radio_button), blob);
  gtk_box_pack_start (GTK_BOX (vbox), radio_button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (radio_button), "gimp-item-data", blob_diamond);

  g_signal_connect (radio_button, "toggled",
		    G_CALLBACK (ink_type_update), 
		    options);

  options->function_w[2] = radio_button;

  /* Brush shape widget */
  frame = gtk_frame_new (_("Shape"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gtk_aspect_frame_new (NULL, 0.0, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  options->brush_w = g_new (BrushWidget, 1);
  options->brush_w->state       = FALSE;
  options->brush_w->ink_options = options;

  darea = gtk_drawing_area_new ();
  options->brush_w->widget = darea;

  gtk_widget_set_size_request (darea, 60, 60);
  gtk_container_add (GTK_CONTAINER (frame), darea);

  gtk_widget_set_events (darea, 
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);

  g_signal_connect (darea, "button_press_event",
		    G_CALLBACK (brush_widget_button_press),
		    options->brush_w);
  g_signal_connect (darea, "button_release_event",
		    G_CALLBACK (brush_widget_button_release),
		    options->brush_w);
  g_signal_connect (darea, "motion_notify_event",
		    G_CALLBACK (brush_widget_motion_notify),
		    options->brush_w);
  g_signal_connect (darea, "expose_event",
		    G_CALLBACK (brush_widget_expose), 
		    options->brush_w);
  g_signal_connect (darea, "realize",
		    G_CALLBACK (brush_widget_realize),
		    options->brush_w);

  gtk_widget_show_all (hbox);

  gimp_ink_options_reset (tool_options);
}

static void
gimp_ink_options_reset (GimpToolOptions *tool_options)
{
  GimpInkOptions *options;

  options = GIMP_INK_OPTIONS (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->size_w),
			    options->size_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->sensitivity_w),
			    options->sensitivity_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->tilt_sensitivity_w),
			    options->tilt_sensitivity_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->vel_sensitivity_w),
			    options->vel_sensitivity_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->tilt_angle_w),
			    options->tilt_angle_d);
  gtk_toggle_button_set_active (((options->function_d == blob_ellipse) ?
				 GTK_TOGGLE_BUTTON (options->function_w[0]) :
				 ((options->function_d == blob_square) ?
				  GTK_TOGGLE_BUTTON (options->function_w[1]) :
				  GTK_TOGGLE_BUTTON (options->function_w[2]))),
				TRUE);
  options->aspect = options->aspect_d;
  options->angle  = options->angle_d;
  gtk_widget_queue_draw (options->brush_w->widget);

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (tool_options);
}

static void
ink_type_update (GtkWidget      *radio_button,
		 GimpInkOptions *options)
{
  BlobFunc function;

  function = g_object_get_data (G_OBJECT (radio_button), "gimp-item-data");

  if (GTK_TOGGLE_BUTTON (radio_button)->active)
    options->function = function;

  gtk_widget_queue_draw (options->brush_w->widget);
}
