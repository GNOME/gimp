/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPatternPreview Widget
 * Copyright (C) 1999 Sven Neumann
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

#include "gimppatternpreview.h"

#define PATTERN_PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK |   \
                                     GDK_BUTTON_RELEASE_MASK | \
                                     GDK_ENTER_NOTIFY_MASK |   \
                                     GDK_LEAVE_NOTIFY_MASK)

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint gimp_pattern_preview_signals[LAST_SIGNAL] = { 0 };
static GtkPreviewClass *parent_class = NULL;

static gint gimp_pattern_preview_button_press_event   (GtkWidget *, GdkEventButton *);
static gint gimp_pattern_preview_button_release_event (GtkWidget *, GdkEventButton *);
static void gimp_pattern_preview_popup_open  (GimpPatternPreview *, gint, gint);
static void gimp_pattern_preview_popup_close (GimpPatternPreview *);
static void gimp_pattern_preview_draw        (GimpPatternPreview *);


static void
gimp_pattern_preview_destroy (GtkObject *object)
{
  GimpPatternPreview *gpp;

  g_return_if_fail (GIMP_IS_PATTERN_PREVIEW (object));

  gpp = GIMP_PATTERN_PREVIEW (object);

  if (gpp->popup)
    gtk_widget_destroy (gpp->popup);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_pattern_preview_class_init (GimpPatternPreviewClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_preview_get_type ());

  gimp_pattern_preview_signals[CLICKED] = 
    gtk_signal_new ("clicked",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpPatternPreviewClass, clicked),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, gimp_pattern_preview_signals, 
				LAST_SIGNAL);

  class->clicked = NULL;

  widget_class->button_press_event = gimp_pattern_preview_button_press_event;
  widget_class->button_release_event = gimp_pattern_preview_button_release_event;

  object_class->destroy = gimp_pattern_preview_destroy;
}

static void
gimp_pattern_preview_init (GimpPatternPreview *gpp)
{
  gpp->pattern = NULL;
  gpp->width = 0;
  gpp->height = 0;
  gpp->tooltips = NULL;
  gpp->popup = NULL;
  gpp->popup_preview = NULL;
  GTK_PREVIEW (gpp)->type = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (gpp)->bpp = 3;
  GTK_PREVIEW (gpp)->dither = GDK_RGB_DITHER_NORMAL;

  gtk_widget_set_events (GTK_WIDGET (gpp), PATTERN_PREVIEW_EVENT_MASK);
}

GtkType
gimp_pattern_preview_get_type ()
{
  static GtkType gpp_type = 0;

  if (!gpp_type)
    {
      GtkTypeInfo gpp_info =
      {
	"GimpPatternPreview",
	sizeof (GimpPatternPreview),
	sizeof (GimpPatternPreviewClass),
	(GtkClassInitFunc) gimp_pattern_preview_class_init,
	(GtkObjectInitFunc) gimp_pattern_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gpp_type = gtk_type_unique (gtk_preview_get_type (), &gpp_info);
    }
  
  return gpp_type;
}

GtkWidget*
gimp_pattern_preview_new (gint width,
			  gint height)
{
  GimpPatternPreview *gpp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);

  gpp = gtk_type_new (gimp_pattern_preview_get_type ());

  gpp->width  = width;
  gpp->height = height;
  gtk_preview_size (GTK_PREVIEW (gpp), width, height);

  return GTK_WIDGET (gpp);
}

void
gimp_pattern_preview_update (GimpPatternPreview *gpp,
			     GPattern           *pattern)
{
  g_return_if_fail (GIMP_IS_PATTERN_PREVIEW (gpp));
  g_return_if_fail (pattern != NULL);
      
  if (gpp->pattern == pattern)
    return;
  gpp->pattern = pattern;
  gimp_pattern_preview_draw (gpp);
  if (gpp->tooltips)
    gtk_tooltips_set_tip (gpp->tooltips, GTK_WIDGET (gpp), gpp->pattern->name, NULL);
}

void
gimp_pattern_preview_set_tooltips (GimpPatternPreview *gpp,
				   GtkTooltips        *tooltips)
{
  g_return_if_fail (GIMP_IS_PATTERN_PREVIEW (gpp));
  g_return_if_fail (GTK_IS_TOOLTIPS (tooltips));
      
  gpp->tooltips = tooltips;
  gtk_signal_connect (GTK_OBJECT (gpp->tooltips), "destroy", 
		      gtk_widget_destroyed, &gpp->tooltips);
  if (gpp->pattern)
    gtk_tooltips_set_tip (gpp->tooltips, GTK_WIDGET (gpp), gpp->pattern->name, NULL);
}

