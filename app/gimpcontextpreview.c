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
#include "config.h"

#include <string.h>

#include "appenv.h"
#include "brush_scale.h"
#include "gimpbrush.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpipeP.h"
#include "gimpcontextpreview.h"
#include "gimpdnd.h"
#include "gradient_header.h"
#include "interface.h"  /* for tool_tips */
#include "patterns.h"
#include "temp_buf.h"

#include "libgimp/gimplimits.h"

/*  the pixmaps for the [scale|pipe]_indicators  */
#define indicator_width 7
#define indicator_height 7

#define WHT {255, 255, 255}
#define BLK {  0,   0,   0}
#define RED {255, 127, 127}

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

static unsigned char pipe_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, WHT, WHT, WHT, RED },
  { WHT, WHT, WHT, WHT, WHT, RED, RED },
  { WHT, WHT, WHT, WHT, RED, RED, RED },
  { WHT, WHT, WHT, RED, RED, RED, RED },
  { WHT, WHT, RED, RED, RED, RED, RED },
  { WHT, RED, RED, RED, RED, RED, RED }
};

static unsigned char scale_pipe_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, RED },
  { WHT, WHT, WHT, BLK, WHT, RED, RED },
  { WHT, BLK, BLK, BLK, BLK, BLK, RED },
  { WHT, WHT, WHT, BLK, RED, RED, RED },
  { WHT, WHT, RED, BLK, RED, RED, RED },
  { WHT, RED, RED, RED, RED, RED, RED }
};


/*  size of the gradient popup  */
#define GRADIENT_POPUP_WIDTH  128
#define GRADIENT_POPUP_HEIGHT 32

#define DRAG_PREVIEW_SIZE 32

/*  event mask for the context_preview  */
#define CONTEXT_PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK |   \
				     GDK_BUTTON_RELEASE_MASK | \
				     GDK_ENTER_NOTIFY_MASK |   \
				     GDK_LEAVE_NOTIFY_MASK)

/*  shared widgets for the popups  */
static GtkWidget *gcp_popup = NULL;
static GtkWidget *gcp_popup_preview = NULL;
static guint gcp_pipe_timer = 0;
static guint gcp_pipe_index = 0;

/*  dnd stuff  */
static GtkTargetEntry context_preview_target_table[3][1] =
{
  { GIMP_TARGET_BRUSH },
  { GIMP_TARGET_PATTERN },
  { GIMP_TARGET_GRADIENT }
};
static guint n_targets = 1;

/*  signals  */
enum
{
  CLICKED,
  LAST_SIGNAL
};

static guint gimp_context_preview_signals[LAST_SIGNAL] = { 0 };
static GtkPreviewClass *parent_class = NULL;

static gpointer gimp_context_preview_get_data             (GimpContextPreview *,
							   gpointer);
static gint     gimp_context_preview_button_press_event   (GtkWidget *, 
							   GdkEventButton *);
static gint     gimp_context_preview_button_release_event (GtkWidget *, 
							   GdkEventButton *);
static void     gimp_context_preview_popup_open           (GimpContextPreview *, 
							   gint, gint);
static void     gimp_context_preview_popup_close          (void);
static gboolean gimp_context_preview_data_matches_type    (GimpContextPreview *,
							   gpointer);
static void     gimp_context_preview_draw_brush           (GimpContextPreview *);
static void     gimp_context_preview_draw_brush_popup     (GimpContextPreview *);
static gint     gimp_context_preview_animate_pipe         (GimpContextPreview *);
static void     gimp_context_preview_draw_pattern         (GimpContextPreview *);
static void     gimp_context_preview_draw_pattern_popup   (GimpContextPreview *);
static void     gimp_context_preview_draw_gradient        (GimpContextPreview *);
static void     gimp_context_preview_draw_gradient_popup  (GimpContextPreview *);

