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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "resize.h"

#define EVENT_MASK  GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
#define DRAWING_AREA_SIZE 200
#define TEXT_WIDTH 35

typedef struct _ResizePrivate ResizePrivate;

struct _ResizePrivate
{
  GtkWidget *width_text;
  GtkWidget *height_text;
  GtkWidget *ratio_x_text;
  GtkWidget *ratio_y_text;
  GtkWidget *off_x_text;
  GtkWidget *off_y_text;
  GtkWidget *drawing_area;

  double ratio;
  int constrain;
  int old_width, old_height;
  int area_width, area_height;
  int start_x, start_y;
  int orig_x, orig_y;
};

static void resize_draw (Resize *);
static int  resize_bound_off_x (Resize *, int);
static int  resize_bound_off_y (Resize *, int);
static void off_x_update (GtkWidget *w, gpointer data);
static void off_y_update (GtkWidget *w, gpointer data);
static void width_update (GtkWidget *w, gpointer data);
static void height_update (GtkWidget *w, gpointer data);
static void ratio_x_update (GtkWidget *w, gpointer data);
static void ratio_y_update (GtkWidget *w, gpointer data);
static void constrain_update (GtkWidget *w, gpointer data);
static gint resize_events (GtkWidget *area, GdkEvent *event);


Resize *
resize_widget_new (ResizeType type,
		   int        width,
		   int        height)
{
  Resize *resize;
  ResizePrivate *private;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *constrain;
  GtkWidget *table;
  char size[12];
  char ratio_text[12];

  table = NULL;

  resize = g_new (Resize, 1);
  private = g_new (ResizePrivate, 1);
  resize->type = type;
  resize->private_part = private;
  resize->width = width;
  resize->height = height;
  resize->ratio_x = 1.0;
  resize->ratio_y = 1.0;
  resize->off_x = 0;
  resize->off_y = 0;
  private->old_width = width;
  private->old_height = height;
  private->constrain = TRUE;

  /*  Get the image width and height variables, based on the gimage  */
  if (width > height)
    private->ratio = (double) DRAWING_AREA_SIZE / (double) width;
  else
    private->ratio = (double) DRAWING_AREA_SIZE / (double) height;
  private->area_width = (int) (private->ratio * width);
  private->area_height = (int) (private->ratio * height);

  switch (type)
    {
    case ScaleWidget:
      resize->resize_widget = gtk_frame_new ("Scale");
      table = gtk_table_new (4, 2, TRUE);
      break;
    case ResizeWidget:
      resize->resize_widget = gtk_frame_new ("Resize");
      table = gtk_table_new (6, 2, TRUE);
      break;
    }
  gtk_frame_set_shadow_type (GTK_FRAME (resize->resize_widget), GTK_SHADOW_ETCHED_IN);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (resize->resize_widget), vbox);

  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the width label and entry  */
  sprintf (size, "%d", width);
  label = gtk_label_new ("New width:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_show (label);
  private->width_text = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), private->width_text, 1, 2, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_set_usize (private->width_text, TEXT_WIDTH, 25);
  gtk_entry_set_text (GTK_ENTRY (private->width_text), size);
  gtk_signal_connect (GTK_OBJECT (private->width_text), "changed",
		      (GtkSignalFunc) width_update,
		      resize);
  gtk_widget_show (private->width_text);

  /*  the height label and entry  */
  sprintf (size, "%d", height);
  label = gtk_label_new ("New height:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_show (label);
  private->height_text = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), private->height_text, 1, 2, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_set_usize (private->height_text, TEXT_WIDTH, 25);
  gtk_entry_set_text (GTK_ENTRY (private->height_text), size);
  gtk_signal_connect (GTK_OBJECT (private->height_text), "changed",
		      (GtkSignalFunc) height_update,
		      resize);
  gtk_widget_show (private->height_text);

  /*  the x scale ratio label and entry  */
  sprintf (ratio_text, "%0.4f", resize->ratio_x);
  label = gtk_label_new ("X ratio:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_show (label);
  private->ratio_x_text = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), private->ratio_x_text, 1, 2, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_set_usize (private->ratio_x_text, TEXT_WIDTH, 25);
  gtk_entry_set_text (GTK_ENTRY (private->ratio_x_text), ratio_text);
  gtk_signal_connect (GTK_OBJECT (private->ratio_x_text), "changed",
		      (GtkSignalFunc) ratio_x_update,
		      resize);
  gtk_widget_show (private->ratio_x_text);

  /*  the y scale ratio label and entry  */
  sprintf (ratio_text, "%0.4f", resize->ratio_y);
  label = gtk_label_new ("Y ratio:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_show (label);
  private->ratio_y_text = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), private->ratio_y_text, 1, 2, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  gtk_widget_set_usize (private->ratio_y_text, TEXT_WIDTH, 25);
  gtk_entry_set_text (GTK_ENTRY (private->ratio_y_text), ratio_text);
  gtk_signal_connect (GTK_OBJECT (private->ratio_y_text), "changed",
		      (GtkSignalFunc) ratio_y_update,
		      resize);
  gtk_widget_show (private->ratio_y_text);

  if (type == ResizeWidget)
    {
      /*  the off_x label and entry  */
      sprintf (size, "%d", 0);
      label = gtk_label_new ("X Offset:");
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
      gtk_widget_show (label);
      private->off_x_text = gtk_entry_new ();
      gtk_table_attach (GTK_TABLE (table), private->off_x_text, 1, 2, 4, 5,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
      gtk_widget_set_usize (private->off_x_text, TEXT_WIDTH, 25);
      gtk_entry_set_text (GTK_ENTRY (private->off_x_text), size);
      gtk_signal_connect (GTK_OBJECT (private->off_x_text), "changed",
			  (GtkSignalFunc) off_x_update,
			  resize);
      gtk_widget_show (private->off_x_text);

      /*  the off_y label and entry  */
      sprintf (size, "%d", 0);
      label = gtk_label_new ("Y Offset:");
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
      gtk_widget_show (label);
      private->off_y_text = gtk_entry_new ();
      gtk_table_attach (GTK_TABLE (table), private->off_y_text, 1, 2, 5, 6,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
      gtk_widget_set_usize (private->off_y_text, TEXT_WIDTH, 25);
      gtk_entry_set_text (GTK_ENTRY (private->off_y_text), size);
      gtk_signal_connect (GTK_OBJECT (private->off_y_text), "changed",
			  (GtkSignalFunc) off_y_update,
			  resize);
      gtk_widget_show (private->off_y_text);
    }

  /*  the constrain toggle button  */
  constrain = gtk_check_button_new_with_label ("Constrain Ratio");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (constrain), private->constrain);
  gtk_box_pack_start (GTK_BOX (vbox), constrain, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (constrain), "toggled",
		      (GtkSignalFunc) constrain_update,
		      resize);
  gtk_widget_show (constrain);

  if (type == ResizeWidget)
    {
      /*  frame to hold drawing area  */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (frame), 2);
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
      private->drawing_area = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (private->drawing_area),
			     private->area_width, private->area_height);
      gtk_widget_set_events (private->drawing_area, EVENT_MASK);
      gtk_signal_connect (GTK_OBJECT (private->drawing_area), "event",
			  (GtkSignalFunc) resize_events,
			  NULL);
      gtk_object_set_user_data (GTK_OBJECT (private->drawing_area), resize);
      gtk_container_add (GTK_CONTAINER (frame), private->drawing_area);
      gtk_widget_show (private->drawing_area);
      gtk_widget_show (frame);
      gtk_widget_show (hbox);
    }

  gtk_widget_show (table);
  gtk_widget_show (vbox);

  return resize;
}

