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
#include <stdio.h>
#include <string.h>
#include "appenv.h"
#include "actionarea.h"
#include "canvas.h"
#include "patterns.h"
#include "pattern_select.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "disp_callbacks.h"
#include "errors.h"
#include "tag.h"


#define MIN_CELL_SIZE    32
#define MAX_CELL_SIZE    45


/*
#define STD_PATTERN_COLUMNS 6
#define STD_PATTERN_ROWS    5 
*/

#define MAX_WIN_WIDTH     (MIN_CELL_SIZE * NUM_PATTERN_COLUMNS)
#define MAX_WIN_HEIGHT    (MIN_CELL_SIZE * NUM_PATTERN_ROWS)
#define MARGIN_WIDTH      1
#define MARGIN_HEIGHT     1
#define PATTERN_EVENT_MASK GDK_BUTTON1_MOTION_MASK | \
                           GDK_EXPOSURE_MASK | \
                           GDK_BUTTON_PRESS_MASK | \
			   GDK_ENTER_NOTIFY_MASK

/*  local function prototypes  */
static void pattern_popup_open               (PatternSelectP, int, int, GPatternP);
static void pattern_popup_close              (PatternSelectP);
static void display_pattern                  (PatternSelectP, GPatternP, int, int);
static void display_patterns                 (PatternSelectP);
static void display_setup                    (PatternSelectP);
static void draw_preview                     (PatternSelectP);
static void preview_calc_scrollbar           (PatternSelectP);
static void pattern_select_show_selected     (PatternSelectP, int, int);
static void update_active_pattern_field      (PatternSelectP);
static void pattern_select_close_callback    (GtkWidget *, gpointer);
static gint pattern_select_delete_callback    (GtkWidget *, GdkEvent *, gpointer);
static void pattern_select_refresh_callback  (GtkWidget *, gpointer);
static gint pattern_select_events            (GtkWidget *, GdkEvent *, PatternSelectP);
static gint pattern_select_resize            (GtkWidget *, GdkEvent *, PatternSelectP);
static void pattern_select_scroll_update     (GtkAdjustment *, gpointer);

/* 8bit channels data functions */
static void display_pattern_get_row_u8     (guchar *, Canvas *, gint, gint);

/* 16bit channels data functions */
static void display_pattern_get_row_u16    (guchar *, Canvas *, gint, gint);

/* float channels data functions */
static void display_pattern_get_row_float  (guchar *, Canvas *, gint, gint);

/* Convert row of pattern to 8bit display */
typedef void  (*DisplayPatternGetRowFunc) (guchar *, Canvas *, gint, gint);
static DisplayPatternGetRowFunc display_pattern_get_row_funcs[3] =
{
  display_pattern_get_row_u8,
  display_pattern_get_row_u16,
  display_pattern_get_row_float
};

/*  the action area structure  */
static ActionAreaItem action_items[2] =
{
  { "Close", pattern_select_close_callback, NULL, NULL },
  { "Refresh", pattern_select_refresh_callback, NULL, NULL },
};

gint NUM_PATTERN_COLUMNS = 6;
gint NUM_PATTERN_ROWS    = 5;
gint STD_CELL_SIZE = MIN_CELL_SIZE;

