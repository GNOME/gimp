/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpContextPreview Widget
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

#include "brush_scale.h"
#include "gimpbrush.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpipeP.h"
#include "gimpcontextpreview.h"
#include "interface.h"  /* for tool_tips */
#include "patterns.h"
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

#define CONTEXT_PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK |   \
				     GDK_BUTTON_RELEASE_MASK | \
				     GDK_ENTER_NOTIFY_MASK |   \
				     GDK_LEAVE_NOTIFY_MASK)

static GtkWidget *gcp_popup = NULL;
static GtkWidget *gcp_popup_preview = NULL;

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint gimp_context_preview_signals[LAST_SIGNAL] = { 0 };
static GtkPreviewClass *parent_class = NULL;

static gint     gimp_context_preview_button_press_event   (GtkWidget *, 
							   GdkEventButton *);
static gint     gimp_context_preview_button_release_event (GtkWidget *, 
							   GdkEventButton *);
static void     gimp_context_preview_popup_open           (GimpContextPreview *, 
							   gint, gint);
static void     gimp_context_preview_popup_close          ();
static gboolean gimp_context_preview_data_matches_type    (GimpContextPreview *,
							   gpointer);
static void     gimp_context_preview_draw_brush           (GimpContextPreview *);
static void     gimp_context_preview_draw_brush_popup     (GimpContextPreview *);
static void     gimp_context_preview_draw_pattern         (GimpContextPreview *);
static void     gimp_context_preview_draw_pattern_popup   (GimpContextPreview *);


