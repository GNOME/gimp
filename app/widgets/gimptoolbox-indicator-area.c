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
#include <string.h>
#include "appenv.h"
#include "indicator_area.h"
#include "gimpbrushlist.h"

#define CELL_SIZE 23 /* The size of the previews */
#define CELL_PADDING 2 /* How much between brush and pattern cells */
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
                          GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
                          GDK_ENTER_NOTIFY_MASK | \
                          GDK_LEAVE_NOTIFY_MASK

/*  Global variables  */
static void
brush_popup_open (GimpBrushP brush, int x, int y);

/*  Prototypes */
static void brush_popup_open (GimpBrushP brush, int x, int y);
static gint brush_area_events (GtkWidget  *widget, GdkEvent  *event);
static void pattern_popup_open (GPatternP brush, int x, int y);
static gint pattern_area_events (GtkWidget  *widget, GdkEvent  *event);

/*  Static variables  */
static GtkWidget *indicator_table;
static GtkWidget *brush_preview;
static GtkWidget *pattern_preview;
static GtkWidget *device_bpopup = NULL; 
static GtkWidget *device_bpreview = NULL; 
static GtkWidget *device_patpopup = NULL;
static GtkWidget *device_patpreview = NULL;

static void
brush_popup_open (GimpBrushP brush,
		  int          x,
                  int          y)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;
  /* make sure the popup exists and is not visible */
  if (device_bpopup == NULL)
    {
      GtkWidget *frame;

      device_bpopup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (device_bpopup), FALSE, FALSE, TRUE);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (device_bpopup), frame);
      gtk_widget_show (frame);
      device_bpreview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
      gtk_container_add (GTK_CONTAINER (frame), device_bpreview);
      gtk_widget_show (device_bpreview);
    }
  else
    {
      gtk_widget_hide (device_bpopup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (brush_preview->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (brush->mask->width >> 1);
  y = y_org + y - (brush->mask->height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + brush->mask->width > scr_w) ? scr_w - brush->mask->width : x;
  y = (y + brush->mask->height > scr_h) ? scr_h - brush->mask->height : y;
  gtk_preview_size (GTK_PREVIEW (device_bpreview), brush->mask->width, brush->mask->height);
  gtk_widget_popup (device_bpopup, x, y);
 
  /*  Draw the brush  */
  buf = g_new (gchar, brush->mask->width);
  src = (gchar *)temp_buf_data (brush->mask);
  for (y = 0; y < brush->mask->height; y++)
    {
      for (x = 0; x < brush->mask->width; x++)
        buf[x] = 255 - src[x];
      gtk_preview_draw_row (GTK_PREVIEW (device_bpreview), (guchar *)buf, 0, y, brush->mask->width);
      src += brush->mask->width;
    }
  g_free(buf);
  /*  Draw the brush preview  */
  gtk_widget_draw (device_bpreview, NULL);
}

void
brush_area_update ()
{
  guchar buffer[CELL_SIZE];
  TempBuf * brush_buf;
  GimpBrushP brush;
  unsigned char * src, *s = NULL;
  unsigned char *b;
  int width,height;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i, j;

  brush = get_active_brush();
  if (!brush) 
    {
    g_warning("No gimp brush found\n");
    return;
    }
	
  brush_buf = brush->mask;

  /*  Get the pointer into the brush mask data  */
  src = mask_buf_data (brush_buf);

  /* Limit to cell size */
  width = (brush_buf->width > CELL_SIZE) ? CELL_SIZE: brush_buf->width;
  height = (brush_buf->height > CELL_SIZE) ? CELL_SIZE: brush_buf->height;

  /* Set buffer to white */
  memset(buffer, 255, sizeof(buffer));
  for (i = 0; i < CELL_SIZE; i++)
    gtk_preview_draw_row (GTK_PREVIEW(brush_preview), buffer, 0, i, CELL_SIZE);

  offset_x = ((CELL_SIZE - width) >> 1);
  offset_y = ((CELL_SIZE - height) >> 1);
  ystart = BOUNDS (offset_y, 0, CELL_SIZE);
  yend = BOUNDS (offset_y + height, 0, CELL_SIZE);

  for (i = ystart; i < yend; i++)
    {
      /*  Invert the mask for display.  We're doing this because
       *  a value of 255 in the  mask means it is full intensity.
       *  However, it makes more sense for full intensity to show
       *  up as black in this brush preview window...
       */
      s = src;
      b = buffer;
      for (j = 0; j < width ; j++)
        *b++ = 255 - *s++;

      gtk_preview_draw_row (GTK_PREVIEW(brush_preview),
                            buffer,
                            offset_x, i, width);
      src += brush_buf->width;
    }
  gtk_widget_draw(brush_preview, NULL);
  gtk_widget_show(brush_preview);
}

static gint
brush_area_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  
  GimpBrushP brush;
  brush = get_active_brush();

  switch (event->type)
    {
    case GDK_EXPOSE: 
      break;
    case GDK_BUTTON_RELEASE:
      if (device_bpopup != NULL)
         gtk_widget_hide (device_bpopup);
      brush_area_update();
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	/* pop up the brush selection dialog accordingly */
	create_brush_dialog();
	/*  Show the brush popup window if the brush is too large  */
        if (brush->mask->width > CELL_SIZE ||
            brush->mask->height > CELL_SIZE)
       		 brush_popup_open (brush, bevent->x, bevent->y);

	}
      break;
    case GDK_DELETE:
      break;

    default:
      break;
    }

  return FALSE;
}



