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
#include <stdlib.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "colormaps.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "interface.h"
#include "layer_select.h"
#include "layers_dialogP.h"

#include "libgimp/gimpintl.h"


#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK

typedef struct _LayerSelect LayerSelect;

struct _LayerSelect {
  GtkWidget *shell;
  GtkWidget *layer_preview;
  GtkWidget *label;
  GdkPixmap *layer_pixmap;
  GtkWidget *preview;

  GImage *gimage;
  Layer *current_layer;
  int dirty;
  int image_width, image_height;
  double ratio;
};

/*  layer widget function prototypes  */
static void layer_select_advance (LayerSelect *, int);
static void layer_select_forward (LayerSelect *);
static void layer_select_backward (LayerSelect *);
static void layer_select_end (LayerSelect *, guint32);
static void layer_select_set_gimage (LayerSelect *, GImage *);
static void layer_select_set_layer (LayerSelect *);
static gint layer_select_events (GtkWidget *, GdkEvent *);
static gint preview_events (GtkWidget *, GdkEvent *);
static void preview_redraw (LayerSelect *);

/*
 *  Local variables
 */
LayerSelect *layer_select = NULL;


/**********************/
/*  Public functions  */
/**********************/


void
layer_select_init (GImage  *gimage,
		   int      dir,
		   guint32  time)
{
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *hbox;
  GtkWidget *alignment;

  if (!layer_select)
    {
      layer_select = g_malloc (sizeof (LayerSelect));
      layer_select->layer_pixmap = NULL;
      layer_select->layer_preview = NULL;
      layer_select->preview = NULL;
      layer_select->image_width = layer_select->image_height = 0;
      layer_select_set_gimage (layer_select, gimage);
      layer_select_advance (layer_select, dir);

      if (preview_size)
	{
	  layer_select->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (layer_select->preview), preview_size, preview_size);
	}

      /*  The shell and main vbox  */
      layer_select->shell = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_wmclass (GTK_WINDOW (layer_select->shell), "layer_select", "Gimp");
      gtk_window_set_title (GTK_WINDOW (layer_select->shell), _("Layer Select"));
      gtk_window_set_position (GTK_WINDOW (layer_select->shell), GTK_WIN_POS_MOUSE);
      gtk_signal_connect (GTK_OBJECT (layer_select->shell), "event",
			  (GtkSignalFunc) layer_select_events,
			  layer_select);
      gtk_widget_set_events (layer_select->shell, (GDK_KEY_PRESS_MASK |
						   GDK_KEY_RELEASE_MASK |
						   GDK_BUTTON_PRESS_MASK));

      frame1 = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (layer_select->shell), frame1);
      frame2 = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (frame1), frame2);

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_add (GTK_CONTAINER (frame2), hbox);

      /*  The preview  */
      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);
      gtk_widget_show (alignment);

      layer_select->layer_preview = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (layer_select->layer_preview),
			     layer_select->image_width, layer_select->image_height);
      gtk_widget_set_events (layer_select->layer_preview, PREVIEW_EVENT_MASK);
      gtk_signal_connect (GTK_OBJECT (layer_select->layer_preview), "event",
			  (GtkSignalFunc) preview_events, layer_select);
      gtk_object_set_user_data (GTK_OBJECT (layer_select->layer_preview), layer_select);
      gtk_container_add (GTK_CONTAINER (alignment), layer_select->layer_preview);
      gtk_widget_show (layer_select->layer_preview);
      gtk_widget_show (alignment);

      /*  the layer name label */
      layer_select->label = gtk_label_new (_("Layer"));
      gtk_box_pack_start (GTK_BOX (hbox), layer_select->label, FALSE, FALSE, 2);
      gtk_widget_show (layer_select->label);

      gtk_widget_show (hbox);
      gtk_widget_show (frame1);
      gtk_widget_show (frame2);
      gtk_widget_show (layer_select->shell);
    }
  else
    {
      layer_select_set_gimage (layer_select, gimage);
      layer_select_advance (layer_select, dir);

      if (! GTK_WIDGET_VISIBLE (layer_select->shell))
	gtk_widget_show (layer_select->shell);
      else
	gtk_widget_draw (layer_select->layer_preview, NULL);
    }

  gdk_key_repeat_disable ();
  gdk_keyboard_grab (layer_select->shell->window, FALSE, time);
}

void
layer_select_update_preview_size ()
{
  if (layer_select != NULL)
    {
      gtk_preview_size (GTK_PREVIEW (layer_select->preview), preview_size, preview_size);
      if (GTK_WIDGET_VISIBLE (layer_select->shell))
        gtk_widget_draw (layer_select->layer_preview, NULL);
    }  
}


/***********************/
/*  Private functions  */
/***********************/

static void
layer_select_advance (LayerSelect *layer_select,
		      int          dir)
{
  int index;
  int length;
  int count;
  GSList *list;
  GSList *nth;
  Layer *layer;

  index = 0;

  /*  If there is a floating selection, allow no advancement  */
  if (gimage_floating_sel (layer_select->gimage))
    return;

  count = 0;
  list = layer_select->gimage->layer_stack;
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer == layer_select->current_layer)
	index = count;
      count++;
      list = g_slist_next (list);
    }

  length = g_slist_length (layer_select->gimage->layer_stack);

  if (dir == 1)
    index = (index == length - 1) ? 0 : (index + 1);
  else
    index = (index == 0) ? (length - 1) : (index - 1);

  nth = g_slist_nth (layer_select->gimage->layer_stack, index);

  if (nth)
    {
      layer = (Layer *) nth->data;
      layer_select->current_layer = layer;
    }
}