static void
gimp_context_preview_destroy (GtkObject *object)
{
  g_return_if_fail (GIMP_IS_CONTEXT_PREVIEW (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_context_preview_class_init (GimpContextPreviewClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_preview_get_type ());

  gimp_context_preview_signals[CLICKED] = 
    gtk_signal_new ("clicked",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpContextPreviewClass, clicked),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, gimp_context_preview_signals, 
				LAST_SIGNAL);

  class->clicked = NULL;

  widget_class->button_press_event = gimp_context_preview_button_press_event;
  widget_class->button_release_event = gimp_context_preview_button_release_event;

  object_class->destroy = gimp_context_preview_destroy;
}

static void
gimp_context_preview_init (GimpContextPreview *gcp)
{
  gcp->data = NULL;
  gcp->type = GCP_LAST;
  gcp->width = 0;
  gcp->height = 0;
  gcp->show_tooltips = FALSE;
  GTK_PREVIEW (gcp)->type = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (gcp)->bpp = 3;
  GTK_PREVIEW (gcp)->dither = GDK_RGB_DITHER_NORMAL;

  gtk_widget_set_events (GTK_WIDGET (gcp), CONTEXT_PREVIEW_EVENT_MASK);
}

GtkType
gimp_context_preview_get_type ()
{
  static GtkType gcp_type = 0;

  if (!gcp_type)
    {
      GtkTypeInfo gcp_info =
      {
	"GimpContextPreview",
	sizeof (GimpContextPreview),
	sizeof (GimpContextPreviewClass),
	(GtkClassInitFunc) gimp_context_preview_class_init,
	(GtkObjectInitFunc) gimp_context_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gcp_type = gtk_type_unique (gtk_preview_get_type (), &gcp_info);
    }
  
  return gcp_type;
}

GtkWidget*
gimp_context_preview_new (GimpContextPreviewType type,
			  gint                   width,
			  gint                   height,
			  gboolean               show_tooltips)
{
  GimpContextPreview *gcp;

  g_return_val_if_fail (type >= 0 && type < GCP_LAST, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  gcp = gtk_type_new (gimp_context_preview_get_type ());

  gcp->type          = type;
  gcp->width         = width;
  gcp->height        = height;
  gcp->show_tooltips = show_tooltips;

  gtk_preview_size (GTK_PREVIEW (gcp), width, height);

  return GTK_WIDGET (gcp);
}

void
gimp_context_preview_update (GimpContextPreview *gcp,
			     gpointer            data)
{
  g_return_if_fail (GIMP_IS_CONTEXT_PREVIEW (gcp));
  g_return_if_fail (gimp_context_preview_data_matches_type (gcp, data));

  gcp->data = data;
  if (GTK_IS_OBJECT (gcp->data))
    gtk_signal_connect (GTK_OBJECT (gcp->data), "destroy",
			gtk_widget_destroyed, &gcp->data);
  switch (gcp->type)
  {
  case GCP_BRUSH:
    gimp_context_preview_draw_brush (gcp);
    break;
  case GCP_PATTERN:
    gimp_context_preview_draw_pattern (gcp);
  default: 
    break;
  }

  if (gcp->show_tooltips)
  {
    gchar *name = NULL;

    switch (gcp->type)
    {
    case GCP_BRUSH:
      {
	GimpBrush *brush = GIMP_BRUSH (gcp->data);
	name = brush->name;
      }
      break;
    case GCP_PATTERN:
      {
	GPattern *pattern = (GPattern *)(gcp->data);
	name = pattern->name;
      }
      break;
    default:
      break;
    }
   gtk_tooltips_set_tip (tool_tips, GTK_WIDGET (gcp), name, NULL);
  }
}

static gint
gimp_context_preview_button_press_event (GtkWidget      *widget,
					 GdkEventButton *bevent)
{
  if (bevent->button == 1)
    {
      gtk_signal_emit_by_name (GTK_OBJECT (widget), "clicked");
      gimp_context_preview_popup_open (GIMP_CONTEXT_PREVIEW (widget), 
				       bevent->x, bevent->y);
    }
  return TRUE;
}
  
static gint
gimp_context_preview_button_release_event (GtkWidget      *widget,
					   GdkEventButton *bevent)
{
  if (bevent->button == 1)
    gimp_context_preview_popup_close ();

  return TRUE;
}

static void
gimp_context_preview_popup_open (GimpContextPreview *gcp,
				 gint                x,
				 gint                y)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gint width, height;

  g_return_if_fail (gcp != NULL);
  if (!gcp->data)
    return;

  switch (gcp->type)
    {
    case GCP_BRUSH:
      {
	GimpBrush *brush = GIMP_BRUSH (gcp->data);
	width  = brush->mask->width;
	height = brush->mask->height;
      }
      break;
    case GCP_PATTERN:
      {
	GPattern *pattern = (GPattern*)gcp->data;
	width  = pattern->mask->width;
	height = pattern->mask->height;
      }
      break;
    default:
      return;
    }

  if (width <= gcp->width && height <= gcp->height)
    return;

  /* make sure the popup exists and is not visible */
  if (gcp_popup == NULL)
    {
      GtkWidget *frame;

      gcp_popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (gcp_popup), FALSE, FALSE, TRUE);
      gtk_signal_connect (GTK_OBJECT (gcp_popup), "destroy", 
			  gtk_widget_destroyed, &gcp_popup);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (gcp_popup), frame);
      gtk_widget_show (frame);
      gcp_popup_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_signal_connect (GTK_OBJECT (gcp_popup_preview), "destroy", 
			  gtk_widget_destroyed, &gcp_popup_preview);
      gtk_container_add (GTK_CONTAINER (frame), gcp_popup_preview);
      gtk_widget_show (gcp_popup_preview);
    }
  else
    {
      gtk_widget_hide (gcp_popup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (GTK_WIDGET (gcp)->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (width >> 1);
  y = y_org + y - (height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + width > scr_w) ? scr_w - width : x;
  y = (y + height > scr_h) ? scr_h - height : y;
  gtk_preview_size (GTK_PREVIEW (gcp_popup_preview), width, height);
  gtk_widget_popup (gcp_popup, x, y);

  switch (gcp->type)
    {
    case GCP_BRUSH:
      gimp_context_preview_draw_brush_popup (gcp);
      break;
    case GCP_PATTERN:
      gimp_context_preview_draw_pattern_popup (gcp);
      break;
    default:
      break;
    }  
  gtk_widget_queue_draw (gcp_popup_preview);
}

static void
gimp_context_preview_popup_close ()
{
  if (gcp_popup != NULL)
    gtk_widget_hide (gcp_popup);
}

static gboolean
gimp_context_preview_data_matches_type (GimpContextPreview *gcp,
					gpointer            data)
{
  gboolean match = FALSE;

  g_return_val_if_fail (GIMP_IS_CONTEXT_PREVIEW (gcp), FALSE);

  switch (gcp->type)
    {
    case GCP_BRUSH:
      match = GIMP_IS_BRUSH (data);
      break;
    case GCP_PATTERN:
      match = data != NULL;  /*  would be nice if pattern was a real gtk_object  */
      break;
    default:
      break;
    }
  return (match);
}



/*  brush draw functions */

static void
gimp_context_preview_draw_brush_popup (GimpContextPreview *gcp)
{
  GimpBrush *brush;
  guchar *mask, *buf, *b;
  guchar bg;
  gint width, height;
  gint x, y;

  g_return_if_fail (gcp != NULL && GIMP_IS_BRUSH (gcp->data));

  brush = GIMP_BRUSH (gcp->data);
  width = brush->mask->width;
  height = brush->mask->height;
  buf = g_new (guchar, 3 * width);
  mask = temp_buf_data (brush->mask);

  if (GIMP_IS_BRUSH_PIXMAP (brush))
    {
      guchar *pixmap = temp_buf_data (GIMP_BRUSH_PIXMAP (brush)->pixmap_mask);
     
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
	  gtk_preview_draw_row (GTK_PREVIEW (gcp_popup_preview), buf, 0, y, width); 
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
	  gtk_preview_draw_row (GTK_PREVIEW (gcp_popup_preview), buf, 0, y, width);
	}
    }

  g_free(buf);
}

static void
gimp_context_preview_draw_brush (GimpContextPreview *gcp)
{
  GimpBrush *brush;
  gboolean scale = FALSE;
  gint brush_width, brush_height;
  gint offset_x, offset_y;
  TempBuf *mask_buf, *pixmap_buf = NULL;
  guchar *mask, *buf, *b;
  guchar bg;
  gint x, y;

  g_return_if_fail (gcp != NULL && GIMP_IS_BRUSH (gcp->data));
 
  brush = GIMP_BRUSH (gcp->data);
  brush_width = brush->mask->width;
  brush_height = brush->mask->height;

  if (brush_width > gcp->width || brush_height > gcp->height)
    {
      gdouble ratio_x = (gdouble)brush_width / gcp->width;
      gdouble ratio_y = (gdouble)brush_height / gcp->height;
      
      brush_width =  (gdouble)brush_width / MAX (ratio_x, ratio_y); 
      brush_height = (gdouble)brush_height / MAX (ratio_x, ratio_y);
      
      mask_buf = brush_scale_mask (brush->mask, brush_width, brush_height);
      if (GIMP_IS_BRUSH_PIXMAP (brush))
	{
	  /*  TODO: the scale function should scale the pixmap 
	      and the mask in one run                            */
	  pixmap_buf = brush_scale_pixmap (GIMP_BRUSH_PIXMAP(brush)->pixmap_mask, 
					   brush_width, brush_height);
	}
      scale = TRUE;
    }
  else
    {
       mask_buf = brush->mask;
       if (GIMP_IS_BRUSH_PIXMAP (brush))
	 pixmap_buf = GIMP_BRUSH_PIXMAP(brush)->pixmap_mask;
    }

  offset_x = (gcp->width - brush_width) >> 1;
  offset_y = (gcp->height - brush_height) >> 1;
  
  mask = temp_buf_data (mask_buf);
  buf = g_new (guchar, 3 * gcp->width);
  memset (buf, 255, 3 * gcp->width);

  if (GIMP_IS_BRUSH_PIXMAP (brush)) 
    {
      guchar *pixmap = temp_buf_data (pixmap_buf);

      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 
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
	  gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 
	}
      memset (buf, 255, 3 * gcp->width);
      for (y = brush_height + offset_y; y < gcp->height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 
    }
  else
    {
      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 
      for (y = offset_y; y < brush_height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < brush_width ; x++)
	    {
	      bg = 255 - *mask++;
	      memset (b, bg, 3);
	      b += 3;
	    }   
	  gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 
	}
      memset (buf, 255, 3 * gcp->width);
      for (y = brush_height + offset_y; y < gcp->height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width);    
    }

  if (scale)
    {
      offset_x = gcp->width - scale_indicator_width - 1;
      offset_y = gcp->height - scale_indicator_height - 1;
      for (y = 0; y < scale_indicator_height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gcp), scale_indicator_bits[y][0],
			      offset_x, offset_y + y, scale_indicator_width);
      temp_buf_free (mask_buf);
      if (GIMP_IS_BRUSH_PIXMAP (brush))
	temp_buf_free (pixmap_buf);
    }

  g_free (buf);
 
  gtk_widget_queue_draw (GTK_WIDGET (gcp));
}