static gint brush_dirty_callback  (GimpBrush *, GimpContextPreview *);
static gint brush_rename_callback (GimpBrush *, GimpContextPreview *);

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
  gcp->data          = NULL;
  gcp->type          = GCP_LAST;
  gcp->width         = 0;
  gcp->height        = 0;
  gcp->popup_width   = 0;
  gcp->popup_height  = 0;
  gcp->show_popup    = FALSE;
  gcp->show_tooltips = FALSE;
  gcp->drag_source   = FALSE;

  GTK_PREVIEW (gcp)->type   = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (gcp)->bpp    = 3;
  GTK_PREVIEW (gcp)->dither = GDK_RGB_DITHER_NORMAL;

  gtk_widget_set_events (GTK_WIDGET (gcp), CONTEXT_PREVIEW_EVENT_MASK);
}

GtkType
gimp_context_preview_get_type (void)
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

GtkWidget *
gimp_context_preview_new (GimpContextPreviewType  type,
			  gint                    width,
			  gint                    height,
			  gboolean                show_popup,
			  gboolean                show_tooltips,
			  gboolean                drag_source,
			  GtkSignalFunc           drop_data_callback,
			  gpointer                drop_data_data)
{
  GimpContextPreview *gcp;

  g_return_val_if_fail (type >= 0 && type < GCP_LAST, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  gcp = gtk_type_new (gimp_context_preview_get_type ());

  gcp->type          = type;
  gcp->width         = width;
  gcp->height        = height;
  gcp->show_popup    = !drag_source && show_popup;
  gcp->show_tooltips = show_tooltips;
  gcp->drag_source   = drag_source;

  gtk_preview_size (GTK_PREVIEW (gcp), width, height);

  /*  drag dest  */
  gtk_drag_dest_set (GTK_WIDGET (gcp),
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
 		     context_preview_target_table[type], n_targets,
		     GDK_ACTION_COPY); 

  switch (type)
    {
    case GCP_BRUSH:
      gimp_dnd_brush_dest_set (GTK_WIDGET (gcp),
			       (GimpDndDropBrushFunc) drop_data_callback,
			       drop_data_data);
      break;
    case GCP_PATTERN:
      gimp_dnd_pattern_dest_set (GTK_WIDGET (gcp),
				 (GimpDndDropPatternFunc) drop_data_callback,
				 drop_data_data);
      break;
    case GCP_GRADIENT:
      gimp_dnd_gradient_dest_set (GTK_WIDGET (gcp),
				  (GimpDndDropGradientFunc) drop_data_callback,
				  drop_data_data);
      break;
    default: 
      break;
    }

  return GTK_WIDGET (gcp);
}

void
gimp_context_preview_update (GimpContextPreview *gcp,
			     gpointer            data)
{
  g_return_if_fail (GIMP_IS_CONTEXT_PREVIEW (gcp));
  g_return_if_fail (gimp_context_preview_data_matches_type (gcp, data));

  if (!gcp->data && gcp->drag_source)  /* first call  */
    {
      gtk_drag_source_set (GTK_WIDGET (gcp),
			   GDK_BUTTON1_MASK,
			   context_preview_target_table[gcp->type], n_targets,
			   GDK_ACTION_COPY);

      switch (gcp->type)
	{
	case GCP_BRUSH:
	  gimp_dnd_brush_source_set
	    (GTK_WIDGET (gcp),
	     (GimpDndDragBrushFunc) gimp_context_preview_get_data,
	     NULL);
	  break;
	case GCP_PATTERN:
	  gimp_dnd_pattern_source_set
	    (GTK_WIDGET (gcp),
	     (GimpDndDragPatternFunc) gimp_context_preview_get_data,
	     NULL);
	  break;
	case GCP_GRADIENT:
	  gimp_dnd_gradient_source_set
	    (GTK_WIDGET (gcp),
	     (GimpDndDragGradientFunc) gimp_context_preview_get_data,
	     NULL);
	  break;
	default: 
	  break;
	}
    }

  if (gcp->data && gcp->type == GCP_BRUSH)
    gtk_signal_disconnect_by_data (GTK_OBJECT (gcp->data), gcp);

  gcp->data = data;
  if (GTK_IS_OBJECT (gcp->data))
    gtk_signal_connect (GTK_OBJECT (gcp->data), "destroy",
			gtk_widget_destroyed, &gcp->data);
  switch (gcp->type)
    {
    case GCP_BRUSH:
      gimp_context_preview_draw_brush (gcp);
      gtk_signal_connect (GTK_OBJECT (gcp->data), "dirty",
			  GTK_SIGNAL_FUNC (brush_dirty_callback), gcp);
      gtk_signal_connect (GTK_OBJECT (gcp->data), "rename",
			  GTK_SIGNAL_FUNC (brush_rename_callback), gcp);
      break;
    case GCP_PATTERN:
      gimp_context_preview_draw_pattern (gcp);
      break;
    case GCP_GRADIENT:
      gimp_context_preview_draw_gradient (gcp);
      break;
    default: 
      break;
    }
  gtk_widget_queue_draw (GTK_WIDGET (gcp));

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
	    GPattern *pattern = (GPattern *) (gcp->data);
	    name = pattern->name;
	  }
	  break;
	case GCP_GRADIENT:
	  {
	    gradient_t *gradient = (gradient_t *) (gcp->data);
	    name = gradient->name;
	  }
	  break;
	default:
	  break;
	}
      gtk_tooltips_set_tip (tool_tips, GTK_WIDGET (gcp), name, NULL);
    }
}