PatternSelectP
pattern_select_new ()
{
  PatternSelectP psp;
  GPatternP active;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *sbar;
  GtkWidget *label_box;

  psp = g_malloc (sizeof (_PatternSelect));
  psp->preview = NULL;
  psp->pattern_popup = NULL;

  /*  The shell and main vbox  */
  psp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (psp->shell), "patternselection", "Gimp");
  gtk_window_set_title (GTK_WINDOW (psp->shell), "Pattern Selection");
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_window_set_policy(GTK_WINDOW(psp->shell), FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (psp->shell)->vbox), vbox, TRUE, TRUE, 0);

  /* handle the wm close event */
  gtk_signal_connect (GTK_OBJECT (psp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (pattern_select_delete_callback),
		      psp);

  psp->options_box = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), psp->options_box, FALSE, FALSE, 0);

  /*  Create the active pattern label  */
  label_box = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (label_box), 2);
  gtk_box_pack_start (GTK_BOX (psp->options_box), label_box, FALSE, FALSE, 0);
  psp->pattern_name = gtk_label_new ("Active");
  gtk_box_pack_start (GTK_BOX (label_box), psp->pattern_name, FALSE, FALSE, 2);
  psp->pattern_size = gtk_label_new ("(0x0)");
  gtk_box_pack_start (GTK_BOX (label_box), psp->pattern_size, FALSE, FALSE, 5);

  gtk_widget_show (psp->pattern_name);
  gtk_widget_show (psp->pattern_size);
  gtk_widget_show (label_box);

  /*  The horizontal box containing preview & scrollbar  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  psp->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (psp->frame), GTK_SHADOW_IN);

  gtk_box_pack_start (GTK_BOX (hbox), psp->frame, TRUE, TRUE, 0);

  psp->sbar_data = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, MAX_WIN_HEIGHT, 1, 1, MAX_WIN_HEIGHT));
  gtk_signal_connect (GTK_OBJECT (psp->sbar_data), "value_changed",
		      (GtkSignalFunc) pattern_select_scroll_update,
		      psp);
  sbar = gtk_vscrollbar_new (psp->sbar_data);
  gtk_box_pack_start (GTK_BOX (hbox), sbar, FALSE, FALSE, 0);

  /*  Create the pattern preview window and the underlying image  */
  psp->cell_width = STD_CELL_SIZE;
  psp->cell_height = STD_CELL_SIZE;

  psp->width = MAX_WIN_WIDTH;
  psp->height = MAX_WIN_HEIGHT;

  psp->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (psp->preview), psp->width, psp->height);
  gtk_widget_set_events (psp->preview, PATTERN_EVENT_MASK);

  gtk_signal_connect (GTK_OBJECT (psp->preview), "event",
		      (GtkSignalFunc) pattern_select_events,
		      psp);

  gtk_signal_connect_after (GTK_OBJECT(psp->frame), "size_allocate",
                           (GtkSignalFunc)pattern_select_resize,
                           psp);

  gtk_container_add (GTK_CONTAINER (psp->frame), psp->preview);
  gtk_widget_show (psp->preview);

  gtk_widget_show (sbar);
  gtk_widget_show (hbox);
  gtk_widget_show (psp->frame);

  /*  The action area  */
  action_items[0].user_data = psp;
  action_items[1].user_data = psp;
  build_action_area (GTK_DIALOG (psp->shell), action_items, 2, 0);

  gtk_widget_show (psp->options_box);
  gtk_widget_show (vbox);
  gtk_widget_show (psp->shell);

  if(no_data)   /* if patterns are already loaded, dont do it now... */
    patterns_init(FALSE);
  preview_calc_scrollbar (psp);
  display_patterns (psp);

  
  /*  update the active selection  */
  active = get_active_pattern ();
  if (active)
    pattern_select_select (psp, active->index);

  return psp;
}

void
pattern_select_select (PatternSelectP psp,
		       int            index)
{
  int row, col;

  update_active_pattern_field (psp);
  row = index / NUM_PATTERN_COLUMNS;
  col = index - row * NUM_PATTERN_COLUMNS;

  pattern_select_show_selected (psp, row, col);
}

void
pattern_select_free (PatternSelectP psp)
{
  if (psp)
    {
      if (psp->pattern_popup != NULL)
	gtk_widget_destroy (psp->pattern_popup);
      g_free (psp);
    }
}

/*
 *  Local functions
 */
