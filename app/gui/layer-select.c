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
#include <gdk/gdkkeysyms.h>

#include "core/core-types.h"
#include "widgets/widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "widgets/gimppreview.h"

#include "layer-select.h"
#include "gdisplay.h"

#include "gimprc.h"
#include "temp_buf.h"

#include "libgimp/gimpintl.h"


#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK


typedef struct _LayerSelect LayerSelect;

struct _LayerSelect
{
  GtkWidget *shell;
  GtkWidget *preview;
  GtkWidget *label;

  GimpImage *gimage;
  GimpLayer *current_layer;
};


/*  layer widget function prototypes  */
static void   layer_select_advance    (LayerSelect *layer_select, 
				       gint         move);
static void   layer_select_forward    (LayerSelect *layer_select);
static void   layer_select_backward   (LayerSelect *layer_select);
static void   layer_select_end        (LayerSelect *layer_select, 
				       guint32      time);
static void   layer_select_set_gimage (LayerSelect *layer_select, 
				       GimpImage   *gimage);
static gint   layer_select_events     (GtkWidget   *widget, 
				       GdkEvent    *event);


/*
 *  Local variables
 */
LayerSelect *layer_select = NULL;


/**********************/
/*  Public functions  */
/**********************/


void
layer_select_init (GimpImage *gimage,
		   gint       move,
		   guint32    time)
{
  GimpLayer *layer;
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *hbox;
  GtkWidget *alignment;

  layer = gimp_image_get_active_layer (gimage);

  if (! layer)
    return;

  if (!layer_select)
    {
      layer_select = g_new0 (LayerSelect, 1);

      layer_select->preview      = gimp_preview_new (GIMP_VIEWABLE (layer),
						     preview_size, 1, FALSE);
      layer_select->label        = gtk_label_new (NULL);

      layer_select_set_gimage (layer_select, gimage);
      layer_select_advance (layer_select, move);

      /*  The shell and main vbox  */
      layer_select->shell = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_wmclass (GTK_WINDOW (layer_select->shell), 
			      "layer_select", "Gimp");
      gtk_window_set_title (GTK_WINDOW (layer_select->shell), 
			    _("Layer Select"));
      gtk_window_set_position (GTK_WINDOW (layer_select->shell), 
			       GTK_WIN_POS_MOUSE);
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

      gtk_container_add (GTK_CONTAINER (alignment), layer_select->preview);
      gtk_widget_show (layer_select->preview);
      gtk_widget_show (alignment);

      /*  the layer name label */
      gtk_box_pack_start (GTK_BOX (hbox), 
			  layer_select->label, FALSE, FALSE, 2);
      gtk_widget_show (layer_select->label);

      gtk_widget_show (hbox);
      gtk_widget_show (frame1);
      gtk_widget_show (frame2);
      gtk_widget_show (layer_select->shell);
    }
  else
    {
      layer_select_set_gimage (layer_select, gimage);
      layer_select_advance (layer_select, move);

      if (! GTK_WIDGET_VISIBLE (layer_select->shell))
	gtk_widget_show (layer_select->shell);
    }

  gdk_key_repeat_disable ();
  gdk_keyboard_grab (layer_select->shell->window, FALSE, time);
}

void
layer_select_update_preview_size (void)
{
  if (layer_select != NULL)
    {
      gimp_preview_set_size (GIMP_PREVIEW (layer_select->preview), 
			     preview_size, 1);
    }  
}


/***********************/
/*  Private functions  */
/***********************/


static void
layer_select_advance (LayerSelect *layer_select,
		      gint         move)
{
  gint       index;
  gint       count;
  GSList    *list;
  GSList    *nth;
  GimpLayer *layer;

  index = 0;

  if (move == 0)
    return;

  /*  If there is a floating selection, allow no advancement  */
  if (gimp_image_floating_sel (layer_select->gimage))
    return;

  for (list = layer_select->gimage->layer_stack, count = 0; 
       list; 
       list = g_slist_next (list), count++)
    {
      layer = (GimpLayer *) list->data;

      if (layer == layer_select->current_layer)
	index = count;
    }

  index += move;
  index = CLAMP (index, 0, count - 1);

  nth = g_slist_nth (layer_select->gimage->layer_stack, index);

  if (nth)
    {
      layer = (GimpLayer *) nth->data;
      layer_select->current_layer = layer;

      gimp_preview_set_viewable (GIMP_PREVIEW (layer_select->preview),
				 GIMP_VIEWABLE (layer_select->current_layer));

      gtk_label_set_text (GTK_LABEL (layer_select->label),
			  GIMP_OBJECT (layer_select->current_layer)->name);
    }
}

static void
layer_select_forward (LayerSelect *layer_select)
{
  layer_select_advance (layer_select, 1);
}

static void
layer_select_backward (LayerSelect *layer_select)
{
  layer_select_advance (layer_select, -1);
}

static void
layer_select_end (LayerSelect *layer_select,
		  guint32      time)
{
  gdk_key_repeat_restore ();
  gdk_keyboard_ungrab (time);

  gtk_widget_hide (layer_select->shell);

  /*  only reset the active layer if a new layer was specified  */
  if (layer_select->current_layer !=
      gimp_image_get_active_layer (layer_select->gimage))
    {
      gimp_image_set_active_layer (layer_select->gimage, 
				   layer_select->current_layer);
      gdisplays_flush ();
    }
}

static void
layer_select_set_gimage (LayerSelect *layer_select,
			 GimpImage   *gimage)
{
  layer_select->gimage        = gimage;
  layer_select->current_layer = gimp_image_get_active_layer (gimage);

  gimp_preview_set_viewable (GIMP_PREVIEW (layer_select->preview),
			     GIMP_VIEWABLE (layer_select->current_layer));
  gimp_preview_set_size (GIMP_PREVIEW (layer_select->preview),
			 preview_size, 1);
  gtk_label_set_text (GTK_LABEL (layer_select->label),
		      GIMP_OBJECT (layer_select->current_layer)->name);
}

static gint
layer_select_events (GtkWidget *widget,
		     GdkEvent  *event)
{
  GdkEventKey    *kevent;
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