static gpointer
gimp_context_preview_get_data (GimpContextPreview *gcp,
			       gpointer            data)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT_PREVIEW (gcp), NULL);

  return gcp->data;
}

static gint
gimp_context_preview_button_press_event (GtkWidget      *widget,
					 GdkEventButton *bevent)
{
  if (bevent->button == 1)
    {
      if (GIMP_CONTEXT_PREVIEW (widget)->show_popup)
	{
	  gdk_pointer_grab (widget->window, FALSE, GDK_BUTTON_RELEASE_MASK,
			    NULL, NULL, bevent->time);
	  gimp_context_preview_popup_open (GIMP_CONTEXT_PREVIEW (widget), 
					   bevent->x, bevent->y);
	}
      gtk_signal_emit_by_name (GTK_OBJECT (widget), "clicked");
    }
  return FALSE;
}
  
static gint
gimp_context_preview_button_release_event (GtkWidget      *widget,
					   GdkEventButton *bevent)
{
  if (bevent->button == 1 && GIMP_CONTEXT_PREVIEW (widget)->show_popup)
    {
      gdk_pointer_ungrab (bevent->time);
      gimp_context_preview_popup_close ();
    }
  return FALSE;
}

static void
gimp_context_preview_popup_open (GimpContextPreview *gcp,
				 gint                x,
				 gint                y)
{
  gint x_org, y_org;
  gint scr_w, scr_h;

  g_return_if_fail (gcp != NULL);
  if (!gcp->data)
    return;

  switch (gcp->type)
    {
    case GCP_BRUSH:
      {
	GimpBrush *brush = GIMP_BRUSH (gcp->data);
	gcp->popup_width  = brush->mask->width;
	gcp->popup_height = brush->mask->height;

	if (GIMP_IS_REALLY_A_BRUSH_PIPE (brush))
	  {
	    GimpBrushPipe *pipe = GIMP_BRUSH_PIPE (brush);
	    gint i;

	    for (i = 1; i < pipe->nbrushes; i++)
	      {
		brush = GIMP_BRUSH (pipe->brushes[i]);
		gcp->popup_width = MAX (gcp->popup_width, brush->mask->width);
		gcp->popup_height = MAX (gcp->popup_height, brush->mask->height);
	      }
	  }
	else if (gcp->popup_width <= gcp->width && gcp->popup_height <= gcp->height)
	  return;
      }
      break;
    case GCP_PATTERN:
      {
	GPattern *pattern = (GPattern*)gcp->data;
	gcp->popup_width  = pattern->mask->width;
	gcp->popup_height = pattern->mask->height;
	if (gcp->popup_width <= gcp->width && gcp->popup_height <= gcp->height)
	  return;      
      }
      break;
    case GCP_GRADIENT:
      gcp->popup_width  = GRADIENT_POPUP_WIDTH;
      gcp->popup_height = GRADIENT_POPUP_HEIGHT;
      if (gcp->popup_width <= gcp->width && gcp->popup_height <= gcp->height)
	return;
     break;
     default:
      return;
    }

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
  x = x_org + x - (gcp->popup_width >> 1);
  y = y_org + y - (gcp->popup_height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + gcp->popup_width > scr_w) ? scr_w - gcp->popup_width : x;
  y = (y + gcp->popup_height > scr_h) ? scr_h - gcp->popup_height : y;
  gtk_preview_size (GTK_PREVIEW (gcp_popup_preview), gcp->popup_width, gcp->popup_height);
  gtk_widget_popup (gcp_popup, x, y);

  switch (gcp->type)
    {
    case GCP_BRUSH:      
      gimp_context_preview_draw_brush_popup (gcp);
      if (GIMP_IS_REALLY_A_BRUSH_PIPE (gcp->data) && gcp_pipe_timer == 0)
	{
	  gcp_pipe_index = 0;
	  gcp_pipe_timer = gtk_timeout_add (300, (GtkFunction)gimp_context_preview_animate_pipe, gcp);
	}
      break;
    case GCP_PATTERN:
      gimp_context_preview_draw_pattern_popup (gcp);
      break;
    case GCP_GRADIENT:
      gimp_context_preview_draw_gradient_popup (gcp);
      break;
    default:
      break;
    }  
  gtk_widget_queue_draw (gcp_popup_preview);
}
  