static gint
gimp_pattern_preview_button_press_event (GtkWidget      *widget,
					 GdkEventButton *bevent)
{
  if (bevent->button == 1)
    {
      gtk_signal_emit_by_name (GTK_OBJECT (widget), "clicked");
      gimp_pattern_preview_popup_open (GIMP_PATTERN_PREVIEW (widget),
				       bevent->x, bevent->y);
    }
  return TRUE;
}
  
static gint
gimp_pattern_preview_button_release_event (GtkWidget      *widget,
					   GdkEventButton *bevent)
{
  if (bevent->button == 1)
    gimp_pattern_preview_popup_close (GIMP_PATTERN_PREVIEW (widget));

  return TRUE;
}

static void
gimp_pattern_preview_popup_open (GimpPatternPreview *gpp,
				 gint                x,
				 gint                y)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  guchar *mask;
  gint width, height;

  width = gpp->pattern->mask->width;
  height = gpp->pattern->mask->height;
  
  if (width <= gpp->width && height <= gpp->height)
    return;

  /* make sure the popup exists and is not visible */
  if (gpp->popup == NULL)
    {
      GtkWidget *frame;

      gpp->popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (gpp->popup), FALSE, FALSE, TRUE);
      gtk_signal_connect (GTK_OBJECT (gpp->popup), "destroy", 
			  gtk_widget_destroyed, &gpp->popup);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (gpp->popup), frame);
      gtk_widget_show (frame);
      gpp->popup_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_signal_connect (GTK_OBJECT (gpp->popup_preview), "destroy", 
			  gtk_widget_destroyed, &gpp->popup_preview);
      gtk_container_add (GTK_CONTAINER (frame), gpp->popup_preview);
      gtk_widget_show (gpp->popup_preview);
    }
  else
    {
      gtk_widget_hide (gpp->popup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (GTK_WIDGET (gpp)->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (width >> 1);
  y = y_org + y - (height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + width > scr_w) ? scr_w - width : x;
  y = (y + height > scr_h) ? scr_h - height : y;
  gtk_preview_size (GTK_PREVIEW (gpp->popup_preview), width, height);
  gtk_widget_popup (gpp->popup, x, y);
  
  /*  Draw the pattern  */
  mask = temp_buf_data (gpp->pattern->mask);

  if (gpp->pattern->mask->bytes == 1)
    {
      guchar *buf = g_new (guchar, 3 * width);
      guchar *b;
	
      for (y = 0; y < height; y++)
	{
	  b = buf;
	  for (x = 0; x < width ; x++)
	    {
	      memset (b, *mask++, 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (gpp->popup_preview), buf, 0, y, width); 
	}
      g_free(buf);
   }
  else
    {
      for (y = 0; y < height; y++)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (gpp->popup_preview), mask, 0, y, width);
	  mask += 3 * width;
	}
    }
  
  gtk_widget_queue_draw (gpp->popup_preview);
}

static void
gimp_pattern_preview_popup_close (GimpPatternPreview *gpp)
{
  if (gpp->popup != NULL)
    gtk_widget_hide (gpp->popup);
}

static void
gimp_pattern_preview_draw (GimpPatternPreview *gpp)
{
  gint pattern_width, pattern_height;
  gint width, height;
  gint offset_x, offset_y;
  guchar *mask, *buf, *b;
  gint x, y;

  g_return_if_fail (gpp->pattern != NULL);
 
  pattern_width = gpp->pattern->mask->width;
  pattern_height = gpp->pattern->mask->height;

  width = (pattern_width > gpp->width) ?  gpp->width : pattern_width;
  height = (pattern_height > gpp->height) ? gpp->height: pattern_height;

  offset_x = (gpp->width - width) >> 1;
  offset_y = (gpp->height - height) >> 1;
  
  mask = temp_buf_data (gpp->pattern->mask);
  buf = g_new (guchar, 3 * gpp->width);

  memset (buf, 255, 3 * gpp->width);
  for (y = 0; y < offset_y; y++)
    gtk_preview_draw_row (GTK_PREVIEW (gpp), buf, 0, y, gpp->width); 

  if (gpp->pattern->mask->bytes == 1) 
    {
      for (y = offset_y; y < height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < width ; x++)
	    {
	      memset (b, mask[x], 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (gpp), buf, 0, y, gpp->width);
	  mask += pattern_width;
	}
    }
  else
    {
      for (y = offset_y; y < height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  memcpy (b, mask, 3 * width);
	  gtk_preview_draw_row (GTK_PREVIEW (gpp), buf, 0, y, gpp->width); 
	  mask += 3 * pattern_width;
	}
    }

  memset (buf, 255, 3 * gpp->width);
  for (y = height + offset_y; y < gpp->height; y++)
    gtk_preview_draw_row (GTK_PREVIEW (gpp), buf, 0, y, gpp->width); 

  g_free(buf);
 
  gtk_widget_queue_draw (GTK_WIDGET (gpp));
}




