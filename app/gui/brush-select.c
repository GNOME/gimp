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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "actionarea.h"
#include "brushes.h"
#include "brush_select.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "disp_callbacks.h"
#include "errors.h"
#include "paint_funcs.h"


#define STD_CELL_WIDTH    24
#define STD_CELL_HEIGHT   24

#define STD_BRUSH_COLUMNS 5
#define STD_BRUSH_ROWS    5

#define MAX_WIN_WIDTH     (STD_CELL_WIDTH * NUM_BRUSH_COLUMNS)
#define MAX_WIN_HEIGHT    (STD_CELL_HEIGHT * NUM_BRUSH_ROWS)
#define MARGIN_WIDTH      3
#define MARGIN_HEIGHT     3

#define BRUSH_EVENT_MASK  GDK_EXPOSURE_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
			  GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
			  GDK_ENTER_NOTIFY_MASK

/*  local function prototypes  */
static void brush_popup_open             (BrushSelectP, int, int, GBrushP);
static void brush_popup_close            (BrushSelectP);
static void display_brush                (BrushSelectP, GBrushP, int, int);
static void display_brushes              (BrushSelectP);
static void display_setup                (BrushSelectP);
static void preview_calc_scrollbar       (BrushSelectP);
static void brush_select_show_selected   (BrushSelectP, int, int);
static void update_active_brush_field    (BrushSelectP);
static void brush_select_close_callback  (GtkWidget *, gpointer);
static void brush_select_refresh_callback(GtkWidget *, gpointer);
static void paint_mode_menu_callback     (GtkWidget *, gpointer);
static gint brush_select_events          (GtkWidget *, GdkEvent *, BrushSelectP);
static gint brush_select_resize		 (GtkWidget *, GdkEvent *, BrushSelectP);

static gint brush_select_delete_callback        (GtkWidget *, GdkEvent *, gpointer);
static void preview_scroll_update        (GtkAdjustment *, gpointer);
static void opacity_scale_update         (GtkAdjustment *, gpointer);
static void spacing_scale_update         (GtkAdjustment *, gpointer);

