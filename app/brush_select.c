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
#include "gimpbrushlist.h"
#include "gimplist.h"
#include "gimpbrushgenerated.h"
#include "brush_edit.h"
#include "brush_select.h"
#include "brush_select.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "disp_callbacks.h"
#include "errors.h"
#include "paint_funcs.h"
#include "session.h"

#include "libgimp/gimpintl.h"

#define STD_CELL_WIDTH    24
#define STD_CELL_HEIGHT   24

#define STD_BRUSH_COLUMNS 5
#define STD_BRUSH_ROWS    5

#define MAX_WIN_WIDTH(p)     (STD_CELL_WIDTH * ((p)->NUM_BRUSH_COLUMNS))
#define MAX_WIN_HEIGHT(p)    (STD_CELL_HEIGHT * ((p)->NUM_BRUSH_ROWS))
#define MARGIN_WIDTH      3
#define MARGIN_HEIGHT     3

#define BRUSH_EVENT_MASK  GDK_EXPOSURE_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
			  GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
			  GDK_ENTER_NOTIFY_MASK

/*  local function prototypes  */
static void brush_popup_open             (BrushSelectP, int, int, GimpBrushP);
static void brush_popup_close            (BrushSelectP);
static void display_brush                (BrushSelectP, GimpBrushP, int, int);
static void display_brushes              (BrushSelectP);
static void display_setup                (BrushSelectP);
static void preview_calc_scrollbar       (BrushSelectP);
static void brush_select_show_selected   (BrushSelectP, int, int);
static void update_active_brush_field    (BrushSelectP);
static gint edit_brush_callback		(GtkWidget *w, GdkEvent *e,
					 gpointer data);
static gint new_brush_callback		(GtkWidget *w, GdkEvent *e,
					 gpointer data);
static gint brush_select_brush_dirty_callback(GimpBrushP brush,
					      BrushSelectP bsp);
static void connect_signals_to_brush    (GimpBrushP brush, 
					 BrushSelectP bsp);
static void disconnect_signals_from_brush(GimpBrushP brush,
					  BrushSelectP bsp);
static void brush_added_callback     (GimpBrushList *list,
				      GimpBrushP brush,
				      BrushSelectP bsp);
static void brush_removed_callback   (GimpBrushList *list,
				      GimpBrushP brush,
				      BrushSelectP bsp);
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
  { N_("Normal"), 0, 0, paint_mode_menu_callback, (gpointer) NORMAL_MODE, NULL, NULL },
  { N_("Dissolve"), 0, 0, paint_mode_menu_callback, (gpointer) DISSOLVE_MODE, NULL, NULL },
  { N_("Behind"), 0, 0, paint_mode_menu_callback, (gpointer) BEHIND_MODE, NULL, NULL },
  { N_("Multiply (Burn)"), 0, 0, paint_mode_menu_callback, (gpointer) MULTIPLY_MODE, NULL, NULL },
  { N_("Divide (Dodge)"), 0, 0, paint_mode_menu_callback, (gpointer) DIVIDE_MODE, NULL, NULL },
  { N_("Screen"), 0, 0, paint_mode_menu_callback, (gpointer) SCREEN_MODE, NULL, NULL },
  { N_("Overlay"), 0, 0, paint_mode_menu_callback, (gpointer) OVERLAY_MODE, NULL, NULL },
  { N_("Difference"), 0, 0, paint_mode_menu_callback, (gpointer) DIFFERENCE_MODE, NULL, NULL },
  { N_("Addition"), 0, 0, paint_mode_menu_callback, (gpointer) ADDITION_MODE, NULL, NULL },
  { N_("Subtract"), 0, 0, paint_mode_menu_callback, (gpointer) SUBTRACT_MODE, NULL, NULL },
  { N_("Darken Only"), 0, 0, paint_mode_menu_callback, (gpointer) DARKEN_ONLY_MODE, NULL, NULL },
  { N_("Lighten Only"), 0, 0, paint_mode_menu_callback, (gpointer) LIGHTEN_ONLY_MODE, NULL, NULL },
  { N_("Hue"), 0, 0, paint_mode_menu_callback, (gpointer) HUE_MODE, NULL, NULL },
  { N_("Saturation"), 0, 0, paint_mode_menu_callback, (gpointer) SATURATION_MODE, NULL, NULL },
  { N_("Color"), 0, 0, paint_mode_menu_callback, (gpointer) COLOR_MODE, NULL, NULL },
  { N_("Value"), 0, 0, paint_mode_menu_callback, (gpointer) VALUE_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { N_("Refresh"), brush_select_refresh_callback, NULL, NULL },
  { N_("Close"), brush_select_close_callback, NULL, NULL }
};

