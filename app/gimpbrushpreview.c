/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpBrushPreview Widget
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

#include "gimpbrushpreview.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpipeP.h"
#include "brush_scale.h"
#include "temp_buf.h"


/*  the pixmap for the scale_indicator  */
#define scale_indicator_width 7
#define scale_indicator_height 7

#define WHT {255,255,255}
#define BLK {  0,  0,  0}

static unsigned char scale_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, BLK, BLK, BLK, BLK, BLK, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT }
};

#define BRUSH_PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK |   \
                                   GDK_BUTTON_RELEASE_MASK | \
                                   GDK_ENTER_NOTIFY_MASK |   \
                                   GDK_LEAVE_NOTIFY_MASK)

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint gimp_brush_preview_signals[LAST_SIGNAL] = { 0 };
static GtkPreviewClass *parent_class = NULL;

static gint gimp_brush_preview_button_press_event   (GtkWidget *, GdkEventButton *);
static gint gimp_brush_preview_button_release_event (GtkWidget *, GdkEventButton *);
static void gimp_brush_preview_popup_open  (GimpBrushPreview *, gint, gint);
static void gimp_brush_preview_popup_close (GimpBrushPreview *);
static void gimp_brush_preview_draw        (GimpBrushPreview *);


static void
gimp_brush_preview_destroy (GtkObject *object)
{
  GimpBrushPreview *gbp;

  g_return_if_fail (GIMP_IS_BRUSH_PREVIEW (object));

  gbp = GIMP_BRUSH_PREVIEW (object);

  if (gbp->popup)
    gtk_widget_destroy (gbp->popup);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_brush_preview_class_init (GimpBrushPreviewClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_preview_get_type ());

  gimp_brush_preview_signals[CLICKED] = 
    gtk_signal_new ("clicked",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpBrushPreviewClass, clicked),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, gimp_brush_preview_signals, 
				LAST_SIGNAL);

  class->clicked = NULL;

  widget_class->button_press_event = gimp_brush_preview_button_press_event;
  widget_class->button_release_event = gimp_brush_preview_button_release_event;

  object_class->destroy = gimp_brush_preview_destroy;
}

static void
gimp_brush_preview_init (GimpBrushPreview *gbp)
{
  gbp->brush = NULL;
  gbp->width = 0;
  gbp->height = 0;
  gbp->tooltips = NULL;
  gbp->popup = NULL;
  gbp->popup_preview = NULL;
  GTK_PREVIEW (gbp)->type = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (gbp)->bpp = 3;
  GTK_PREVIEW (gbp)->dither = GDK_RGB_DITHER_NORMAL;

  gtk_widget_set_events (GTK_WIDGET (gbp), BRUSH_PREVIEW_EVENT_MASK);
}

GtkType
gimp_brush_preview_get_type ()
{
  static GtkType gbp_type = 0;

  if (!gbp_type)
    {
      GtkTypeInfo gbp_info =
      {
	"GimpBrushPreview",
	sizeof (GimpBrushPreview),
	sizeof (GimpBrushPreviewClass),
	(GtkClassInitFunc) gimp_brush_preview_class_init,
	(GtkObjectInitFunc) gimp_brush_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gbp_type = gtk_type_unique (gtk_preview_get_type (), &gbp_info);
    }
  
  return gbp_type;
}

GtkWidget*
gimp_brush_preview_new (gint width,
			gint height)
{
  GimpBrushPreview *gbp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);

  gbp = gtk_type_new (gimp_brush_preview_get_type ());

  gbp->width  = width;
  gbp->height = height;
  gtk_preview_size (GTK_PREVIEW (gbp), width, height);

  return GTK_WIDGET (gbp);
}

void
gimp_brush_preview_update (GimpBrushPreview *gbp,
			   GimpBrush        *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH_PREVIEW (gbp));
  g_return_if_fail (GIMP_IS_BRUSH (brush));
      
  if (gbp->brush == brush)
    return;
  gbp->brush = brush;
  gtk_signal_connect (GTK_OBJECT (gbp->brush), "destroy", 
		      gtk_widget_destroyed, &gbp->brush);
  gimp_brush_preview_draw (gbp);
  if (gbp->tooltips)
    gtk_tooltips_set_tip (gbp->tooltips, GTK_WIDGET (gbp), gbp->brush->name, NULL);
}