static void
pattern_popup_open (PatternSelectP psp,
		    int            x,
		    int            y,
		    GPatternP      pattern)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;
  gint pattern_width, pattern_height;
  gint rowbytes;
  Tag pattern_tag;
  Precision pattern_prec;

  /* make sure the popup exists and is not visible */
  if (psp->pattern_popup == NULL)
    {
      GtkWidget *frame;
      psp->pattern_popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (psp->pattern_popup), FALSE, FALSE, TRUE);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (psp->pattern_popup), frame);
      gtk_widget_show (frame);
      psp->pattern_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_container_add (GTK_CONTAINER (frame), psp->pattern_preview);
      gtk_widget_show (psp->pattern_preview);
    }
  else
    {
      gtk_widget_hide (psp->pattern_popup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (psp->preview->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  pattern_width = canvas_width (pattern->mask);
  pattern_height = canvas_height (pattern->mask);
  x = x_org + x - pattern_width * 0.5;
  y = y_org + y - pattern_height * 0.5;
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + pattern_width > scr_w) ? scr_w - pattern_width : x;
  y = (y + pattern_height > scr_h) ? scr_h - pattern_height : y;
  gtk_preview_size (GTK_PREVIEW (psp->pattern_preview), pattern_width, pattern_height);
  gtk_widget_popup (psp->pattern_popup, x, y);

  /*  Draw the pattern  */
  buf = g_new (gchar, pattern_width * 3);
  pattern_tag = canvas_tag (pattern->mask);
  pattern_prec = tag_precision (pattern_tag);
  src = canvas_portion_data (pattern->mask,0,0);
  rowbytes = canvas_portion_rowstride (pattern->mask,0,0);
  for (y = 0; y < pattern_height; y++)
    {
      (*display_pattern_get_row_funcs [pattern_prec-1]) (buf, pattern->mask, y, pattern_width); 
      gtk_preview_draw_row (GTK_PREVIEW (psp->pattern_preview), buf, 0, y, pattern_width);
    }
  g_free(buf);
  
  /*  Draw the pattern preview  */
  gtk_widget_draw (psp->pattern_preview, NULL);
}

static void
pattern_popup_close (PatternSelectP psp)
{
  if (psp->pattern_popup != NULL)
    gtk_widget_hide (psp->pattern_popup);
}

static void
display_pattern (PatternSelectP psp,
		 GPatternP      pattern,
		 int            col,
		 int            row)
{
  Canvas *pattern_buf_canvas = pattern->mask;
  unsigned char * buf;
  int cell_width, cell_height;
  int width, height;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i;
  Tag pattern_tag = canvas_tag (pattern->mask);
  Precision pattern_prec = tag_precision (pattern_tag);
  Alpha pattern_alpha = tag_alpha (pattern_tag);
  
  if ( pattern_alpha == ALPHA_YES )
    {
      g_warning("Pattern %s has alpha channel. Cant display.\n", pattern->name);
      return;
    }
  
  /*  get a 8bit channels buffer for display */
  buf = (unsigned char *) g_malloc (sizeof (char) * psp->cell_width * 3);

  /*  calculate the offset into the image  */
  cell_width = psp->cell_width - 2*MARGIN_WIDTH;
  cell_height = psp->cell_height - 2*MARGIN_HEIGHT;
  
  width = (canvas_width (pattern_buf_canvas) > cell_width) ? cell_width :
    canvas_width (pattern_buf_canvas);
  height = (canvas_height(pattern_buf_canvas) > cell_height) ? cell_height :
    canvas_height(pattern_buf_canvas);
  
  offset_x = col * psp->cell_width + ((cell_width - width) >> 1) + MARGIN_WIDTH;
  offset_y = row * psp->cell_height + ((cell_height - height) >> 1)
    - psp->scroll_offset + MARGIN_HEIGHT;

  ystart = BOUNDS (offset_y, 0, psp->preview->requisition.height);
  yend = BOUNDS (offset_y + height, 0, psp->preview->requisition.height);

  /*  Fill the pattern display buffer and display it */ 
  for (i = ystart; i < yend; i++)
    {
    (*display_pattern_get_row_funcs [pattern_prec-1]) (buf, pattern_buf_canvas, 
						       i - offset_y , width); 
      gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x, i, width);
    }
  g_free (buf);
}