void
resize_widget_free (Resize *resize)
{
  g_free (resize->private_part);
  g_free (resize);
}

static void
resize_draw (Resize *resize)
{
  GtkWidget *widget;
  ResizePrivate *private;
  int aw, ah;
  int x, y;
  int w, h;

  /*  Only need to draw if it's a resize widget  */
  if (resize->type != ResizeWidget)
    return;

  private = (ResizePrivate *) resize->private_part;
  widget = private->drawing_area;

  /*  If we're making the size larger  */
  if (private->old_width <= resize->width)
    w = resize->width;
  /*  otherwise, if we're making the size smaller  */
  else
    w = private->old_width * 2 - resize->width;
  /*  If we're making the size larger  */
  if (private->old_height <= resize->height)
    h = resize->height;
  /*  otherwise, if we're making the size smaller  */
  else
    h = private->old_height * 2 - resize->height;

  if (w > h)
    private->ratio = (double) DRAWING_AREA_SIZE / (double) w;
  else
    private->ratio = (double) DRAWING_AREA_SIZE / (double) h;

  aw = (int) (private->ratio * w);
  ah = (int) (private->ratio * h);

  if (aw != private->area_width || ah != private->area_height)
    {
      private->area_width = aw;
      private->area_height = ah;
      gtk_widget_set_usize (private->drawing_area, aw, ah);
    }

  if (private->old_width <= resize->width)
    x = private->ratio * resize->off_x;
  else
    x = private->ratio * (resize->off_x + private->old_width - resize->width);
  if (private->old_height <= resize->height)
    y = private->ratio * resize->off_y;
  else
    y = private->ratio * (resize->off_y + private->old_height - resize->height);

  w = private->ratio * private->old_width;
  h = private->ratio * private->old_height;

  gdk_window_clear (private->drawing_area->window);
  gtk_draw_shadow (widget->style, widget->window,
		   GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		   x, y, w, h);

  /*  If we're making the size smaller  */
  if (private->old_width > resize->width ||
      private->old_height > resize->height)
    {
      if (private->old_width > resize->width)
	{
	  x = private->ratio * (private->old_width - resize->width);
	  w = private->ratio * resize->width;
	}
      else
	{
	  x = -1;
	  w = aw + 2;
	}
      if (private->old_height > resize->height)
	{
	  y = private->ratio * (private->old_height - resize->height);
	  h = private->ratio * resize->height;
	}
      else
	{
	  y = -1;
	  h = ah + 2;
	}

      gdk_draw_rectangle (private->drawing_area->window,
			  widget->style->black_gc, 0,
			  x, y, w, h);
    }

}