/*  the option menu items -- the paint modes  */
static MenuItem option_items[] =
{
  { "Normal", 0, 0, paint_mode_menu_callback, (gpointer) NORMAL_MODE, NULL, NULL },
  { "Dissolve", 0, 0, paint_mode_menu_callback, (gpointer) DISSOLVE_MODE, NULL, NULL },
  { "Behind", 0, 0, paint_mode_menu_callback, (gpointer) BEHIND_MODE, NULL, NULL },
  { "Multiply (Burn)", 0, 0, paint_mode_menu_callback, (gpointer) MULTIPLY_MODE, NULL, NULL },
  { "Divide (Dodge)", 0, 0, paint_mode_menu_callback, (gpointer) DIVIDE_MODE, NULL, NULL },
  { "Screen", 0, 0, paint_mode_menu_callback, (gpointer) SCREEN_MODE, NULL, NULL },
  { "Overlay", 0, 0, paint_mode_menu_callback, (gpointer) OVERLAY_MODE, NULL, NULL },
  { "Difference", 0, 0, paint_mode_menu_callback, (gpointer) DIFFERENCE_MODE, NULL, NULL },
  { "Addition", 0, 0, paint_mode_menu_callback, (gpointer) ADDITION_MODE, NULL, NULL },
  { "Subtract", 0, 0, paint_mode_menu_callback, (gpointer) SUBTRACT_MODE, NULL, NULL },
  { "Darken Only", 0, 0, paint_mode_menu_callback, (gpointer) DARKEN_ONLY_MODE, NULL, NULL },
  { "Lighten Only", 0, 0, paint_mode_menu_callback, (gpointer) LIGHTEN_ONLY_MODE, NULL, NULL },
  { "Hue", 0, 0, paint_mode_menu_callback, (gpointer) HUE_MODE, NULL, NULL },
  { "Saturation", 0, 0, paint_mode_menu_callback, (gpointer) SATURATION_MODE, NULL, NULL },
  { "Color", 0, 0, paint_mode_menu_callback, (gpointer) COLOR_MODE, NULL, NULL },
  { "Value", 0, 0, paint_mode_menu_callback, (gpointer) VALUE_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Close", brush_select_close_callback, NULL, NULL },
  { "Refresh", brush_select_refresh_callback, NULL, NULL },
};

static double old_opacity;
static int old_spacing;
static int old_paint_mode;

int NUM_BRUSH_COLUMNS=5;
int NUM_BRUSH_ROWS=5;


GtkWidget *
create_paint_mode_menu (MenuItemCallback callback)
{
  GtkWidget *menu;
  int i;

  for (i = 0; i <= VALUE_MODE; i++)
    option_items[i].callback = callback;

  menu = build_menu (option_items, NULL);

  return menu;
}


BrushSelectP
brush_select_new ()
{
  BrushSelectP bsp;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *sbar;
  GtkWidget *label;
  GtkWidget *menu;
  GtkWidget *util_box;
  GtkWidget *option_menu;
  GtkWidget *slider;
  GBrushP active;

  bsp = g_malloc (sizeof (_BrushSelect));
  bsp->redraw = TRUE;
  bsp->scroll_offset = 0;
  bsp->brush_popup = NULL;

  /*  The shell and main vbox  */
  bsp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bsp->shell), "brushselection", "Gimp");
  gtk_window_set_title (GTK_WINDOW (bsp->shell), "Brush Selection");
  gtk_window_set_policy(GTK_WINDOW(bsp->shell), FALSE, TRUE, FALSE);
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bsp->shell)->vbox), vbox, TRUE, TRUE, 0);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (bsp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (brush_select_delete_callback),
		      bsp);

  /*  The horizontal box containing preview & scrollbar & options box */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  bsp->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (bsp->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), bsp->frame, TRUE, TRUE, 0);
  bsp->sbar_data = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, MAX_WIN_HEIGHT, 1, 1, MAX_WIN_HEIGHT));
  sbar = gtk_vscrollbar_new (bsp->sbar_data);
  gtk_signal_connect (GTK_OBJECT (bsp->sbar_data), "value_changed",
		      (GtkSignalFunc) preview_scroll_update, bsp);
  gtk_box_pack_start (GTK_BOX (hbox), sbar, FALSE, FALSE, 0);


  /*  Create the brush preview window and the underlying image  */
  /*  Get the maximum brush extents  */

  bsp->cell_width = STD_CELL_WIDTH;
  bsp->cell_height = STD_CELL_HEIGHT;

  bsp->width = MAX_WIN_WIDTH;
  bsp->height = MAX_WIN_HEIGHT;

  bsp->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (bsp->preview), bsp->width, bsp->height);
  gtk_widget_set_events (bsp->preview, BRUSH_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (bsp->preview), "event",
		      (GtkSignalFunc) brush_select_events,
		      bsp);
  gtk_signal_connect_after (GTK_OBJECT(bsp->frame), "size_allocate",
		       (GtkSignalFunc) brush_select_resize,
		       bsp);

  gtk_container_add (GTK_CONTAINER (bsp->frame), bsp->preview);
  gtk_widget_show (bsp->preview);

  gtk_widget_show (sbar);
  gtk_widget_show (bsp->frame);

  /*  options box  */
  bsp->options_box = gtk_vbox_new (TRUE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), bsp->options_box, TRUE, TRUE, 0);

  /*  Create the active brush label  */
  util_box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);

  bsp->brush_name = gtk_label_new ("Active");
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_name, FALSE, FALSE, 2);
  bsp->brush_size = gtk_label_new ("(0x0)");
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_size, FALSE, FALSE, 2);

  gtk_widget_show (bsp->brush_name);
  gtk_widget_show (bsp->brush_size);
  gtk_widget_show (util_box);

  /*  Create the paint mode option menu  */
  old_paint_mode = get_brush_paint_mode ();

  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new ("Mode:");
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  menu = create_paint_mode_menu (paint_mode_menu_callback);
  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (util_box), option_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (option_menu);
  gtk_widget_show (util_box);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  /*  Create the opacity scale widget  */
  old_opacity = get_brush_opacity ();

  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new ("Opacity:");
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  bsp->opacity_data = GTK_ADJUSTMENT (gtk_adjustment_new (100.0, 0.0, 100.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (bsp->opacity_data);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (bsp->opacity_data), "value_changed",
		      (GtkSignalFunc) opacity_scale_update, bsp);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (util_box);

  /*  Create the brush spacing scale widget  */
  old_spacing = get_brush_spacing ();

  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new ("Spacing:");
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  bsp->spacing_data = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1000.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (bsp->spacing_data);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (bsp->spacing_data), "value_changed",
		      (GtkSignalFunc) spacing_scale_update, bsp);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (util_box);

  /*  The action area  */
  action_items[0].user_data = bsp;
  action_items[1].user_data = bsp;
  build_action_area (GTK_DIALOG (bsp->shell), action_items, 2, 0);

  gtk_widget_show (bsp->options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (bsp->shell);

  /* calculate the scrollbar */
  if(no_data)
    brushes_init(FALSE);
  /* This is done by size_allocate anyway, which is much better */
  preview_calc_scrollbar (bsp);


  /*  render the brushes into the newly created image structure  */
  display_brushes (bsp);

  /*  update the active selection  */
  active = get_active_brush ();
  if (active)
    {
      int old_value = bsp->redraw;
      bsp->redraw = FALSE;
      brush_select_select (bsp, active->index);
      bsp->redraw = old_value;
    }

  return bsp;
}