void
pattern_area_update()
{
  guchar *buffer = NULL;
  TempBuf * pattern_buf;
  unsigned char * src, *s = NULL;
  unsigned char *b;
  int rowstride;
  int width,height;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i, j;

  GPatternP pattern = get_active_pattern();
  
  buffer = g_new (guchar, pattern->mask->width * 3);
  pattern_buf = pattern->mask;

  /* Limit to cell size */
  width = (pattern_buf->width > CELL_SIZE) ? CELL_SIZE: pattern_buf->width;
  height = (pattern_buf->height > CELL_SIZE) ? CELL_SIZE: pattern_buf->height;

  /* Set buffer to white */
  memset(buffer, 255, sizeof(buffer));
  for (i = 0; i < CELL_SIZE; i++)
    gtk_preview_draw_row (GTK_PREVIEW(pattern_preview), buffer, 0, i, CELL_SIZE);

  offset_x = ((CELL_SIZE - width) >> 1);
  offset_y = ((CELL_SIZE - height) >> 1);

  ystart = BOUNDS (offset_y, 0, CELL_SIZE);
  yend = BOUNDS (offset_y + height, 0, CELL_SIZE);

  /*  Get the pointer into the brush mask data  */
  rowstride = pattern_buf->width * pattern_buf->bytes;
  src = mask_buf_data (pattern_buf)  + (ystart - offset_y) * rowstride;

  for (i = ystart; i < yend; i++)
    {
      s = src;
      b = buffer;

      if (pattern_buf->bytes == 1)
        for (j = 0; j < width; j++)
          {
            *b++ = *s;
            *b++ = *s;
            *b++ = *s++;
          }
      else
        for (j = 0; j < width; j++)
          {
            *b++ = *s++;
            *b++ = *s++;
            *b++ = *s++;
          }
      gtk_preview_draw_row (GTK_PREVIEW (pattern_preview),
                            buffer,
                            offset_x, i, width);

      src += rowstride;
    }
  gtk_widget_draw(pattern_preview, NULL);
  gtk_widget_show(pattern_preview);
  g_free(buffer);
}

static void
pattern_popup_open ( GPatternP pattern,
		     int          x, 
		     int          y )
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;

  /* make sure the popup exists and is not visible */
  if (device_patpopup == NULL)
    {
      GtkWidget *frame;

      device_patpopup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (device_patpopup), FALSE, FALSE, TRUE);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (device_patpopup), frame);
      gtk_widget_show (frame);
      device_patpreview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_container_add (GTK_CONTAINER (frame), device_patpreview);
      gtk_widget_show (device_patpreview);
    }
  else
    {
      gtk_widget_hide (device_patpopup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (pattern_preview->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (pattern->mask->width >> 1);
  y = y_org + y - (pattern->mask->height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + pattern->mask->width > scr_w) ? scr_w - pattern->mask->width : x;
  y = (y + pattern->mask->height > scr_h) ? scr_h - pattern->mask->height : y;
  gtk_preview_size (GTK_PREVIEW (device_patpreview), pattern->mask->width, pattern->mask->height);

  gtk_widget_popup (device_patpopup, x, y);

  /*  Draw the pattern  */
  buf = g_new (gchar, pattern->mask->width * 3);
  src = (gchar *)temp_buf_data (pattern->mask);
  for (y = 0; y < pattern->mask->height; y++)
    {
      if (pattern->mask->bytes == 1)
        for (x = 0; x < pattern->mask->width; x++)
          {
            buf[x*3+0] = src[x];
            buf[x*3+1] = src[x];
            buf[x*3+2] = src[x];
          }
      else
        for (x = 0; x < pattern->mask->width; x++)
          {
            buf[x*3+0] = src[x*3+0];
            buf[x*3+1] = src[x*3+1];
            buf[x*3+2] = src[x*3+2];
          }
      gtk_preview_draw_row (GTK_PREVIEW (device_patpreview), (guchar *)buf, 0, y, pattern->mask->width);
      src += pattern->mask->width * pattern->mask->bytes;
    }
  g_free(buf);

  /*  Draw the brush preview  */
  gtk_widget_draw (device_patpreview, NULL);
}

static gint
pattern_area_events (GtkWidget    *widget,
                       GdkEvent     *event)
{
  GdkEventButton *bevent;
  GPatternP pattern;

  pattern = get_active_pattern();
  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
	 create_pattern_dialog(); 

              /*  Show the pattern popup window if the pattern is too large  */
              if (pattern->mask->width > CELL_SIZE ||
                  pattern->mask->height > CELL_SIZE)
              		pattern_popup_open (pattern, bevent->x, bevent->y);
      }
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
  
      if (bevent->button == 1)
        {
        if (device_patpopup != NULL)
           gtk_widget_hide (device_patpopup);
        pattern_area_update();
        }
      break;
    case GDK_DELETE:
      break;

    default:
      break;
    }
  return FALSE;
}



GtkWidget *
indicator_area_create (int        width,
		   int        height)
{
/* Put the brush in the table */
  indicator_table = gtk_table_new (1,3, FALSE);
  
  brush_preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (brush_preview), CELL_SIZE, CELL_SIZE); 
  gtk_widget_set_events (brush_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (brush_preview), "event",
                     (GtkSignalFunc) brush_area_events,
                     NULL);
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), brush_preview,
                            0, 1, 0, 1);
                            
  pattern_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (pattern_preview), CELL_SIZE, CELL_SIZE); 
  gtk_widget_set_events (pattern_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (pattern_preview), "event",
                     (GtkSignalFunc) pattern_area_events,
                     NULL);
 
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), pattern_preview,
                           1, 2, 0, 1);

  brush_area_update();
  pattern_area_update();
  gtk_widget_show(indicator_table);
  return indicator_table;
}
