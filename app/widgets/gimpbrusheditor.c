/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush_edit module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "appenv.h"
#include "gimpbrushgenerated.h"
#include "brush_edit.h"
#include "actionarea.h"
#include "math.h"


static void
brush_edit_close_callback (GtkWidget *w, void *data);

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Close", brush_edit_close_callback, NULL, NULL }
};

static void
update_brush_callback (GtkAdjustment *adjustment,
		       BrushEditGeneratedWindow *begw)
{
  if (begw->brush &&
      ((begw->radius_data->value  
	!= gimp_brush_generated_get_radius(begw->brush))
       || (begw->hardness_data->value
	   != gimp_brush_generated_get_hardness(begw->brush))
       || (begw->aspect_ratio_data->value
	   != gimp_brush_generated_get_aspect_ratio(begw->brush))
       || (begw->angle_data->value
	   !=gimp_brush_generated_get_angle(begw->brush))))
  {
    gimp_brush_generated_freeze (begw->brush);
    gimp_brush_generated_set_radius       (begw->brush,
					   begw->radius_data->value);
    gimp_brush_generated_set_hardness     (begw->brush,
					   begw->hardness_data->value);
    gimp_brush_generated_set_aspect_ratio (begw->brush,
					   begw->aspect_ratio_data->value);
    gimp_brush_generated_set_angle        (begw->brush,
					   begw->angle_data->value);
    gimp_brush_generated_thaw (begw->brush);
  }
}

static gint
brush_edit_delete_callback (GtkWidget *w,
			    BrushEditGeneratedWindow *begw)
{
  if (GTK_WIDGET_VISIBLE (w))
    gtk_widget_hide (w);
  return TRUE;
}

static void
brush_edit_clear_preview (BrushEditGeneratedWindow *begw)
{
  unsigned char * buf;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * begw->preview->requisition.width);

  /*  Set the buffer to white  */
  memset (buf, 255, begw->preview->requisition.width);

  /*  Set the image buffer to white  */
  for (i = 0; i < begw->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (begw->preview), buf, 0, i,
			  begw->preview->requisition.width);

  g_free (buf);
}

static gint 
brush_edit_brush_dirty_callback(GimpBrush *brush,
				BrushEditGeneratedWindow *begw)
{
  int x, y, width, yend, ystart, xo;
  int scale;
  gchar *src, *buf;

  brush_edit_clear_preview (begw);

  scale = MAX(ceil(brush->mask->width/(float)begw->preview->requisition.width),
	      ceil(brush->mask->height/(float)begw->preview->requisition.height));

  ystart = 0;
  xo = begw->preview->requisition.width/2 - brush->mask->width/(2*scale);
  ystart = begw->preview->requisition.height/2 - brush->mask->height/(2*scale);
  yend = ystart + brush->mask->height/(scale);
  width = BOUNDS (brush->mask->width/scale, 0, begw->preview->requisition.width);

  buf = g_new (gchar, width);
  src = (gchar *)temp_buf_data (brush->mask);

  for (y = ystart; y < yend; y++)
  {
    /*  Invert the mask for display.  We're doing this because
     *  a value of 255 in the  mask means it is full intensity.
     *  However, it makes more sense for full intensity to show
     *  up as black in this brush preview window...
       */
    for (x = 0; x < width; x++)
      buf[x] = 255 - src[x*scale];
    gtk_preview_draw_row (GTK_PREVIEW (begw->preview), (guchar *)buf, xo, y,
			  width);
    src += brush->mask->width*scale;
  }
  g_free(buf);
  gtk_widget_draw(begw->preview, NULL);
  return TRUE;
}