void
brush_select_select (BrushSelectP bsp,
		     int          index)
{
  int row, col;

  update_active_brush_field (bsp);
  row = index / NUM_BRUSH_COLUMNS;
  col = index - row * NUM_BRUSH_COLUMNS;

  brush_select_show_selected (bsp, row, col);
}


void
brush_select_free (BrushSelectP bsp)
{
  if (bsp)
    {
      if (bsp->brush_popup != NULL)
	gtk_widget_destroy (bsp->brush_popup);
      g_free (bsp);
    }
}


/*
 *  Local functions
 */
static void
brush_popup_open (BrushSelectP bsp,
		  int          x,
		  int          y,
		  GBrushP      brush)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;

  /* make sure the popup exists and is not visible */
  if (bsp->brush_popup == NULL)
    {
      GtkWidget *frame;

      bsp->brush_popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (bsp->brush_popup), FALSE, FALSE, TRUE);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (bsp->brush_popup), frame);
      gtk_widget_show (frame);
      bsp->brush_preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
      gtk_container_add (GTK_CONTAINER (frame), bsp->brush_preview);
      gtk_widget_show (bsp->brush_preview);
    }
  else
    {
      gtk_widget_hide (bsp->brush_popup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (bsp->preview->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - brush->mask->width * 0.5;
  y = y_org + y - brush->mask->height * 0.5;
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + brush->mask->width > scr_w) ? scr_w - brush->mask->width : x;
  y = (y + brush->mask->height > scr_h) ? scr_h - brush->mask->height : y;
  gtk_preview_size (GTK_PREVIEW (bsp->brush_preview), brush->mask->width, brush->mask->height);

  gtk_widget_popup (bsp->brush_popup, x, y);
  
  /*  Draw the brush  */
  buf = g_new (gchar, brush->mask->width);
  src = (gchar *)temp_buf_data (brush->mask);
  for (y = 0; y < brush->mask->height; y++)
    {
      /*  Invert the mask for display.  We're doing this because
       *  a value of 255 in the  mask means it is full intensity.
       *  However, it makes more sense for full intensity to show
       *  up as black in this brush preview window...
       */
      for (x = 0; x < brush->mask->width; x++)
	buf[x] = 255 - src[x];
      gtk_preview_draw_row (GTK_PREVIEW (bsp->brush_preview), (guchar *)buf, 0, y, brush->mask->width);
      src += brush->mask->width;
    }
  g_free(buf);
  
  /*  Draw the brush preview  */
  gtk_widget_draw (bsp->brush_preview, NULL);
}


static void
brush_popup_close (BrushSelectP bsp)
{
  if (bsp->brush_popup != NULL)
    gtk_widget_hide (bsp->brush_popup);
}
static void
display_brush (BrushSelectP bsp,
	       GBrushP      brush,
	       int          col,
	       int          row)
{
  TempBuf * brush_buf;
  unsigned char * src, *s;
  unsigned char * buf, *b;
  int width, height;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i, j;

  buf = (unsigned char *) g_malloc (sizeof (char) * bsp->cell_width);

  brush_buf = brush->mask;

  /*  calculate the offset into the image  */
  width = (brush_buf->width > bsp->cell_width) ? bsp->cell_width :
    brush_buf->width;
  height = (brush_buf->height > bsp->cell_height) ? bsp->cell_height :
    brush_buf->height;

  offset_x = col * bsp->cell_width + ((bsp->cell_width - width) >> 1);
  offset_y = row * bsp->cell_height + ((bsp->cell_height - height) >> 1)
    - bsp->scroll_offset;

  ystart = BOUNDS (offset_y, 0, bsp->preview->requisition.height);
  yend = BOUNDS (offset_y + height, 0, bsp->preview->requisition.height);

  /*  Get the pointer into the brush mask data  */
  src = mask_buf_data (brush_buf) + (ystart - offset_y) * brush_buf->width;

  for (i = ystart; i < yend; i++)
    {
      /*  Invert the mask for display.  We're doing this because
       *  a value of 255 in the  mask means it is full intensity.
       *  However, it makes more sense for full intensity to show
       *  up as black in this brush preview window...
       */
      s = src;
      b = buf;
      for (j = 0; j < width; j++)
	*b++ = 255 - *s++;

      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x, i, width);

      src += brush_buf->width;
    }

  g_free (buf);
}