static BrushEditGeneratedWindow *brush_edit_generated_dialog;

/* PDB interface data */
static int          success;

static GSList *active_dialogs = NULL; /* List of active dialogs */


extern BrushSelectP brush_select_dialog; /* The main brush dialog */


GtkWidget *
create_paint_mode_menu (MenuItemCallback callback, gpointer udata)
{
  GtkWidget *menu;
  int i;

  for (i = 0; i <= VALUE_MODE; i++)
    option_items[i].callback = callback;

  menu = build_menu (option_items, NULL);

  for (i = 0; i <= VALUE_MODE; i++)
    gtk_object_set_user_data(GTK_OBJECT(option_items[i].widget),udata);

  return menu;
}


/* If title is null then it is the main brush dialog */

BrushSelectP
brush_select_new (gchar * title,
		  gchar *init_name, /* These are the required initial vals*/
		  gdouble init_opacity, /* If init_name == NULL then 
					 * use current brush 
					 */
		  gint init_spacing,
		  gint init_mode)
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
  GimpBrushP active = NULL;
  GtkWidget *button2;
  gint gotinitbrush = FALSE;

  bsp = g_malloc (sizeof (_BrushSelect));
  bsp->redraw = TRUE;
  bsp->scroll_offset = 0;
  bsp->callback_name = 0;
  bsp->old_row = bsp->old_col = 0;
  bsp->brush = NULL; /* NULL -> main dialog window */
  bsp->brush_popup = NULL;
  bsp->NUM_BRUSH_COLUMNS = STD_BRUSH_COLUMNS;
  bsp->NUM_BRUSH_ROWS = STD_BRUSH_ROWS;

  /*  The shell and main vbox  */
  bsp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bsp->shell), "brushselection", "Gimp");

  if(!title)
    {
      gtk_window_set_title (GTK_WINDOW (bsp->shell), _("Brush Selection"));
      session_set_window_geometry (bsp->shell, &brush_select_session_info, TRUE);
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (bsp->shell), title);
      if(init_name && strlen(init_name))
	 active = gimp_brush_list_get_brush(brush_list, init_name);
      if(active)
	gotinitbrush = TRUE;
    }

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
  bsp->sbar_data = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, MAX_WIN_HEIGHT(bsp), 1, 1, MAX_WIN_HEIGHT(bsp)));
  sbar = gtk_vscrollbar_new (bsp->sbar_data);
  gtk_signal_connect (GTK_OBJECT (bsp->sbar_data), "value_changed",
		      (GtkSignalFunc) preview_scroll_update, bsp);
  gtk_box_pack_start (GTK_BOX (hbox), sbar, FALSE, FALSE, 0);


  /*  Create the brush preview window and the underlying image  */
  /*  Get the maximum brush extents  */

  bsp->cell_width = STD_CELL_WIDTH;
  bsp->cell_height = STD_CELL_HEIGHT;

  bsp->width = MAX_WIN_WIDTH(bsp);
  bsp->height = MAX_WIN_HEIGHT(bsp);

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

  bsp->brush_name = gtk_label_new (_("Active"));
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_name, FALSE, FALSE, 2);
  bsp->brush_size = gtk_label_new ("(0x0)");
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_size, FALSE, FALSE, 2);

  gtk_widget_show (bsp->brush_name);
  gtk_widget_show (bsp->brush_size);
  gtk_widget_show (util_box);

  /*  Create the paint mode option menu  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  menu = create_paint_mode_menu (paint_mode_menu_callback,(gpointer)bsp);
  bsp->option_menu =
    option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (util_box), option_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (option_menu);
  gtk_widget_show (util_box);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  /*  Create the opacity scale widget  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new (_("Opacity:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  bsp->opacity_data = 
    GTK_ADJUSTMENT (gtk_adjustment_new ((active)?(init_opacity*100.0):100.0, 0.0, 100.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (bsp->opacity_data);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (bsp->opacity_data), "value_changed",
		      (GtkSignalFunc) opacity_scale_update, bsp);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (util_box);

  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new (_("Spacing:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  bsp->spacing_data = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 1000.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (bsp->spacing_data);
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (bsp->spacing_data), "value_changed",
		      (GtkSignalFunc) spacing_scale_update, bsp);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (util_box);

  /*  Create the edit/new buttons  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 0);
  bsp->edit_button =  gtk_button_new_with_label (_("Edit Brush"));
  gtk_signal_connect (GTK_OBJECT (bsp->edit_button), "clicked",
		      (GtkSignalFunc) edit_brush_callback,
		      NULL);

  /* We can only edit in the main window! (for now)*/

  gtk_box_pack_start (GTK_BOX (util_box), bsp->edit_button, TRUE, TRUE, 5);

  button2 =  gtk_button_new_with_label (_("New Brush"));
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
		      (GtkSignalFunc) new_brush_callback,
		      NULL);
  gtk_box_pack_start (GTK_BOX (util_box), button2, TRUE, TRUE, 5);

  gtk_widget_show (bsp->edit_button);    
  gtk_widget_show (button2);
  gtk_widget_show (util_box);

  if(title)
    {
      gtk_widget_set_sensitive(bsp->edit_button,FALSE);
      gtk_widget_set_sensitive(button2,FALSE);
    }

  /*  The action area  */
  action_items[0].user_data = bsp;
  action_items[1].user_data = bsp;
  build_action_area (GTK_DIALOG (bsp->shell), action_items, 2, 1);

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

  /*  add callbacks to keep the display area current  */
  /* Only for main dialog */

  if(!title)
    {
      gimp_list_foreach(GIMP_LIST(brush_list), (GFunc)connect_signals_to_brush,
			bsp);
      gtk_signal_connect (GTK_OBJECT (brush_list), "add",
			  (GtkSignalFunc) brush_added_callback,
			  bsp);
      gtk_signal_connect (GTK_OBJECT (brush_list), "remove",
			  (GtkSignalFunc) brush_removed_callback,
			  bsp);
    }
  
  /*  update the active selection  */
  if(!active)
    active = get_active_brush ();

  if(title)
    {
      bsp->brush = active;
    }

  if (active)
    {
      int old_value = bsp->redraw;
      bsp->redraw = FALSE;
      if(!gotinitbrush)
	{
	  bsp->opacity_value = gimp_brush_get_opacity();
	  bsp->spacing_value = gimp_brush_get_spacing();
	  bsp->paint_mode = gimp_brush_get_paint_mode();
	}
      else
	{
	  bsp->opacity_value = init_opacity;
	  bsp->paint_mode = init_mode;
	}
      brush_select_select (bsp, gimp_brush_list_get_brush_index(brush_list, 
								active));

      if(gotinitbrush && init_spacing >= 0)
	{
	  /* Use passed spacing instead of brushes default */
	  bsp->spacing_data->value = init_spacing;
	  gtk_signal_emit_by_name (GTK_OBJECT (bsp->spacing_data), "value_changed");
	}
      bsp->redraw = old_value;
      if (GIMP_IS_BRUSH_GENERATED(active))
	gtk_widget_set_sensitive (bsp->edit_button, 1);
      else
	gtk_widget_set_sensitive (bsp->edit_button, 0); 
    }
  

  return bsp;
}