static void  
display_pattern_get_row_u8(  
                        guchar *buf,
			Canvas *pattern_canvas,
			gint row,
                        gint width
		  )
{
  Tag tag = canvas_tag (pattern_canvas);
  gint channels_per_row = tag_num_channels (tag) * canvas_width (pattern_canvas);
  guint8 * s = (guint8*)canvas_portion_data (pattern_canvas,0,0) + row * channels_per_row;
  guchar *b = buf; 
  gint i;
  Format f = tag_format (canvas_tag (pattern_canvas));
  
  switch (f)
    {
    case FORMAT_RGB:
      for (i = 0; i < width; i++)
	{
	  *b++ = *s++;
	  *b++ = *s++;
	  *b++ = *s++;
	}
      break;	
    case FORMAT_GRAY:
      for (i = 0; i < width; i++)
        {
	  *b++ = *s;
	  *b++ = *s;
	  *b++ = *s++;
        }
      break;	
    default:
      break;
    }	
}
#define U16_TO_U8(x) ((x)>>8)
static void  
display_pattern_get_row_u16(  
                        guchar *buf,
			Canvas *pattern_canvas,
			gint row,
                        gint width
		  )
{
  Tag tag = canvas_tag (pattern_canvas);
  gint channels_per_row = tag_num_channels (tag) * canvas_width (pattern_canvas);
  guint16 * s = (guint16*)canvas_portion_data (pattern_canvas,0,0) + row * channels_per_row;
  guchar *b = buf; 
  gint i;
  Format f = tag_format (canvas_tag (pattern_canvas));
  
  switch (f)
    {
    case FORMAT_RGB:
      for (i = 0; i < width; i++)
	{
	  *b++ = U16_TO_U8(*s++);
	  *b++ = U16_TO_U8(*s++);
	  *b++ = U16_TO_U8(*s++);
	}
      break;	
    case FORMAT_GRAY:
      for (i = 0; i < width; i++)
        {
	  *b++ = U16_TO_U8(*s);
	  *b++ = U16_TO_U8(*s);
	  *b++ = U16_TO_U8(*s++);
        }
      break;	
    default:
      break;
    }	
}


static void  
display_pattern_get_row_float(  
                        guchar *buf,
			Canvas *pattern_canvas,
			gint row,
                        gint width
		  )
{
  Tag tag = canvas_tag (pattern_canvas);
  gint channels_per_row = tag_num_channels (tag) * canvas_width (pattern_canvas);
  gfloat * s = (gfloat*)canvas_portion_data (pattern_canvas,0,0) + row * channels_per_row;
  guchar *b = buf; 
  gint i;
  Format f = tag_format (canvas_tag (pattern_canvas));
  
  switch (f)
    {
    case FORMAT_RGB:
      for (i = 0; i < width; i++)
	{
	  *b++ = *s++ * 255.0 + .5;
	  *b++ = *s++ * 255.0 + .5;
	  *b++ = *s++ * 255.0 + .5;
	}
      break;	
    case FORMAT_GRAY:
      for (i = 0; i < width; i++)
        {
	  *b++ = *s * 255.0 + .5;
	  *b++ = *s * 255.0 + .5;
	  *b++ = *s++ * 255.0 + .5;
        }
      break;	
    default:
      break;
    }	
}


static void
display_setup (PatternSelectP psp)
{
  unsigned char * buf;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * psp->preview->requisition.width * 3);

  /*  Set the buffer to white  */
  memset (buf, 255, psp->preview->requisition.width * 3);

  /*  Set the image buffer to white  */
  for (i = 0; i < psp->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, 0, i, psp->preview->requisition.width);

  g_free (buf);
}

static void
display_patterns (PatternSelectP psp)
{
  GSList *list = pattern_list;    /*  the global pattern list  */
  int row, col;
  GPatternP pattern;

  /*  If there are no patterns, insensitize widgets  */
  if (pattern_list == NULL)
    {
      gtk_widget_set_sensitive (psp->options_box, FALSE);
      return;
    }
  /*  Else, sensitize widgets  */
  else
    gtk_widget_set_sensitive (psp->options_box, TRUE);

  /*  setup the display area  */
  display_setup (psp);

  row = col = 0;
  while (list)
    {
      pattern = (GPatternP) list->data;

      /*  Display the pattern  */
      display_pattern (psp, pattern, col, row);

      /*  increment the counts  */
      if (++col == NUM_PATTERN_COLUMNS)
	{
	  row ++;
	  col = 0;
	}

      list = g_slist_next (list);
    }
}