static int
resize_bound_off_x (Resize *resize,
		    int     off_x)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize->private_part;

  if (private->old_width <= resize->width)
    off_x = BOUNDS (off_x, 0, (resize->width - private->old_width));
  else
    off_x = BOUNDS (off_x, (resize->width - private->old_width), 0);

  return off_x;
}

static int
resize_bound_off_y (Resize *resize,
		    int     off_y)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize->private_part;

  if (private->old_height <= resize->height)
    off_y = BOUNDS (off_y, 0, (resize->height - private->old_height));
  else
    off_y = BOUNDS (off_y, (resize->height - private->old_height), 0);

  return off_y;
}

static void
constrain_update (GtkWidget *w,
		  gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

  if (GTK_TOGGLE_BUTTON (w)->active)
    private->constrain = TRUE;
  else
    private->constrain = FALSE;
}

static void
off_x_update (GtkWidget *w,
	      gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  char *str;
  int offset;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;
  str = gtk_entry_get_text (GTK_ENTRY (w));

  offset = atoi (str);
  offset = resize_bound_off_x (resize, offset);

  if (offset != resize->off_x)
    {
      resize->off_x = offset;
      resize_draw (resize);
    }
}

static void
off_y_update (GtkWidget *w,
	      gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  char *str;
  int offset;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;
  str = gtk_entry_get_text (GTK_ENTRY (w));

  offset = atoi (str);
  offset = resize_bound_off_y (resize, offset);

  if (offset != resize->off_y)
    {
      resize->off_y = offset;
      resize_draw (resize);
    }
}