void
gimp_brush_preview_set_tooltips (GimpBrushPreview *gbp,
				 GtkTooltips      *tooltips)
{
  g_return_if_fail (GIMP_IS_BRUSH_PREVIEW (gbp));
  g_return_if_fail (GTK_IS_TOOLTIPS (tooltips));
      
  gbp->tooltips = tooltips;
  gtk_signal_connect (GTK_OBJECT (gbp->tooltips), "destroy", 
		      gtk_widget_destroyed, &gbp->tooltips);
  if (gbp->brush)
     gtk_tooltips_set_tip (gbp->tooltips, GTK_WIDGET (gbp), gbp->brush->name, NULL);
}

static gint
gimp_brush_preview_button_press_event (GtkWidget      *widget,
				       GdkEventButton *bevent)
{
  if (bevent->button == 1)
    {
      gtk_signal_emit_by_name (GTK_OBJECT (widget), "clicked");
      gimp_brush_preview_popup_open (GIMP_BRUSH_PREVIEW (widget), 
				     bevent->x, bevent->y);
    }
  return TRUE;
}
  
static gint
gimp_brush_preview_button_release_event (GtkWidget      *widget,
					 GdkEventButton *bevent)
{
  if (bevent->button == 1)
    gimp_brush_preview_popup_close (GIMP_BRUSH_PREVIEW (widget));

  return TRUE;
}

