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

#define CELL_SIZE 19 /* The size of the previews */
#define CELL_PADDING 2 /* How much between brush and pattern cells */
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
                          GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
                          GDK_ENTER_NOTIFY_MASK | \
                          GDK_LEAVE_NOTIFY_MASK

/*  Global variables  */

/*  Static variables  */
static GtkWidget *indicator_table;
static GtkWidget *brush_preview;

/* brush_area_edit */
static gint
brush_area_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  
  switch (event->type)
    {
    case GDK_EXPOSE: 
      break;
    case GDK_BUTTON_RELEASE:
      brush_area_update();
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	/* pop up the brush selection dialog accordingly */
	create_brush_dialog();
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
                            
  brush_area_update();
  gtk_widget_show(indicator_table);
/* Not yet - get the brush working first
   pattern_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
   gtk_preview_size (GTK_PREVIEW (pattern_preview, CELL_SIZE, CELL_SIZE); 
   gtk_widget_set_events (pattern_preview, PREVIEW_EVENT_MASK);
   gtk_signal_connect (GTK_OBJECT (pattern_preview), "event",
                      (GtkSignalFunc) pattern_preview_events,
                      NULL;

   gtk_table_attach (GTK_TABLE(indicator_table), pattern_preview,
                            1, 2, 0, 1,
                            0, 0, CELL_SPACING, CELL_SPACING);
*/

  return indicator_table;
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