void
brush_select_select (BrushSelectP bsp,
		     int          index)
{
  int row, col;
  if (index < 0)
    return;
  update_active_brush_field (bsp);
  row = index / bsp->NUM_BRUSH_COLUMNS;
  col = index - row * (bsp->NUM_BRUSH_COLUMNS);

  brush_select_show_selected (bsp, row, col);
}


void
brush_select_free (BrushSelectP bsp)
{
  if (bsp)
    {
      /* Only main one is saved */
      if(bsp == brush_select_dialog)
	session_get_window_info (bsp->shell, &brush_select_session_info);
      if (bsp->brush_popup != NULL)
	gtk_widget_destroy (bsp->brush_popup);

      if(bsp->callback_name)
	g_free(bsp->callback_name);

      /* remove from active list */

      active_dialogs = g_slist_remove(active_dialogs,bsp);

      g_free (bsp);
    }
}

void
brush_change_callbacks(BrushSelectP bsp, gint closing)
{
  gchar * name;
  ProcRecord *prec = NULL;
  GimpBrushP brush;
  int nreturn_vals;
  static int busy = 0;

  /* Any procs registered to callback? */
  Argument *return_vals; 
  
  if(!bsp || !bsp->callback_name || busy != 0)
    return;

  busy = 1;
  name = bsp->callback_name;
  brush = bsp->brush;

  /* If its still registered run it */
  prec = procedural_db_lookup(name);

  if(prec && brush)
    {
      return_vals = procedural_db_run_proc (name,
					    &nreturn_vals,
					    PDB_STRING,brush->name,
					    PDB_FLOAT,bsp->opacity_value,
					    PDB_INT32,bsp->spacing_value,
					    PDB_INT32,(gint)bsp->paint_mode,
					    PDB_INT32,brush->mask->width,
					    PDB_INT32,brush->mask->height,
					    PDB_INT32,brush->mask->width * brush->mask->height,
					    PDB_INT8ARRAY,temp_buf_data (brush->mask),
					    PDB_INT32,closing,
					    PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message (_("failed to run brush callback function"));
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = 0;
}

static void
brush_select_brush_changed(BrushSelectP bsp, GimpBrushP brush)
{
/* TODO: be smarter here and only update the part of the preview
 *       that has changed */
  if (bsp)
  {
    display_brushes(bsp);
    gtk_widget_draw (bsp->preview, NULL);
  }
}

static gint
brush_select_brush_dirty_callback(GimpBrushP brush, BrushSelectP bsp)
{
  brush_select_brush_changed(bsp, brush);
  return TRUE;
}

static void
connect_signals_to_brush(GimpBrushP brush, BrushSelectP bsp)
{
  gtk_signal_connect(GTK_OBJECT (brush), "dirty",
		     GTK_SIGNAL_FUNC(brush_select_brush_dirty_callback),
		     bsp);
}

static void
disconnect_signals_from_brush(GimpBrushP brush, BrushSelectP bsp)
{
  if (!GTK_OBJECT_DESTROYED(brush))
    gtk_signal_disconnect_by_data(GTK_OBJECT(brush), bsp);
}

static void
brush_added_callback(GimpBrushList *list, GimpBrushP brush, 
		     BrushSelectP bsp)
{
  connect_signals_to_brush(brush, bsp);
  preview_calc_scrollbar(bsp);
  brush_select_brush_changed(bsp, brush);
}

static void
brush_removed_callback(GimpBrushList *list, GimpBrushP brush,
		       BrushSelectP bsp)
{
  disconnect_signals_from_brush(brush, bsp);
  preview_calc_scrollbar(bsp);
}


/*
 *  Local functions
 */
static void
brush_popup_open (BrushSelectP bsp,
		  int          x,
		  int          y,
		  GimpBrushP   brush)
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
	       GimpBrushP    brush,
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


static int brush_counter = 0;
static void do_display_brush (GimpBrush *brush, BrushSelectP bsp)
{
  display_brush (bsp, brush, brush_counter % (bsp->NUM_BRUSH_COLUMNS),
		 brush_counter / (bsp->NUM_BRUSH_COLUMNS));
  brush_counter++;
}

static void
display_brushes (BrushSelectP bsp)
{
  /*  If there are no brushes, insensitize widgets  */
  if (brush_list == NULL || gimp_brush_list_length(brush_list) == 0)
  {
    gtk_widget_set_sensitive (bsp->options_box, FALSE);
    return;
  }
  /*  Else, sensitize widgets  */
  else
    gtk_widget_set_sensitive (bsp->options_box, TRUE);

  /*  setup the display area  */
  display_setup (bsp);
  brush_counter = 0;
  gimp_list_foreach(GIMP_LIST(brush_list), (GFunc)do_display_brush, bsp);
}


static void
brush_select_show_selected (BrushSelectP bsp,
			    int          row,
			    int          col)
{
  GdkRectangle area;
  unsigned char * buf;
  int yend;
  int ystart;
  int offset_x, offset_y;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * bsp->cell_width);

  if (bsp->old_col != col || bsp->old_row != row)
    {
      /*  remove the old selection  */
      offset_x = bsp->old_col * bsp->cell_width;
      offset_y = bsp->old_row * bsp->cell_height - bsp->scroll_offset;

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

  bsp->old_row = row;
  bsp->old_col = col;

  g_free (buf);
}


static void
preview_calc_scrollbar (BrushSelectP bsp)
{
  int num_rows;
  int page_size;
  int max;
  int offs;

  offs = bsp->scroll_offset;
  num_rows = (gimp_brush_list_length(brush_list) + (bsp->NUM_BRUSH_COLUMNS) - 1)
    / (bsp->NUM_BRUSH_COLUMNS);
  max = num_rows * bsp->cell_width;
  if (!num_rows) num_rows = 1;
  page_size = bsp->preview->allocation.height;
  page_size = ((page_size < max) ? page_size : max);

  bsp->scroll_offset = offs;
  bsp->sbar_data->value = bsp->scroll_offset;
  bsp->sbar_data->upper = max;
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
   bsp->NUM_BRUSH_COLUMNS = (gint)((widget->allocation.width - 4) / STD_CELL_WIDTH);
   bsp->NUM_BRUSH_ROWS = (gimp_brush_list_length(brush_list) + bsp->NUM_BRUSH_COLUMNS - 1) / bsp->NUM_BRUSH_COLUMNS;
   
   bsp->width = widget->allocation.width - 4;
   bsp->height = widget->allocation.height - 4;
 
   gtk_preview_size (GTK_PREVIEW (bsp->preview), bsp->width, bsp->height);
   
   /*  recalculate scrollbar extents  */
   preview_calc_scrollbar (bsp);
 
   /*  render the brush into the newly created image structure  */
   display_brushes (bsp);
 
   /*  update the display  */   
   if (bsp->redraw)
	gtk_widget_draw (bsp->preview, NULL);
   
   return FALSE;
}
 
static void
update_active_brush_field (BrushSelectP bsp)
{
  GimpBrushP brush;
  char buf[32];

  if(bsp->brush)
    brush = bsp->brush;
  else
    brush = get_active_brush ();

  if (!brush)
    return;

  /*  Set brush name  */
  gtk_label_set_text (GTK_LABEL (bsp->brush_name), brush->name);

  /*  Set brush size  */
  sprintf (buf, "(%d X %d)", brush->mask->width, brush->mask->height);
  gtk_label_set_text (GTK_LABEL (bsp->brush_size), buf);

  /*  Set brush spacing  */
  if(bsp == brush_select_dialog)
    bsp->spacing_data->value = gimp_brush_get_spacing ();
  else
    bsp->spacing_data->value = bsp->spacing_value = brush->spacing;

  gtk_signal_emit_by_name (GTK_OBJECT (bsp->spacing_data), "value_changed");
}


static gint
brush_select_events (GtkWidget    *widget,
		     GdkEvent     *event,
		     BrushSelectP  bsp)
{
  GdkEventButton *bevent;
  GimpBrushP brush;
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
	  index = row * bsp->NUM_BRUSH_COLUMNS + col;

	  /*  Get the brush and display the popup brush preview  */
	  if ((brush = gimp_brush_list_get_brush_by_index (brush_list, index)))
	    {

	      gdk_pointer_grab (bsp->preview->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	      /*  Make this brush the active brush  */
	      /* only if dialog is main one */
	      if(bsp == brush_select_dialog)
		{
		  select_brush (brush);
		}
	      else
		{
		  /* Keeping up appearances */
		  if(bsp)
		    {
		      bsp->brush = brush;
		      brush_select_select (bsp,
			gimp_brush_list_get_brush_index(brush_list, brush));
		    }
		}
		

	      if (GIMP_IS_BRUSH_GENERATED(brush) && bsp == brush_select_dialog)
		gtk_widget_set_sensitive (bsp->edit_button, 1);
	      else
		gtk_widget_set_sensitive (bsp->edit_button, 0);
	      
	      if (brush_edit_generated_dialog)
		brush_edit_generated_set_brush(brush_edit_generated_dialog,
					       get_active_brush());
	      
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

	  /* Call any callbacks registered */
	  brush_change_callbacks(bsp,0);
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
edit_brush_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  if (GIMP_IS_BRUSH_GENERATED(get_active_brush()))
  {
    if (!brush_edit_generated_dialog)
    {
      /*  Create the dialog...  */
      brush_edit_generated_dialog = brush_edit_generated_new();
      brush_edit_generated_set_brush(brush_edit_generated_dialog,
				     get_active_brush());
    }
    else
    {
      /*  Popup the dialog  */
      if (!GTK_WIDGET_VISIBLE (brush_edit_generated_dialog->shell))
	gtk_widget_show (brush_edit_generated_dialog->shell);
      else
	gdk_window_raise(brush_edit_generated_dialog->shell->window);
    }
  }
  else
    g_message(_("We are all fresh out of brush editors today,\n"
	        "please write your own or try back tomorrow\n"));
  return TRUE;
}

static gint
new_brush_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  GimpBrushGenerated *brush;
  brush = gimp_brush_generated_new(10, .5, 0.0, 1.0);
  gimp_brush_list_add(brush_list, GIMP_BRUSH(brush));
  select_brush(GIMP_BRUSH(brush));
  if (brush_edit_generated_dialog)
    brush_edit_generated_set_brush(brush_edit_generated_dialog,
				   get_active_brush());
  edit_brush_callback(w, e, data);
  return TRUE;
}

static gint
brush_select_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  brush_select_close_callback (w, data);

  return TRUE;
}

static void
brush_select_close_callback (GtkWidget *w, /* Unused so can be NULL */
			     gpointer   client_data)
{
  BrushSelectP bsp;

  bsp = (BrushSelectP) client_data;

  if (GTK_WIDGET_VISIBLE (bsp->shell))
    gtk_widget_hide (bsp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if(bsp != brush_select_dialog)
    {
      /* Send data back */
      brush_change_callbacks(bsp,1);
      gtk_widget_destroy(bsp->shell); 
      brush_select_free(bsp); 
    }
}


static void
brush_select_refresh_callback (GtkWidget *w,
			       gpointer   client_data)
{
  BrushSelectP bsp;
  GimpBrushP active;

  bsp = (BrushSelectP) client_data;

  /*  re-init the brush list  */
  brushes_init(FALSE);

  /*  update the active selection  */
  active = get_active_brush ();
  if (active)
    brush_select_select (bsp, gimp_brush_list_get_brush_index(brush_list, 
							      active));

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
  GimpBrushP active;
  int row, col, index;

  bsp = data;

  if (bsp)
    {
      bsp->scroll_offset = adjustment->value;
      display_brushes (bsp);

      if(bsp->brush)
	active = bsp->brush;
      else
	active = get_active_brush ();

      if (active)
	{
	  index = gimp_brush_list_get_brush_index(brush_list, active);
	  if (index < 0)
	    return;
	  row = index / bsp->NUM_BRUSH_COLUMNS;
	  col = index - row * bsp->NUM_BRUSH_COLUMNS;
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
  BrushSelectP bsp = (BrushSelectP)gtk_object_get_user_data(GTK_OBJECT(w));
  
  if(bsp == brush_select_dialog)
    gimp_brush_set_paint_mode ((int) client_data);
  else
    {
      bsp->paint_mode = (int) client_data;
      brush_change_callbacks(bsp,0);
    }
}


static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelectP bsp = (BrushSelectP)data;
  
  if(bsp == brush_select_dialog)
    gimp_brush_set_opacity (adjustment->value / 100.0);
  else
    {
      bsp->opacity_value = (adjustment->value / 100.0);
      brush_change_callbacks(bsp,0); 
    }
}


static void
spacing_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelectP bsp = (BrushSelectP)data;
  
  if(bsp == brush_select_dialog)
    gimp_brush_set_spacing ((int) adjustment->value);
  else
    {
      if(bsp->spacing_value != adjustment->value)
	{
	  bsp->spacing_value = adjustment->value;
	  brush_change_callbacks(bsp,0);
	}
    }
}

/* Close active dialogs that no longer have PDB registered for them */

void
brushes_check_dialogs()
{
  GSList *list;
  BrushSelectP bsp;
  gchar * name;
  ProcRecord *prec = NULL;

  list = active_dialogs;


  while (list)
    {
      bsp = (BrushSelectP) list->data;
      list = list->next;

      name = bsp->callback_name;
      prec = procedural_db_lookup(name);
      
      if(!prec)
	{
	  active_dialogs = g_slist_remove(active_dialogs,bsp);

	  /* Can alter active_dialogs list*/
	  brush_select_close_callback(NULL,bsp);
	}
    }
}

/************
 * PDB interfaces.
 */


static Argument *
brushes_popup_invoker (Argument *args)
{
  gchar * name; 
  gchar * title;
  gchar * initial_brush;
  gdouble initial_opacity = 1.0;
  gint initial_spacing = 20;
  gint initial_mode = 0;
  ProcRecord *prec = NULL;
  BrushSelectP newdialog;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;
  title = (char *) args[1].value.pdb_pointer;
  initial_brush = (char *) args[2].value.pdb_pointer;

  /* If null just use the active brush */
  if(initial_brush && strlen(initial_brush))
    {
      initial_opacity = args[3].value.pdb_float;
      initial_spacing = args[4].value.pdb_int;
      initial_mode = args[5].value.pdb_int;
    }
  
  /* Check the proc exists */
  if(!success || (prec = procedural_db_lookup(name)) == NULL)
    {
      success = 0;
      return procedural_db_return_args (&brushes_popup_proc, success);
    }

  /*create_brush_dialog();*/
  if(initial_brush && strlen(initial_brush))
    newdialog = brush_select_new(title,
				 initial_brush,
				 initial_opacity,
				 initial_spacing,
				 initial_mode);
  else
    newdialog = brush_select_new(title,NULL,0.0,0,0);

  /* Add to list of proc to run when brush changes */
  /* change_callbacks = g_list_append(change_callbacks,g_strdup(name));*/
  newdialog->callback_name = g_strdup(name);

  /* Add to active brush dialogs list */
  active_dialogs = g_slist_append(active_dialogs,newdialog);

  return procedural_db_return_args (&brushes_popup_proc, success);
}

/*  The procedure definition  */
ProcArg brushes_popup_in_args[] =
{
  { PDB_STRING,
    "brush_callback",
    "the callback PDB proc to call when brush selection is made"
  },
  { PDB_STRING,
    "popup title",
    "title to give the popup window",
  },
  { PDB_STRING,
    "initial brush",
    "The name of the brush to set as the first selected",
  },
  { PDB_FLOAT,
    "initial opacity",
    "The initial opacity of the brush",
  },
  { PDB_INT32,
    "initial spacing",
    "The initial spacing of the brush (if < 0 then use brush default spacing)",
  },
  { PDB_INT32,
    "initial paint mode",
    "The initial paint mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY/BURN (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14), DIVIDE/DODGE (15) }",
  },
};

ProcRecord brushes_popup_proc =
{
  "gimp_brushes_popup",
  "Invokes the Gimp brush selection",
  "This procedure popups the brush selection dialog",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(brushes_popup_in_args) / sizeof(brushes_popup_in_args[0]),
  brushes_popup_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brushes_popup_invoker } },
};

static BrushSelectP
brush_get_brushselect(gchar *name)
{
  GSList *list;
  BrushSelectP bsp;

  list = active_dialogs;

  while (list)
    {
      bsp = (BrushSelectP) list->data;
      list = list->next;
      
      if(strcmp(name,bsp->callback_name) == 0)
	{
	  return bsp;
	}
    }

  return NULL;
}

static Argument *
brush_close_popup_invoker (Argument *args)
{
  gchar * name; 
  ProcRecord *prec = NULL;
  BrushSelectP bsp;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;

  /* Check the proc exists */
  if(!success || (prec = procedural_db_lookup(name)) == NULL)
    {
      success = 0;
      return procedural_db_return_args (&brushes_close_popup_proc, success);
    }

  bsp = brush_get_brushselect(name);

  if(bsp)
    {
      active_dialogs = g_slist_remove(active_dialogs,bsp);
      
      if (GTK_WIDGET_VISIBLE (bsp->shell))
	gtk_widget_hide (bsp->shell);

      /* Free memory if poping down dialog which is not the main one */
      if(bsp != brush_select_dialog)
	{
	  gtk_widget_destroy(bsp->shell); 
	  brush_select_free(bsp); 
	}
    }
  else
    {
      success = FALSE;
    }

  return procedural_db_return_args (&brushes_close_popup_proc, success);
}

/*  The procedure definition  */
ProcArg brush_close_popup_in_args[] =
{
  { PDB_STRING,
    "callback_PDB_entry_name",
    "The name of the callback registered for this popup",
  },
};

ProcRecord brushes_close_popup_proc =
{
  "gimp_brushes_close_popup",
  "Popdown the Gimp brush selection",
  "This procedure closes an opened brush selection dialog",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(brush_close_popup_in_args) / sizeof(brush_close_popup_in_args[0]),
  brush_close_popup_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brush_close_popup_invoker } },
};

