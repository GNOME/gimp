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
#include <stdlib.h>
#include "appenv.h"
#include "color_panel.h"
#include "color_notebook.h"
#include "colormaps.h"
#include "gimpdnd.h"

#define EVENT_MASK  GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | \
                    GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | \
                    GDK_LEAVE_NOTIFY_MASK

typedef struct _ColorPanelPrivate ColorPanelPrivate;

struct _ColorPanelPrivate
{
  GtkWidget *drawing_area;
  GdkGC     *gc;

  gboolean   button_down;

  ColorNotebookP  color_notebook;
  gboolean        color_notebook_active;
};

/*  local function prototypes  */
static void color_panel_draw            (ColorPanel *);
static gint color_panel_events          (GtkWidget *, GdkEvent *);
static void color_panel_select_callback (gint, gint, gint,
					 ColorNotebookState, void *);

static void color_panel_drag_begin  (GtkWidget *, GdkDragContext *, gpointer);
static void color_panel_drag_end    (GtkWidget *, GdkDragContext *, gpointer);
static void color_panel_drop_handle (GtkWidget        *widget, 
				     GdkDragContext   *context,
				     gint              x,
				     gint              y,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint             time,
				     gpointer          data);
static void color_panel_drag_handle (GtkWidget        *widget, 
				     GdkDragContext   *context,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint             time,
				     gpointer          data);

/*  dnd structures  */
static GtkTargetEntry color_panel_target_table[] =
{
  GIMP_TARGET_COLOR
};
static guint n_color_panel_targets = (sizeof (color_panel_target_table) /
				      sizeof (color_panel_target_table[0]));

ColorPanel *
color_panel_new (guchar *initial,
		 gint    width,
		 gint    height)
{
  ColorPanel *color_panel;
  ColorPanelPrivate *private;
  gint i;

  private = g_new (ColorPanelPrivate, 1);
  private->color_notebook        = NULL;
  private->color_notebook_active = FALSE;
  private->gc                    = NULL;
  private->button_down           = FALSE;

  color_panel = g_new (ColorPanel, 1);
  color_panel->private_part = private;

  /*  set the initial color  */
  for (i = 0; i < 3; i++)
    color_panel->color[i] = (initial) ? initial[i] : 0;

  color_panel->color_panel_widget = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (color_panel->color_panel_widget),
			     GTK_SHADOW_IN);

  /*  drawing area  */
  private->drawing_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (private->drawing_area),
			 width, height);
  gtk_widget_set_events (private->drawing_area, EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (private->drawing_area), "event",
		      (GtkSignalFunc) color_panel_events,
		      color_panel);
  gtk_object_set_user_data (GTK_OBJECT (private->drawing_area), color_panel);
  gtk_container_add (GTK_CONTAINER (color_panel->color_panel_widget),
		     private->drawing_area);
  gtk_widget_show (private->drawing_area);

  /*  dnd stuff  */
  gtk_drag_dest_set (private->drawing_area,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     color_panel_target_table, n_color_panel_targets,
		     GDK_ACTION_COPY);

  gtk_drag_source_set (private->drawing_area,
                       GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
                       color_panel_target_table, n_color_panel_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gtk_signal_connect (GTK_OBJECT (private->drawing_area),
                      "drag_begin",
                      GTK_SIGNAL_FUNC (color_panel_drag_begin),
                      color_panel);
  gtk_signal_connect (GTK_OBJECT (private->drawing_area),
                      "drag_end",
                      GTK_SIGNAL_FUNC (color_panel_drag_end),
                      color_panel);
  gtk_signal_connect (GTK_OBJECT (private->drawing_area),
                      "drag_data_get",
                      GTK_SIGNAL_FUNC (color_panel_drag_handle),
                      color_panel);
  gtk_signal_connect (GTK_OBJECT (private->drawing_area),
                      "drag_data_received",
                      GTK_SIGNAL_FUNC (color_panel_drop_handle),
                      color_panel);

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
	private->button_down = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1 &&
	  private->button_down)
	{
	  if (! private->color_notebook)
	    {
	      private->color_notebook =
		color_notebook_new (color_panel->color[0],
				    color_panel->color[1],
				    color_panel->color[2],
				    color_panel_select_callback,
				    color_panel,
				    FALSE);
	      private->color_notebook_active = TRUE;
	    }
	  else
	    {
	      if (! private->color_notebook_active)
		{
		  color_notebook_show (private->color_notebook);
		  private->color_notebook_active = TRUE;
		}
	      color_notebook_set_color (private->color_notebook,
					color_panel->color[0],
					color_panel->color[1],
					color_panel->color[2], 1);
	    }
	  private->button_down = FALSE;
	}
      break;

    case GDK_LEAVE_NOTIFY:
      private->button_down = FALSE;
      break;

    default:
      break;
    }

  return FALSE;
}