static void
pattern_select_show_selected (PatternSelectP psp,
			      int            row,
			      int            col)
{
  static int old_row = 0;
  static int old_col = 0;
  GdkRectangle area;
  unsigned char * buf;
  int yend;
  int ystart;
  int offset_x, offset_y;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * psp->cell_width * 3);

  if (old_col != col || old_row != row)
    {
      /*  remove the old selection  */
      offset_x = old_col * psp->cell_width;
      offset_y = old_row * psp->cell_height - psp->scroll_offset;

      ystart = BOUNDS (offset_y , 0, psp->preview->requisition.height);
      yend = BOUNDS (offset_y + psp->cell_height, 0, psp->preview->requisition.height);

      /*  set the buf to white  */
      memset (buf, 255, psp->cell_width * 3);

      for (i = ystart; i < yend; i++)
	{
	  if (i == offset_y || i == (offset_y + psp->cell_height - 1))
	    gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x, i, psp->cell_width);
	  else
	    {
	      gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x, i, 1);
	      gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x + psp->cell_width - 1, i, 1);
	    }
	}

      area.x = offset_x;
      area.y = ystart;
      area.width = psp->cell_width;
      area.height = yend - ystart;
      gtk_widget_draw (psp->preview, &area);
    }

  /*  make the new selection  */
  offset_x = col * psp->cell_width;
  offset_y = row * psp->cell_height - psp->scroll_offset;

  ystart = BOUNDS (offset_y , 0, psp->preview->requisition.height);
  yend = BOUNDS (offset_y + psp->cell_height, 0, psp->preview->requisition.height);

  /*  set the buf to black  */
  memset (buf, 0, psp->cell_width * 3);

  for (i = ystart; i < yend; i++)
    {
      if (i == offset_y || i == (offset_y + psp->cell_height - 1))
	gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x, i, psp->cell_width);
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x, i, 1);
	  gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x + psp->cell_width - 1, i, 1);
	}
    }

  area.x = offset_x;
  area.y = ystart;
  area.width = psp->cell_width;
  area.height = yend - ystart;
  gtk_widget_draw (psp->preview, &area);

  old_row = row;
  old_col = col;

  g_free (buf);
}

static void
draw_preview (PatternSelectP psp)
{
  /*  Draw the image buf to the preview window  */
  gtk_widget_draw (psp->preview, NULL);
}

static void
preview_calc_scrollbar (PatternSelectP psp)
{
  int num_rows;
  int page_size;
  int max;

  psp->scroll_offset = 0;
  num_rows = (num_patterns + NUM_PATTERN_COLUMNS - 1) / NUM_PATTERN_COLUMNS;
  max = num_rows * psp->cell_width;
  if (!num_rows) num_rows = 1;
  page_size = psp->preview->allocation.height;

  psp->sbar_data->value = psp->scroll_offset;
  psp->sbar_data->upper = max;
  psp->sbar_data->page_size = (page_size < max) ? page_size : max;
  psp->sbar_data->page_increment = (page_size >> 1);
  psp->sbar_data->step_increment = psp->cell_width;

  gtk_signal_emit_by_name (GTK_OBJECT (psp->sbar_data), "changed");
}

static void
update_active_pattern_field (PatternSelectP psp)
{
  GPatternP pattern;
  char buf[32];

  pattern = get_active_pattern ();

  if (!pattern)
    return;

  /*  Set pattern name  */
  gtk_label_set (GTK_LABEL (psp->pattern_name), pattern->name);

  /*  Set pattern size  */
  sprintf (buf, "(%d X %d)", canvas_width (pattern->mask), canvas_height (pattern->mask));
  gtk_label_set (GTK_LABEL (psp->pattern_size), buf);
}