void
brush_edit_generated_set_brush(BrushEditGeneratedWindow *begw,
			       GimpBrush *gbrush)
{
  GimpBrushGenerated *brush = 0;
  if (!GIMP_IS_BRUSH_GENERATED(gbrush))
  {
    if (GTK_WIDGET_VISIBLE (begw->shell))
      gtk_widget_hide (begw->shell);
    return;
  }
  brush = GIMP_BRUSH_GENERATED(gbrush);
  if (begw->brush)
    gtk_signal_disconnect_by_data(GTK_OBJECT(begw->brush), begw);
  if (begw)
  {
    gtk_signal_connect(GTK_OBJECT (brush), "dirty",
		       GTK_SIGNAL_FUNC(brush_edit_brush_dirty_callback),
		       begw);
    begw->brush = NULL;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->radius_data),
			     gimp_brush_generated_get_radius (brush));
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->hardness_data),
			     gimp_brush_generated_get_hardness (brush));
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->angle_data),
			     gimp_brush_generated_get_angle (brush));
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->aspect_ratio_data),
			     gimp_brush_generated_get_aspect_ratio(brush));
    begw->brush = brush;
    brush_edit_brush_dirty_callback(GIMP_BRUSH(brush), begw);
  }
}

BrushEditGeneratedWindow *
brush_edit_generated_new ()
{
  BrushEditGeneratedWindow *begw ;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *table;



  begw = g_malloc (sizeof (BrushEditGeneratedWindow));
  begw->brush = NULL;
  begw->redraw = TRUE;

  begw->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (begw->shell), "generatedbrusheditor",
			  "Gimp");
  gtk_window_set_title (GTK_WINDOW (begw->shell), "Brush Editor");
  gtk_window_set_policy(GTK_WINDOW(begw->shell), FALSE, TRUE, FALSE);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (begw->shell)->vbox), vbox,
		      TRUE, TRUE, 0);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (begw->shell), "delete_event",
		      GTK_SIGNAL_FUNC (brush_edit_delete_callback),
		      begw);

/*  Populate the window with some widgets */

  /* brush's preview widget w/frame  */
  begw->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (begw->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), begw->frame, TRUE, TRUE, 0);

  begw->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (begw->preview), 100, 100);
  gtk_container_add (GTK_CONTAINER (begw->frame), begw->preview);

  gtk_widget_show(begw->preview);
  gtk_widget_show(begw->frame);

  /* table for sliders/labels */
  table = gtk_table_new(2, 4, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /* brush radius scale */
  label = gtk_label_new ("Radius:");
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
/*  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 0, 0); */
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, 3, 0, 0, 0);
  begw->radius_data = GTK_ADJUSTMENT (gtk_adjustment_new (10.0, 0.0, 100.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->radius_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 0, 1);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->radius_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  /* brush hardness scale */
  
  label = gtk_label_new ("Hardness:");
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 1, 2, 0, 0, 0, 0);
  begw->hardness_data = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.01, 0.0));
  slider = gtk_hscale_new (begw->hardness_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 1, 2);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->hardness_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  /* brush aspect ratio scale */
  
  label = gtk_label_new ("Aspect Ratio:");
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 2, 3, 0, 0, 0, 0);
  begw->aspect_ratio_data = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 1.0, 20.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->aspect_ratio_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 2, 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->aspect_ratio_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  /* brush angle scale */

  label = gtk_label_new ("Angle:");
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 3, 4, 0, 0, 0, 0);
  begw->angle_data = GTK_ADJUSTMENT (gtk_adjustment_new (00.0, 0.0, 180.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->angle_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 3, 4);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->angle_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  gtk_table_set_row_spacings(GTK_TABLE (table), 3);
  gtk_table_set_col_spacing(GTK_TABLE (table), 0, 3);
  gtk_widget_show (table);

  /*  The action area  */
  action_items[0].user_data = begw;
  build_action_area (GTK_DIALOG (begw->shell), action_items, 1, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (begw->shell);


  return begw;
}

static void
brush_edit_close_callback (GtkWidget *w, void *data)
{
  BrushEditGeneratedWindow *begw = (BrushEditGeneratedWindow *)data;
  if (GTK_WIDGET_VISIBLE (begw->shell))
    gtk_widget_hide (begw->shell);
}