static void
display_setup (BrushSelectP bsp)
{
  unsigned char * buf;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * bsp->preview->requisition.width);

  /*  Set the buffer to white  */
  memset (buf, 255, bsp->preview->requisition.width);

  /*  Set the image buffer to white  */
  for (i = 0; i < bsp->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, 0, i, bsp->preview->requisition.width);

  g_free (buf);
}


static void
display_brushes (BrushSelectP bsp)
{
  GSList * list = brush_list;    /*  the global brush list  */
  int row, col;
  GBrushP brush;

  /*  If there are no brushes, insensitize widgets  */
  if (brush_list == NULL)
    {
      gtk_widget_set_sensitive (bsp->options_box, FALSE);
      return;
    }
  /*  Else, sensitize widgets  */
  else
    gtk_widget_set_sensitive (bsp->options_box, TRUE);

  /*  setup the display area  */
  display_setup (bsp);

  row = col = 0;
  while (list)
    {
      brush = (GBrushP) list->data;

      /*  Display the brush  */
      display_brush (bsp, brush, col, row);

      /*  increment the counts  */
      if (++col == NUM_BRUSH_COLUMNS)
	{
	  row ++;
	  col = 0;
	}

      list = g_slist_next (list);
    }
}


static void
brush_select_show_selected (BrushSelectP bsp,
			    int          row,
			    int          col)
{
  GdkRectangle area;
  static int old_row = 0;
  static int old_col = 0;
  unsigned char * buf;
  int yend;
  int ystart;
  int offset_x, offset_y;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * bsp->cell_width);

  if (old_col != col || old_row != row)
    {
      /*  remove the old selection  */
      offset_x = old_col * bsp->cell_width;
      offset_y = old_row * bsp->cell_height - bsp->scroll_offset;

      ystart = BOUNDS (offset_y , 0, bsp->preview->requisition.height);
      yend = BOUNDS (offset_y + bsp->cell_height, 0, bsp->preview->requisition.height);

      /*  set the buf to white  */
      memset (buf, 255, bsp->cell_width);

      for (i = ystart; i < yend; i++)
	{
	  if (i == offset_y || i == (offset_y + bsp->cell_height - 1))
	    gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x, i, bsp->cell_width);
	  else
	    {
	      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x, i, 1);
	      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x + bsp->cell_width - 1, i, 1);
	    }
	}

      if (bsp->redraw)
	{
	  area.x = offset_x;
	  area.y = ystart;
	  area.width = bsp->cell_width;
	  area.height = (yend - ystart);
	  gtk_widget_draw (bsp->preview, &area);
	}
    }

  /*  make the new selection  */
  offset_x = col * bsp->cell_width;
  offset_y = row * bsp->cell_height - bsp->scroll_offset;

  ystart = BOUNDS (offset_y , 0, bsp->preview->requisition.height);
  yend = BOUNDS (offset_y + bsp->cell_height, 0, bsp->preview->requisition.height);

  /*  set the buf to black  */
  memset (buf, 0, bsp->cell_width);

  for (i = ystart; i < yend; i++)
    {
      if (i == offset_y || i == (offset_y + bsp->cell_height - 1))
	gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x, i, bsp->cell_width);
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x, i, 1);
	  gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, offset_x + bsp->cell_width - 1, i, 1);
	}
    }

  if (bsp->redraw)
    {
      area.x = offset_x;
      area.y = ystart;
      area.width = bsp->cell_width;
      area.height = (yend - ystart);
      gtk_widget_draw (bsp->preview, &area);
    }

  old_row = row;
  old_col = col;

  g_free (buf);
}