static void
width_update (GtkWidget *w,
	      gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  char *str;
  double ratio;
  int new_height;
  char size[12];
  char ratio_text[12];

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;
  str = gtk_entry_get_text (GTK_ENTRY (w));

  resize->width = atoi (str);

  ratio = (double) resize->width / (double) private->old_width;
  resize->ratio_x = ratio;
  sprintf (ratio_text, "%0.4f", ratio);  

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_x_text), data);
  gtk_entry_set_text (GTK_ENTRY (private->ratio_x_text), ratio_text);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_x_text), data);

  if (resize->type == ResizeWidget)
    {
      resize->off_x = resize_bound_off_x (resize, (resize->width - private->old_width) / 2);
      sprintf (size, "%d", resize->off_x);

      gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_x_text), data);
      gtk_entry_set_text (GTK_ENTRY (private->off_x_text), size);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_x_text), data);
    }

  if (private->constrain && resize->width != 0)
    {
      private->constrain = FALSE;
      new_height = (int) (private->old_height * ratio);
      if (new_height == 0) new_height = 1;

      if (new_height != resize->height)
	{
	  resize->height = new_height;
	  sprintf (size, "%d", resize->height);

	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->height_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->height_text), size);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->height_text), data);

	  resize->ratio_y = ratio;

	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_y_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->ratio_y_text), ratio_text);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_y_text), data);

	  if (resize->type == ResizeWidget)
	    {
	      resize->off_y = resize_bound_off_y (resize, (resize->height - private->old_height) / 2);
	      sprintf (size, "%d", resize->off_y);

	      gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_y_text), data);
	      gtk_entry_set_text (GTK_ENTRY (private->off_y_text), size);
	      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_y_text), data);
	    }
	}

      private->constrain = TRUE;
    }

  resize_draw (resize);
}

static void
height_update (GtkWidget *w,
	       gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  char *str;
  double ratio;
  int new_width;
  char size[12];
  char ratio_text[12];

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;
  str = gtk_entry_get_text (GTK_ENTRY (w));

  resize->height = atoi (str);

  ratio = (double) resize->height / (double) private->old_height;
  resize->ratio_y = ratio;
  sprintf (ratio_text, "%0.4f", ratio);

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_y_text), data);
  gtk_entry_set_text (GTK_ENTRY (private->ratio_y_text), ratio_text);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_y_text), data);
  if (resize->type == ResizeWidget)
    {
      resize->off_y = resize_bound_off_y (resize, (resize->height - private->old_height) / 2);
      sprintf (size, "%d", resize->off_y);

      gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_y_text), data);
      gtk_entry_set_text (GTK_ENTRY (private->off_y_text), size);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_y_text), data);
    }

  if (private->constrain && resize->height != 0)
    {
      private->constrain = FALSE;
      ratio = (double) resize->height / (double) private->old_height;
      new_width = (int) (private->old_width * ratio);
      if (new_width == 0) new_width = 1;

      if (new_width != resize->width)
	{
	  resize->width = new_width;
	  sprintf (size, "%d", resize->width);

	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->width_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->width_text), size);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->width_text), data);
	  
	  resize->ratio_x = ratio;

	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_x_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->ratio_x_text), ratio_text);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_x_text), data);

	  if (resize->type == ResizeWidget)
	    {
	      resize->off_x = resize_bound_off_x (resize, (resize->width - private->old_width) / 2);
	      sprintf (size, "%d", resize->off_x);

	      gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_x_text), data);
	      gtk_entry_set_text (GTK_ENTRY (private->off_x_text), size);
	      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_x_text), data);
	    }
	}

      private->constrain = TRUE;
    }

  resize_draw (resize);
}

static void
ratio_x_update (GtkWidget *w,
		gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  char *str;
  int new_width;
  int new_height;
  char size[12];
  char ratio_text[12];
  
  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;
  str = gtk_entry_get_text (GTK_ENTRY (w));

  resize->ratio_x = atof (str);

  new_width = (int) ((double) private->old_width * resize->ratio_x);

  if (new_width != resize->width)
    {
      resize->width = new_width;
      sprintf (size, "%d", new_width);

      gtk_signal_handler_block_by_data (GTK_OBJECT (private->width_text), data);
      gtk_entry_set_text (GTK_ENTRY (private->width_text), size);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->width_text), data);

      if (resize->type == ResizeWidget)
	{
	  resize->off_x = resize_bound_off_x (resize, (resize->width - private->old_width) / 2);
	  sprintf (size, "%d", resize->off_x);
	  
	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_x_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->off_x_text), size);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_x_text), data);
	}
    }

  if (private->constrain && resize->width != 0)
    {
      private->constrain = FALSE;

      resize->ratio_y = resize->ratio_x;

      new_height = (int) (private->old_height * resize->ratio_y);
      if (new_height == 0) new_height = 1;

      if (new_height != resize->height)
	{
	  resize->height = new_height;

	  sprintf (size, "%d", resize->height);

	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->height_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->height_text), size);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->height_text), data);
	  
	  sprintf (ratio_text, "%0.4f", resize->ratio_y);  
	  
	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_y_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->ratio_y_text), ratio_text);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_y_text), data);

	  if (resize->type == ResizeWidget)
	    {
	      resize->off_y = resize_bound_off_y (resize, (resize->height - private->old_height) / 2);
	      sprintf (size, "%d", resize->off_y);

	      gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_y_text), data);
	      gtk_entry_set_text (GTK_ENTRY (private->off_y_text), size);
	      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_y_text), data);
	    }
	}

      private->constrain = TRUE;
    }

  resize_draw (resize);
}