static void
gimp_context_preview_popup_close ()
{
  if (gcp_pipe_timer > 0)
    gtk_timeout_remove (gcp_pipe_timer);
  gcp_pipe_timer = 0;
  
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
    case GCP_GRADIENT:
      match = data != NULL;  /*  would be nicer if these were real gtk_objects  */
      break;
    default:
      break;
    }
  return (match);
}

/*  brush draw functions */

void
draw_brush (GtkPreview *preview,
	    GimpBrush  *brush,
	    gint        width,
	    gint        height,
	    gboolean    is_popup)
{
  gboolean scale = FALSE;
  gint brush_width, brush_height;
  gint offset_x, offset_y;
  TempBuf *mask_buf, *pixmap_buf = NULL;
  guchar *mask, *buf, *b;
  guchar bg;
  gint x, y;

  mask_buf = brush->mask;
  if (GIMP_IS_BRUSH_PIXMAP (brush))
    pixmap_buf = GIMP_BRUSH_PIXMAP (brush)->pixmap_mask;
  brush_width = mask_buf->width;
  brush_height = mask_buf->height;

  if (brush_width > width || brush_height > height)
    {
      gdouble ratio_x = (gdouble)brush_width / width;
      gdouble ratio_y = (gdouble)brush_height / height;
      
      brush_width =  (gdouble)brush_width / MAX (ratio_x, ratio_y) + 0.5; 
      brush_height = (gdouble)brush_height / MAX (ratio_x, ratio_y) + 0.5;
      
      mask_buf = brush_scale_mask (mask_buf, brush_width, brush_height);
      if (GIMP_IS_BRUSH_PIXMAP (brush))
	{
	  /*  TODO: the scale function should scale the pixmap 
	      and the mask in one run                            */
	  pixmap_buf = brush_scale_pixmap (pixmap_buf, brush_width, brush_height);
	}
      scale = TRUE;
    }

  offset_x = (width - brush_width) / 2;
  offset_y = (height - brush_height) / 2;
  
  mask = temp_buf_data (mask_buf);
  buf = g_new (guchar, 3 * width);
  memset (buf, 255, 3 * width);

  if (GIMP_IS_BRUSH_PIXMAP (brush)) 
    {
      guchar *pixmap = temp_buf_data (pixmap_buf);

      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width); 
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
	  gtk_preview_draw_row (preview, buf, 0, y, width); 
	}
      memset (buf, 255, 3 * width);
      for (y = brush_height + offset_y; y < height; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width); 
    }
  else
    {
      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width); 
      for (y = offset_y; y < brush_height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < brush_width ; x++)
	    {
	      bg = 255 - *mask++;
	      memset (b, bg, 3);
	      b += 3;
	    }   
	  gtk_preview_draw_row (preview, buf, 0, y, width); 
	}
      memset (buf, 255, 3 * width);
      for (y = brush_height + offset_y; y < height; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width);    
    }

  if (scale)
    {
      offset_x = width - indicator_width;
      offset_y = height - indicator_height;
      for (y = 0; y < indicator_height; y++)
	(GIMP_IS_REALLY_A_BRUSH_PIPE (brush)) ?
	  gtk_preview_draw_row (preview, scale_pipe_indicator_bits[y][0],
				offset_x, offset_y + y, indicator_width) :
	  gtk_preview_draw_row (preview, scale_indicator_bits[y][0],
				offset_x, offset_y + y, indicator_width);
      temp_buf_free (mask_buf);
      if (GIMP_IS_BRUSH_PIXMAP (brush))
	temp_buf_free (pixmap_buf);
    }
  else if (!is_popup && GIMP_IS_REALLY_A_BRUSH_PIPE (brush))
    {
      offset_x = width - indicator_width;
      offset_y = height - indicator_height;
      for (y = 0; y < indicator_height; y++)
	gtk_preview_draw_row (preview, pipe_indicator_bits[y][0],
			      offset_x, offset_y + y, indicator_width);
    }

  g_free (buf);
}