static void
preview_calc_scrollbar (BrushSelectP bsp)
{
  int num_rows;
  int page_size;
  int max;
  int offs;
  /* int rowy; */

  offs = bsp->scroll_offset;
  num_rows = (num_brushes + NUM_BRUSH_COLUMNS - 1) / NUM_BRUSH_COLUMNS;
  max = num_rows * bsp->cell_width;
  if (!num_rows) num_rows = 1;
  page_size = bsp->preview->allocation.height;
  page_size = ((page_size < max) ? page_size : max);
  /*
  rowy = (get_active_brush()->index / NUM_BRUSH_COLUMNS) * bsp->cell_width
	 + bsp->cell_width/2;
  if((rowy < offs) || (rowy > (offs + page_size)))
    offs = rowy - page_size/2;
  offs = MIN(MAX(offs, 0), max - page_size);
  */

  bsp->scroll_offset = offs;
  bsp->sbar_data->value = bsp->scroll_offset;
  bsp->sbar_data->upper = max;
  /* bsp->sbar_data->page_size = page_size; */
  bsp->sbar_data->page_size = ((page_size < max) ? page_size : max);
  bsp->sbar_data->page_increment = (page_size >> 1);
  bsp->sbar_data->step_increment = bsp->cell_width;

  gtk_signal_emit_by_name (GTK_OBJECT (bsp->sbar_data), "changed");
}

static gint 
brush_select_resize (GtkWidget *widget, 
		     GdkEvent *event, 
		     BrushSelectP bsp)
{
   NUM_BRUSH_COLUMNS = (gint)((widget->allocation.width - 4) / STD_CELL_WIDTH);
   NUM_BRUSH_ROWS = (num_brushes + NUM_BRUSH_COLUMNS - 1) / NUM_BRUSH_COLUMNS;
   
   bsp->width = widget->allocation.width - 4;
   bsp->height = widget->allocation.height - 4;
 
   gtk_preview_size (GTK_PREVIEW (bsp->preview), bsp->width, bsp->height);
   
   /*  recalculate scrollbar extents  */
   preview_calc_scrollbar (bsp);
 
   /*  render the patterns into the newly created image structure  */
   display_brushes (bsp);
 
   /*  update the display  */   
   if (bsp->redraw)
	gtk_widget_draw (bsp->preview, NULL);
   
   return FALSE;
}
 