static gint
pattern_select_resize (GtkWidget      *widget,
		       GdkEvent       *event,
		       PatternSelectP  psp)
{  
/* calculate the best-fit approximation... */  
  gint wid;
  gint now;

  wid = widget->allocation.width-4;

  for(now = MIN_CELL_SIZE, STD_CELL_SIZE = MIN_CELL_SIZE;
      now < MAX_CELL_SIZE; ++now)
    {
      if ((wid % now) < (wid % STD_CELL_SIZE)) STD_CELL_SIZE = now;
      if ((wid % STD_CELL_SIZE) == 0)
        break;
    }

  NUM_PATTERN_COLUMNS = wid / STD_CELL_SIZE;
  NUM_PATTERN_ROWS = (gint) (num_patterns + NUM_PATTERN_COLUMNS-1) / NUM_PATTERN_COLUMNS;

  psp->cell_width = STD_CELL_SIZE;
  psp->cell_height = STD_CELL_SIZE;
  psp->width = widget->allocation.width - 4;
  psp->height = widget->allocation.height - 4;

  /*
  NUM_PATTERN_COLUMNS=(gint)(widget->allocation.width/STD_CELL_WIDTH);
  NUM_PATTERN_ROWS = (num_patterns + NUM_PATTERN_COLUMNS - 1) / NUM_PATTERN_COLUMNS;
  */
/*  psp->width=widget->allocation.width;
  psp->height=widget->allocation.height; */

  gtk_preview_size (GTK_PREVIEW (psp->preview), psp->width, psp->height);
  /*  recalculate scrollbar extents  */
  preview_calc_scrollbar (psp);

  /*  render the patterns into the newly created image structure  */
  display_patterns (psp);

  /*  update the active selection  */
/*  active = get_active_pattern ();
  if (active)
    pattern_select_select (psp, active->index); */

  /*  update the display  */
  draw_preview (psp);
  return FALSE;
}

static gint
pattern_select_events (GtkWidget      *widget,
		       GdkEvent       *event,
		       PatternSelectP  psp)
{
  GdkEventButton *bevent;
  GPatternP pattern;
  int row, col, index;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  col = bevent->x / psp->cell_width;
	  row = (bevent->y + psp->scroll_offset) / psp->cell_height;
	  index = row * NUM_PATTERN_COLUMNS + col;

	  /*  Get the pattern and display the popup pattern preview  */
	  if ((pattern = get_pattern_by_index (index)))
	    {
	      gdk_pointer_grab (psp->preview->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	      /*  Make this pattern the active pattern  */
	      select_pattern (pattern);
	      /*  Show the pattern popup window if the pattern is too large  */
	      if (canvas_width (pattern->mask) > psp->cell_width ||
		  canvas_height (pattern->mask) > psp->cell_height)
		pattern_popup_open (psp, bevent->x, bevent->y, pattern);
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  /*  Ungrab the pointer  */
	  gdk_pointer_ungrab (bevent->time);

	  /*  Close the brush popup window  */
	  pattern_popup_close (psp);
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
pattern_select_delete_callback (GtkWidget *w,
				GdkEvent *e,
				gpointer client_data)
{
  pattern_select_close_callback (w, client_data);

  return TRUE;
}

static void
pattern_select_close_callback (GtkWidget *w,
			       gpointer   client_data)
{
  PatternSelectP psp;
  psp = (PatternSelectP) client_data;
  if (GTK_WIDGET_VISIBLE (psp->shell))
    gtk_widget_hide (psp->shell);
}

static void
pattern_select_refresh_callback (GtkWidget *w,
				 gpointer   client_data)
{
  PatternSelectP psp;
  GPatternP active;

  psp = (PatternSelectP) client_data;

  /*  re-init the pattern list  */
  patterns_init (FALSE);

  /*  recalculate scrollbar extents  */
  preview_calc_scrollbar (psp);

  /*  render the patterns into the newly created image structure  */
  display_patterns (psp);

  /*  update the active selection  */
  active = get_active_pattern ();
  if (active)
    pattern_select_select (psp, active->index);

  /*  update the display  */
  draw_preview (psp);
}

static void
pattern_select_scroll_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  PatternSelectP psp;
  GPatternP active;
  int row, col;

  psp = data;

  if (psp)
    {
      psp->scroll_offset = adjustment->value;
      display_patterns (psp);

      active = get_active_pattern ();
      if (active)
	{
	  row = active->index / NUM_PATTERN_COLUMNS;
	  col = active->index - row * NUM_PATTERN_COLUMNS;
	  pattern_select_show_selected (psp, row, col);
	}

      draw_preview (psp);
    }
}