static void
gimp_context_preview_draw_brush_popup (GimpContextPreview *gcp)
{
  GimpBrush *brush;

  g_return_if_fail (gcp != NULL && GIMP_IS_BRUSH (gcp->data));

  brush = GIMP_BRUSH (gcp->data);
  draw_brush (GTK_PREVIEW (gcp_popup_preview), brush, 
	      gcp->popup_width, gcp->popup_height, TRUE);
}

static void
gimp_context_preview_draw_brush (GimpContextPreview *gcp)
{
  GimpBrush *brush;
 
  g_return_if_fail (gcp != NULL && GIMP_IS_BRUSH (gcp->data));

  brush = GIMP_BRUSH (gcp->data);
  draw_brush (GTK_PREVIEW (gcp), brush, gcp->width, gcp->height, FALSE);
}


static gint
gimp_context_preview_animate_pipe (GimpContextPreview *gcp)
{
  GimpBrushPipe *pipe;
  GimpBrush *brush;

  g_return_val_if_fail (gcp != NULL && GIMP_IS_REALLY_A_BRUSH_PIPE (gcp->data), FALSE);
  if (gcp_popup != NULL && !GTK_WIDGET_VISIBLE (gcp_popup))
    {
      gcp_pipe_timer = 0;
      return (FALSE);
    }
  
  pipe = GIMP_BRUSH_PIPE (gcp->data);
  if (++gcp_pipe_index >= pipe->nbrushes)
    gcp_pipe_index = 0;
  brush = GIMP_BRUSH (pipe->brushes[gcp_pipe_index]);
  draw_brush (GTK_PREVIEW (gcp_popup_preview), brush,
	      gcp->popup_width, gcp->popup_height, TRUE);
  gtk_widget_queue_draw (gcp_popup_preview);
  
  return (TRUE);
}

/*  brush callbacks  */
static gint 
brush_dirty_callback (GimpBrush          *brush, 
		      GimpContextPreview *gcp)
{
  gimp_context_preview_draw_brush (gcp);
  gtk_widget_queue_draw (GTK_WIDGET (gcp));

  return TRUE;
}

static gint 
brush_rename_callback (GimpBrush          *brush, 
		       GimpContextPreview *gcp)
{
  if (gcp->show_tooltips)
    gtk_tooltips_set_tip (tool_tips, GTK_WIDGET (gcp), brush->name, NULL);

  return TRUE;
}


/*  pattern draw functions */