/*  pattern draw functions */

static void
gimp_context_preview_draw_pattern_popup (GimpContextPreview *gcp)
{
  GPattern *pattern;
  guchar *mask;
  gint width, height;
  gint x, y;

  g_return_if_fail (gcp != NULL && gcp->data != NULL);

  pattern = (GPattern*)(gcp->data);
  width = pattern->mask->width;
  height = pattern->mask->height;
  mask = temp_buf_data (pattern->mask);

  if (pattern->mask->bytes == 1)
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
	  gtk_preview_draw_row (GTK_PREVIEW (gcp_popup_preview), buf, 0, y, width); 
	}
      g_free(buf);
   }
  else
    {
      for (y = 0; y < height; y++)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (gcp_popup_preview), mask, 0, y, width);
	  mask += 3 * width;
	}
    }
}

static void
gimp_context_preview_draw_pattern (GimpContextPreview *gcp)
{
  GPattern *pattern;
  gint pattern_width, pattern_height;
  gint width, height;
  gint offset_x, offset_y;
  guchar *mask, *buf, *b;
  gint x, y;

  g_return_if_fail (gcp != NULL && gcp->data != NULL);
 
  pattern = (GPattern*)(gcp->data);
  pattern_width = pattern->mask->width;
  pattern_height = pattern->mask->height;

  width = (pattern_width > gcp->width) ?  gcp->width : pattern_width;
  height = (pattern_height > gcp->height) ? gcp->height : pattern_height;

  offset_x = (gcp->width - width) >> 1;
  offset_y = (gcp->height - height) >> 1;
  
  mask = temp_buf_data (pattern->mask);
  buf = g_new (guchar, 3 * gcp->width);

  memset (buf, 255, 3 * gcp->width);
  for (y = 0; y < offset_y; y++)
    gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 

  if (pattern->mask->bytes == 1) 
    {
      for (y = offset_y; y < height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < width ; x++)
	    {
	      memset (b, mask[x], 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width);
	  mask += pattern_width;
	}
    }
  else
    {
      for (y = offset_y; y < height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  memcpy (b, mask, 3 * width);
	  gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 
	  mask += 3 * pattern_width;
	}
    }

  memset (buf, 255, 3 * gcp->width);
  for (y = height + offset_y; y < gcp->height; y++)
    gtk_preview_draw_row (GTK_PREVIEW (gcp), buf, 0, y, gcp->width); 

  g_free(buf);
}