static void
color_panel_select_callback (gint                r,
			     gint                g,
			     gint                b,
			     ColorNotebookState  state,
			     void               *client_data)
{
  ColorPanel *color_panel;
  ColorPanelPrivate *private;

  color_panel = (ColorPanel *) client_data;
  private = (ColorPanelPrivate *) color_panel->private_part;

  if (private->color_notebook)
    {
      switch (state)
	{
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
	  private->color_notebook_active = FALSE;
	}
    }
}

static void
color_panel_drag_begin (GtkWidget      *widget,
			GdkDragContext *context,
			gpointer        data)
{
  ColorPanel *color_panel = data;
  GtkWidget *window;
  GdkColor bg;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  gtk_widget_set_usize (window, 48, 32);
  gtk_widget_realize (window);
  gtk_object_set_data_full (GTK_OBJECT (widget),
                            "gimp-color-panel-drag-window",
                            window,
                            (GtkDestroyNotify) gtk_widget_destroy);

  bg.red   = 0xff * color_panel->color[0];
  bg.green = 0xff * color_panel->color[1];
  bg.blue  = 0xff * color_panel->color[2];

  gdk_color_alloc (gtk_widget_get_colormap (window), &bg);
  gdk_window_set_background (window->window, &bg);

  gtk_drag_set_icon_widget (context, window, -2, -2);
}

static void
color_panel_drag_end (GtkWidget      *widget,
		      GdkDragContext *context,
		      gpointer        data)
{
  gtk_object_set_data (GTK_OBJECT (widget),
		       "gimp-color-panel-drag-window", NULL);
}

static void
color_panel_drop_handle (GtkWidget        *widget, 
			 GdkDragContext   *context,
			 gint              x,
			 gint              y,
			 GtkSelectionData *selection_data,
			 guint             info,
			 guint             time,
			 gpointer          data)
{
  ColorPanel *color_panel = data;
  ColorPanelPrivate *private = (ColorPanelPrivate *) color_panel->private_part;
  guint16 *vals;

  if (selection_data->length < 0)
    return;

  if ((selection_data->format != 16) || 
      (selection_data->length != 8))
    {
      g_warning ("Received invalid color data\n");
      return;
    }
  
  vals = (guint16 *) selection_data->data;

  color_panel->color[0] = vals[0] / 0xff;
  color_panel->color[1] = vals[1] / 0xff;
  color_panel->color[2] = vals[2] / 0xff;

  if (private->color_notebook_active)
    color_notebook_set_color (private->color_notebook,
			      color_panel->color[0],
			      color_panel->color[1],
			      color_panel->color[2],
			      TRUE);

  color_panel_draw (color_panel);
}

static void
color_panel_drag_handle (GtkWidget        *widget, 
			 GdkDragContext   *context,
			 GtkSelectionData *selection_data,
			 guint             info,
			 guint             time,
			 gpointer          data)
{
  ColorPanel *color_panel = data;
  guint16 vals[4];

  vals[0] = color_panel->color[0] * 0xff;
  vals[1] = color_panel->color[1] * 0xff;
  vals[2] = color_panel->color[2] * 0xff;
  vals[3] = 0xffff;

  gtk_selection_data_set (selection_data,
                          gdk_atom_intern ("application/x-color", FALSE),
                          16, (guchar *) vals, 8);
}