static void
gimp_brush_preview_popup_open (GimpBrushPreview *gbp,
			       gint              x,
			       gint              y)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  guchar *mask, *buf, *b;
  gint width, height;
  guchar bg;

  g_return_if_fail (gbp != NULL);
  if (!gbp->brush)
    return;

  width = gbp->brush->mask->width;
  height = gbp->brush->mask->height;
  
  if (width <= gbp->width && height <= gbp->height)
    return;

  /* make sure the popup exists and is not visible */
  if (gbp->popup == NULL)
    {
      GtkWidget *frame;

      gbp->popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (gbp->popup), FALSE, FALSE, TRUE);
      gtk_signal_connect (GTK_OBJECT (gbp->popup), "destroy", 
			  gtk_widget_destroyed, &gbp->popup);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (gbp->popup), frame);
      gtk_widget_show (frame);
      gbp->popup_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_signal_connect (GTK_OBJECT (gbp->popup_preview), "destroy", 
			  gtk_widget_destroyed, &gbp->popup_preview);
      gtk_container_add (GTK_CONTAINER (frame), gbp->popup_preview);
      gtk_widget_show (gbp->popup_preview);
    }
  else
    {
      gtk_widget_hide (gbp->popup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (GTK_WIDGET (gbp)->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (width >> 1);
  y = y_org + y - (height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + width > scr_w) ? scr_w - width : x;
  y = (y + height > scr_h) ? scr_h - height : y;
  gtk_preview_size (GTK_PREVIEW (gbp->popup_preview), width, height);
  gtk_widget_popup (gbp->popup, x, y);
  
  /*  Draw the brush  */
  buf = g_new (guchar, 3 * width);
  mask = temp_buf_data (gbp->brush->mask);

  if (GIMP_IS_BRUSH_PIXMAP (gbp->brush))
    {
      guchar *pixmap = temp_buf_data (GIMP_BRUSH_PIXMAP (gbp->brush)->pixmap_mask);
     
      for (y = 0; y < height; y++)
	{
	  b = buf;
	  for (x = 0; x < width ; x++)
	    {
	      bg = (255 - *mask);
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      *b++ = bg + (*mask * *pixmap++) / 255; 
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      mask++;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (gbp->popup_preview), buf, 0, y, width); 
	}
    }
  else
    {
      for (y = 0; y < height; y++)
	{
	  b = buf;
	  /*  Invert the mask for display.  */ 
	  for (x = 0; x < width; x++)
	    { 
	      bg = 255 - *mask++;
	      memset (b, bg, 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (gbp->popup_preview), buf, 0, y, width);
	}
    }
  g_free(buf);
  
  gtk_widget_queue_draw (gbp->popup_preview);
}

static void
gimp_brush_preview_popup_close (GimpBrushPreview *gbp)
{
  if (gbp->popup != NULL)
    gtk_widget_hide (gbp->popup);
}

static void
gimp_brush_preview_draw (GimpBrushPreview *gbp)
{
  gboolean scale = FALSE;
  gint brush_width, brush_height;
  gint offset_x, offset_y;
  TempBuf *mask_buf, *pixmap_buf = NULL;
  guchar *mask, *buf, *b;
  guchar bg;
  gint x, y;

  g_return_if_fail (gbp != NULL && gbp->brush != NULL);
 
  brush_width = gbp->brush->mask->width;
  brush_height = gbp->brush->mask->height;

  if (brush_width > gbp->width || brush_height > gbp->height)
    {
      gdouble ratio_x = (gdouble)brush_width / gbp->width;
      gdouble ratio_y = (gdouble)brush_height / gbp->height;
      
      brush_width =  (gdouble)brush_width / MAX (ratio_x, ratio_y); 
      brush_height = (gdouble)brush_height / MAX (ratio_x, ratio_y);
      
      mask_buf = brush_scale_mask (gbp->brush->mask, brush_width, brush_height);
      if (GIMP_IS_BRUSH_PIXMAP (gbp->brush))
	{
	  /*  TODO: the scale function should scale the pixmap 
	      and the mask in one run                            */
	  pixmap_buf = brush_scale_pixmap (GIMP_BRUSH_PIXMAP(gbp->brush)->pixmap_mask, 
					   brush_width, brush_height);
	}
      scale = TRUE;
    }
  else
    {
       mask_buf = gbp->brush->mask;
       if (GIMP_IS_BRUSH_PIXMAP (gbp->brush))
	 pixmap_buf = GIMP_BRUSH_PIXMAP(gbp->brush)->pixmap_mask;
    }

  offset_x = (gbp->width - brush_width) >> 1;
  offset_y = (gbp->height - brush_height) >> 1;
  
  mask = temp_buf_data (mask_buf);
  buf = g_new (guchar, 3 * gbp->width);
  memset (buf, 255, 3 * gbp->width);

  if (GIMP_IS_BRUSH_PIXMAP (gbp->brush)) 
    {
      guchar *pixmap = temp_buf_data (pixmap_buf);

      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gbp), buf, 0, y, gbp->width); 
      for (y = offset_y; y < brush_height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < brush_width ; x++)
	    {
	      bg = (255 - *mask);
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      *b++ = bg + (*mask * *pixmap++) / 255; 
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      mask++;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (gbp), buf, 0, y, gbp->width); 
	}
      memset (buf, 255, 3 * gbp->width);
      for (y = brush_height + offset_y; y < gbp->height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gbp), buf, 0, y, gbp->width); 
    }
  else
    {
      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gbp), buf, 0, y, gbp->width); 
      for (y = offset_y; y < brush_height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < brush_width ; x++)
	    {
	      bg = 255 - *mask++;
	      memset (b, bg, 3);
	      b += 3;
	    }   
	  gtk_preview_draw_row (GTK_PREVIEW (gbp), buf, 0, y, gbp->width); 
	}
      memset (buf, 255, 3 * gbp->width);
      for (y = brush_height + offset_y; y < gbp->height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gbp), buf, 0, y, gbp->width);    
    }

  if (scale)
    {
      offset_x = gbp->width - scale_indicator_width - 1;
      offset_y = gbp->height - scale_indicator_height - 1;
      for (y = 0; y < scale_indicator_height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gbp), scale_indicator_bits[y][0],
			      offset_x, offset_y + y, scale_indicator_width);
      temp_buf_free (mask_buf);
      if (GIMP_IS_BRUSH_PIXMAP (gbp->brush))
	temp_buf_free (pixmap_buf);
    }


  g_free(buf);
 
  gtk_widget_queue_draw (GTK_WIDGET (gbp));
}




