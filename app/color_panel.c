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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "color_panel.h"
#include "color_notebook.h"
#include "colormaps.h"

#define EVENT_MASK  GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK

typedef struct _ColorPanelPrivate ColorPanelPrivate;

struct _ColorPanelPrivate
{
  GtkWidget *drawing_area;
  GdkGC *gc;

  ColorNotebookP color_notebook;
  int color_notebook_active;
};

static void color_panel_draw (ColorPanel *);
static gint color_panel_events (GtkWidget *area, GdkEvent *event);
static void color_panel_select_callback (int, int, int, ColorNotebookState, void *);


ColorPanel *
color_panel_new (unsigned char *initial,
		 int            width,
		 int            height)
{
  ColorPanel *color_panel;
  ColorPanelPrivate *private;
  int i;

  color_panel = g_new (ColorPanel, 1);
  private = g_new (ColorPanelPrivate, 1);
  private->color_notebook = NULL;
  private->color_notebook_active = 0;
  private->gc = NULL;
  color_panel->private_part = private;

  /*  set the initial color  */
  for (i = 0; i < 3; i++)
    color_panel->color[i] = (initial) ? initial[i] : 0;

  color_panel->color_panel_widget = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (color_panel->color_panel_widget), GTK_SHADOW_IN);

  /*  drawing area  */
  private->drawing_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (private->drawing_area), width, height);
  gtk_widget_set_events (private->drawing_area, EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (private->drawing_area), "event",
		      (GtkSignalFunc) color_panel_events,
		      color_panel);
  gtk_object_set_user_data (GTK_OBJECT (private->drawing_area), color_panel);
  gtk_container_add (GTK_CONTAINER (color_panel->color_panel_widget), private->drawing_area);
  gtk_widget_show (private->drawing_area);

  return color_panel;
}

void
color_panel_free (ColorPanel *color_panel)
{
  ColorPanelPrivate *private;

  private = (ColorPanelPrivate *) color_panel->private_part;
  
  /* make sure we hide and free color_notebook */
  if (private->color_notebook)
    {
      color_notebook_hide (private->color_notebook);
      color_notebook_free (private->color_notebook);
    }
  
  if (private->gc)
    gdk_gc_destroy (private->gc);
  g_free (color_panel->private_part);
  g_free (color_panel);
}

static void
color_panel_draw (ColorPanel *color_panel)
{
  GtkWidget *widget;
  ColorPanelPrivate *private;
  GdkColor fg;

  private = (ColorPanelPrivate *) color_panel->private_part;
  widget = private->drawing_area;

  fg.pixel = old_color_pixel;
  store_color (&fg.pixel,
	       color_panel->color[0],
	       color_panel->color[1],
	       color_panel->color[2]);

  gdk_gc_set_foreground (private->gc, &fg);
  gdk_draw_rectangle (widget->window, private->gc, 1, 0, 0,
		      widget->allocation.width, widget->allocation.height);
}

static gint
color_panel_events (GtkWidget *widget,
		    GdkEvent  *event)
{
  GdkEventButton *bevent;
  ColorPanel *color_panel;
  ColorPanelPrivate *private;

  color_panel = (ColorPanel *) gtk_object_get_user_data (GTK_OBJECT (widget));
  private = (ColorPanelPrivate *) color_panel->private_part;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!private->gc)
	private->gc = gdk_gc_new (widget->window);

      color_panel_draw (color_panel);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  if (! private->color_notebook)
	    {
	      private->color_notebook = color_notebook_new (color_panel->color[0],
							color_panel->color[1],
							color_panel->color[2],
							color_panel_select_callback,
							color_panel,
							FALSE);
	      private->color_notebook_active = 1;
	    }
	  else
	    {
	      if (! private->color_notebook_active)
		color_notebook_show (private->color_notebook);
	      color_notebook_set_color (private->color_notebook,
					color_panel->color[0],
					color_panel->color[1],
					color_panel->color[2], 1);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
color_panel_select_callback (int   r,
			     int   g,
			     int   b,
			     ColorNotebookState state,
			     void *client_data)
{
  ColorPanel *color_panel;
  ColorPanelPrivate *private;

  color_panel = (ColorPanel *) client_data;
  private = (ColorPanelPrivate *) color_panel->private_part;

  if (private->color_notebook)
    {
      switch (state) {
      case COLOR_NOTEBOOK_UPDATE:
	break;
      case COLOR_NOTEBOOK_OK:
	color_panel->color[0] = r;
	color_panel->color[1] = g;
	color_panel->color[2] = b;
	
	color_panel_draw (color_panel);
	/* Fallthrough */
      case COLOR_NOTEBOOK_CANCEL:
	color_notebook_hide (private->color_notebook);
	private->color_notebook_active = 0;
      }
    }
}