static void
ratio_y_update (GtkWidget *w,
		gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  char *str;
  int new_width;
  int new_height;
  char size[12];
  char ratio_text[12];
  
  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;
  str = gtk_entry_get_text (GTK_ENTRY (w));

  resize->ratio_y = atof (str);

  new_height = (int) ((double) private->old_height * resize->ratio_y);

  if (new_height != resize->height)
    {
      resize->height = new_height;
      sprintf (size, "%d", new_height);

      gtk_signal_handler_block_by_data (GTK_OBJECT (private->height_text), data);
      gtk_entry_set_text (GTK_ENTRY (private->height_text), size);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->height_text), data);

      if (resize->type == ResizeWidget)
	{
	  resize->off_y = resize_bound_off_y (resize, (resize->height - private->old_height) / 2);
	  sprintf (size, "%d", resize->off_y);
	  
	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_y_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->off_y_text), size);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_y_text), data);
	}
    }

  if (private->constrain && resize->height != 0)
    {
      private->constrain = FALSE;

      resize->ratio_x = resize->ratio_y;

      new_width = (int) (private->old_width * resize->ratio_x);
      if (new_width == 0) new_width = 1;

      if (new_width != resize->width)
	{
	  resize->width = new_width;

	  sprintf (size, "%d", resize->width);

	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->width_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->width_text), size);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->width_text), data);
	  
	  sprintf (ratio_text, "%0.4f", resize->ratio_x);  
	  
	  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_x_text), data);
	  gtk_entry_set_text (GTK_ENTRY (private->ratio_x_text), ratio_text);
	  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_x_text), data);

	  if (resize->type == ResizeWidget)
	    {
	      resize->off_x = resize_bound_off_x (resize, (resize->width - private->old_width) / 2);
	      sprintf (size, "%d", resize->off_x);

	      gtk_signal_handler_block_by_data (GTK_OBJECT (private->off_x_text), data);
	      gtk_entry_set_text (GTK_ENTRY (private->off_x_text), size);
	      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->off_x_text), data);
	    }
	}

      private->constrain = TRUE;
    }

  resize_draw (resize);
}

static gint
resize_events (GtkWidget *widget,
	       GdkEvent  *event)
{
  Resize *resize;
  ResizePrivate *private;
  int dx, dy;
  int off_x, off_y;
  char size[12];

  resize = (Resize *) gtk_object_get_user_data (GTK_OBJECT (widget));
  private = (ResizePrivate *) resize->private_part;

  switch (event->type)
    {
    case GDK_EXPOSE:
      resize_draw (resize);
      break;
    case GDK_BUTTON_PRESS:
      gdk_pointer_grab (private->drawing_area->window, FALSE,
			(GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON_RELEASE_MASK),
			NULL, NULL, event->button.time);
      private->orig_x = resize->off_x;
      private->orig_y = resize->off_y;
      private->start_x = event->button.x;
      private->start_y = event->button.y;
      break;
    case GDK_MOTION_NOTIFY:
      /*  X offset  */
      dx = event->motion.x - private->start_x;
      off_x = private->orig_x + dx / private->ratio;
      off_x = resize_bound_off_x (resize, off_x);
      sprintf (size, "%d", off_x);
      gtk_entry_set_text (GTK_ENTRY (private->off_x_text), size);

      /*  Y offset  */
      dy = event->motion.y - private->start_y;
      off_y = private->orig_y + dy / private->ratio;
      off_y = resize_bound_off_y (resize, off_y);
      sprintf (size, "%d", off_y);
      gtk_entry_set_text (GTK_ENTRY (private->off_y_text), size);
      break;
    case GDK_BUTTON_RELEASE:
      gdk_pointer_ungrab (event->button.time);
      break;
    default:
      break;
    }

  return FALSE;
}