static Argument *
brush_set_popup_invoker (Argument *args)
{
  gchar * pdbname; 
  gchar * brush_name;
  ProcRecord *prec = NULL;
  BrushSelectP bsp;

  success = (pdbname = (char *) args[0].value.pdb_pointer) != NULL;
  brush_name = (char *) args[1].value.pdb_pointer;

  /* Check the proc exists */
  if(!success || (prec = procedural_db_lookup(pdbname)) == NULL)
    {
      success = 0;
      return procedural_db_return_args (&brushes_set_popup_proc, success);
    }

  bsp = brush_get_brushselect(pdbname);


  if(bsp)
    {
      GimpBrushP active = gimp_brush_list_get_brush(brush_list,brush_name);

      if(active)
	{
	  /* Must alter the wigdets on screen as well */

	  bsp->brush = active;
	  brush_select_select (bsp, gimp_brush_list_get_brush_index(brush_list, active));

	  bsp->opacity_value = args[2].value.pdb_float;
	  bsp->spacing_value = args[3].value.pdb_int;
	  if(args[4].value.pdb_int >= 0 && args[4].value.pdb_int <= VALUE_MODE)
	    bsp->paint_mode = args[4].value.pdb_int;

	  bsp->spacing_data->value = bsp->spacing_value;
	  gtk_signal_emit_by_name (GTK_OBJECT (bsp->spacing_data), "value_changed");

	  bsp->opacity_data->value = bsp->opacity_value * 100.0;
	  gtk_signal_emit_by_name (GTK_OBJECT (bsp->opacity_data), "value_changed");
	  
	  gtk_option_menu_set_history(GTK_OPTION_MENU(bsp->option_menu),bsp->paint_mode);

	  /* Can alter active_dialogs list*/
	  success = TRUE;
	}
    }
  else
    {
      success = FALSE;
    }

  return procedural_db_return_args (&brushes_set_popup_proc, success);
}

/*  The procedure definition  */
ProcArg brush_set_popup_in_args[] =
{
  { PDB_STRING,
    "callback_PDB_entry_name",
    "The name of the callback registered for this popup",
  },
  { PDB_STRING,
    "brushname",
    "The name of the brush to set as selected",
  },
  { PDB_FLOAT,
    "opacity",
    "The initial opacity of the brush",
  },
  { PDB_INT32,
    "spacing",
    "The initial spacing of the brush (if < 0 then use brush default spacing)",
  },
  { PDB_INT32,
    "initial paint mode",
    "The initial paint mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY/BURN (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14), DIVIDE/DODGE (15) }",
  },
};

ProcRecord brushes_set_popup_proc =
{
  "gimp_brushes_set_popup",
  "Sets the current brush selection in a popup",
  "Sets the current brush selection in a popup",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(brush_set_popup_in_args) / sizeof(brush_set_popup_in_args[0]),
  brush_set_popup_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brush_set_popup_invoker } },
};