static void
layer_select_forward (LayerSelect *layer_select)
{
  layer_select_advance (layer_select, 1);
  layer_select->dirty = TRUE;
  gtk_widget_draw (layer_select->layer_preview, NULL);
}


static void
layer_select_backward (LayerSelect *layer_select)
{
  layer_select_advance (layer_select, -1);
  layer_select->dirty = TRUE;
  gtk_widget_draw (layer_select->layer_preview, NULL);
}


static void
layer_select_end (LayerSelect *layer_select,
		  guint32      time)
{
  gdk_key_repeat_restore ();
  gdk_keyboard_ungrab (time);

  gtk_widget_hide (layer_select->shell);

  /*  only reset the active layer if a new layer was specified  */
  if (layer_select->current_layer != layer_select->gimage->active_layer)
    {
      gimage_set_active_layer (layer_select->gimage, layer_select->current_layer);
      gdisplays_flush ();
    }
}


static void
layer_select_set_gimage (LayerSelect *layer_select,
			 GImage      *gimage)
{
  int image_width, image_height;

  layer_select->gimage = gimage;
  layer_select->current_layer = gimage->active_layer;
  layer_select->dirty = TRUE;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    layer_select->ratio = (double) preview_size / (double) gimage->width;
  else
    layer_select->ratio = (double) preview_size / (double) gimage->height;

  image_width = (int) (layer_select->ratio * gimage->width);
  image_height = (int) (layer_select->ratio * gimage->height);

  if (layer_select->image_width != image_width ||
      layer_select->image_height != image_height)
    {
      layer_select->image_width = image_width;
      layer_select->image_height = image_height;

      if (layer_select->layer_preview)
	gtk_widget_set_usize (layer_select->layer_preview,
			      layer_select->image_width,
			      layer_select->image_height);

      if (layer_select->layer_pixmap)
	{
	  gdk_pixmap_unref (layer_select->layer_pixmap);
	  layer_select->layer_pixmap = NULL;
	}
    }
}


static void
layer_select_set_layer (LayerSelect *layer_select)
{
  Layer *layer;

  if (! (layer =  (layer_select->current_layer)))
    return;

  /*  Set the layer label  */
  gtk_label_set_text (GTK_LABEL (layer_select->label), drawable_get_name (GIMP_DRAWABLE(layer)));
}


static gint
layer_select_events (GtkWidget *widget,
		     GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      layer_select_end (layer_select, bevent->time);
      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
	{
	case GDK_Tab:
	  if (kevent->state & GDK_MOD1_MASK)
	    layer_select_forward (layer_select);
	  else if (kevent->state & GDK_CONTROL_MASK)
	    layer_select_backward (layer_select);
	  break;
	}
      return TRUE;
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
	{
	case GDK_Alt_L: case GDK_Alt_R:
	  kevent->state &= ~GDK_MOD1_MASK;
	  break;
	case GDK_Control_L: case GDK_Control_R:
	  kevent->state &= ~GDK_CONTROL_MASK;
	  break;
	}

      if (! (kevent->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK)))
	layer_select_end (layer_select, kevent->time);

      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}


static gint
preview_events (GtkWidget *widget,
		GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      if (layer_select->dirty)
	{
	  /*  If a preview exists, draw it  */
	  if (preview_size)
	    preview_redraw (layer_select);

	  /*  Change the layer name label  */
	  layer_select_set_layer (layer_select);

	  layer_select->dirty = FALSE;
	}

      if (preview_size)
	gdk_draw_pixmap (layer_select->layer_preview->window,
			 layer_select->layer_preview->style->black_gc,
			 layer_select->layer_pixmap,
			 0, 0, 0, 0,
			 layer_select->image_width,
			 layer_select->image_height);
      break;

    default:
      break;
    }

  return FALSE;
}


static void
preview_redraw (LayerSelect *layer_select)
{
  Layer * layer;
  TempBuf * preview_buf;
  int w, h;
  int offx, offy;

  if (! (layer =  (layer_select->current_layer)))
    return;

  if (! layer_select->layer_pixmap)
    layer_select->layer_pixmap = gdk_pixmap_new (layer_select->layer_preview->window,
						 layer_select->image_width,
						 layer_select->image_height,
						 -1);

  if (layer_is_floating_sel (layer))
    render_fs_preview (layer_select->layer_preview, layer_select->layer_pixmap);
  else
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      /*  determine width and height  */
      w = (int) (layer_select->ratio * drawable_width (GIMP_DRAWABLE(layer)));
      h = (int) (layer_select->ratio * drawable_height (GIMP_DRAWABLE(layer)));
      offx = (int) (layer_select->ratio * off_x);
      offy = (int) (layer_select->ratio * off_y);

      preview_buf = layer_preview (layer, w, h);
      preview_buf->x = offx;
      preview_buf->y = offy;

      render_preview (preview_buf,
		      layer_select->preview,
		      layer_select->image_width,
		      layer_select->image_height,
		      -1);

      /*  Set the layer pixmap  */
      gtk_preview_put (GTK_PREVIEW (layer_select->preview),
		       layer_select->layer_pixmap,
		       layer_select->layer_preview->style->black_gc,
		       0, 0, 0, 0, layer_select->image_width, layer_select->image_height);

      /*  make sure the image has been transfered completely to the pixmap before
       *  we use it again...
       */
      gdk_flush ();
    }
}