static void
update_active_brush_field (BrushSelectP bsp)
{
  GBrushP brush;
  char buf[32];

  brush = get_active_brush ();

  if (!brush)
    return;

  /*  Set brush name  */
  gtk_label_set (GTK_LABEL (bsp->brush_name), brush->name);

  /*  Set brush size  */
  sprintf (buf, "(%d X %d)", brush->mask->width, brush->mask->height);
  gtk_label_set (GTK_LABEL (bsp->brush_size), buf);

  /*  Set brush spacing  */
  bsp->spacing_data->value = get_brush_spacing ();
  gtk_signal_emit_by_name (GTK_OBJECT (bsp->spacing_data), "value_changed");
}


static gint
brush_select_events (GtkWidget    *widget,
		     GdkEvent     *event,
		     BrushSelectP  bsp)
{
  GdkEventButton *bevent;
  GBrushP brush;
  int row, col, index;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  col = bevent->x / bsp->cell_width;
	  row = (bevent->y + bsp->scroll_offset) / bsp->cell_height;
	  index = row * NUM_BRUSH_COLUMNS + col;

	  /*  Get the brush and display the popup brush preview  */
	  if ((brush = get_brush_by_index (index)))
	    {
	      gdk_pointer_grab (bsp->preview->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	      /*  Make this brush the active brush  */
	      select_brush (brush);

	      /*  Show the brush popup window if the brush is too large  */
	      if (brush->mask->width > bsp->cell_width ||
		  brush->mask->height > bsp->cell_height)
		brush_popup_open (bsp, bevent->x, bevent->y, brush);
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
	  brush_popup_close (bsp);
	}
      break;
    case GDK_DELETE:
      /* g_warning ("test"); */
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
brush_select_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  brush_select_close_callback (w, data);

  return TRUE;
}

static void
brush_select_close_callback (GtkWidget *w,
			     gpointer   client_data)
{
  BrushSelectP bsp;

  bsp = (BrushSelectP) client_data;

  old_paint_mode = get_brush_paint_mode ();
  old_opacity = get_brush_opacity ();
  old_spacing = get_brush_spacing ();

  if (GTK_WIDGET_VISIBLE (bsp->shell))
    gtk_widget_hide (bsp->shell);
}


static void
brush_select_refresh_callback (GtkWidget *w,
			       gpointer   client_data)
{
  BrushSelectP bsp;
  GBrushP active;

  bsp = (BrushSelectP) client_data;

  /*  re-init the brush list  */
  brushes_init(FALSE);

  /*  update the active selection  */
  active = get_active_brush ();
  if (active)
    brush_select_select (bsp, active->index);

  /*  recalculate scrollbar extents  */
  preview_calc_scrollbar (bsp);

  /*  render the brushes into the newly created image structure  */
  display_brushes (bsp);

 

  /*  update the display  */
  if (bsp->redraw)
    gtk_widget_draw (bsp->preview, NULL);
}


static void
preview_scroll_update (GtkAdjustment *adjustment,
		       gpointer       data)
{
  BrushSelectP bsp;
  GBrushP active;
  int row, col;

  bsp = data;

  if (bsp)
    {
      bsp->scroll_offset = adjustment->value;
      display_brushes (bsp);

      active = get_active_brush ();
      if (active)
	{
	  row = active->index / NUM_BRUSH_COLUMNS;
	  col = active->index - row * NUM_BRUSH_COLUMNS;
	  brush_select_show_selected (bsp, row, col);
	}

      if (bsp->redraw)
	gtk_widget_draw (bsp->preview, NULL);
    }
}

static void
paint_mode_menu_callback (GtkWidget *w,
			  gpointer   client_data)
{
  set_brush_paint_mode ((long) client_data);
}


static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  set_brush_opacity (adjustment->value / 100.0);
}


static void
spacing_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  set_brush_spacing ((int) adjustment->value);
}