void
draw_pattern (GtkPreview *preview,
	      GPattern   *pattern,
	      gint        width,
	      gint        height)
{
  gint pattern_width, pattern_height;
  guchar *mask, *src, *buf, *b;
  gint x, y;
 
  pattern_width = pattern->mask->width;
  pattern_height = pattern->mask->height;

  mask = temp_buf_data (pattern->mask);
  buf = g_new (guchar, 3 * width);
  if (pattern->mask->bytes == 1) 
    {
      for (y = 0; y < height; y++)
	{
	  b = buf;
	  src = mask + (pattern_width * (y % pattern_height));
	  for (x = 0; x < width ; x++)
	    {
	      memset (b, src[x % pattern_width], 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (preview, buf, 0, y, width);
	}
    }
  else
    {
      for (y = 0; y < height; y++)
	{
	  b = buf;
	  src = mask + 3 * (pattern_width * (y % pattern_height));
	  for (x = 0; x < width ; x++)
	    {
	      memcpy (b, src + 3 * (x % pattern_width), 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (preview, buf, 0, y, width); 
	}
    }
  g_free(buf);
}

static void
gimp_context_preview_draw_pattern_popup (GimpContextPreview *gcp)
{
  GPattern *pattern;

  g_return_if_fail (gcp != NULL && gcp->data != NULL);

  pattern = (GPattern*)(gcp->data);
  draw_pattern (GTK_PREVIEW (gcp_popup_preview), pattern, 
		gcp->popup_width, gcp->popup_height);
}

static void
gimp_context_preview_draw_pattern (GimpContextPreview *gcp)
{
  GPattern *pattern;

  g_return_if_fail (gcp != NULL && gcp->data != NULL);
 
  pattern = (GPattern*)(gcp->data);
  draw_pattern (GTK_PREVIEW (gcp), pattern, gcp->width, gcp->height);
}


/*  gradient draw functions  */

void
draw_gradient (GtkPreview *preview, 
	       gradient_t *gradient,
	       gint        width,
	       gint        height)
{
  guchar *p0, *p1, *even, *odd;
  gint x, y;
  gdouble dx, cur_x;
  gdouble r, g, b, a;
  gdouble c0, c1;

  dx    = 1.0 / (width - 1);
  cur_x = 0.0;
  p0    = even = g_new (guchar, 3 * width);
  p1    = odd  = g_new (guchar, 3 * width);

  for (x = 0; x < width; x++) 
    {
      gradient_get_color_at (gradient, cur_x, &r, &g, &b, &a);
    
      if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	{
	  c0 = GIMP_CHECK_LIGHT;
	  c1 = GIMP_CHECK_DARK;
	} 
      else 
	{
	  c0 = GIMP_CHECK_DARK;
	  c1 = GIMP_CHECK_LIGHT;
	}

      *p0++ = (c0 + (r - c0) * a) * 255.0;
      *p0++ = (c0 + (g - c0) * a) * 255.0;
      *p0++ = (c0 + (b - c0) * a) * 255.0;
      
      *p1++ = (c1 + (r - c1) * a) * 255.0;
      *p1++ = (c1 + (g - c1) * a) * 255.0;
      *p1++ = (c1 + (b - c1) * a) * 255.0;
      
      cur_x += dx;
    }

  for (y = 0; y < height; y++)
    {
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (preview, odd, 0, y, width);
      else
	gtk_preview_draw_row (preview, even, 0, y, width);
    }

  g_free (odd);
  g_free (even);
}

static void
gimp_context_preview_draw_gradient_popup (GimpContextPreview *gcp)
{
  gradient_t *gradient;

  g_return_if_fail (gcp != NULL && gcp->data != NULL);
  
  gradient = (gradient_t*)(gcp->data);
  draw_gradient (GTK_PREVIEW (gcp_popup_preview), gradient, 
		 gcp->popup_width, gcp->popup_height);
}

static void
gimp_context_preview_draw_gradient (GimpContextPreview *gcp)
{
  gradient_t *gradient;

  g_return_if_fail (gcp != NULL && gcp->data != NULL);

  gradient = (gradient_t*)(gcp->data);
  draw_gradient (GTK_PREVIEW (gcp), gradient, gcp->width, gcp->height);
}
