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
#include <string.h>
#include "appenv.h"
#include "patterns.h"
#include "pattern_select.h"
#include "colormaps.h"
#include "errors.h"
#include "gimpui.h"
#include "session.h"

#include "libgimp/gimpintl.h"

#define MIN_CELL_SIZE       32
#define MAX_CELL_SIZE       45

#define STD_PATTERN_COLUMNS 6
#define STD_PATTERN_ROWS    5 

/* how long to wait after mouse-down before showing pattern popup */
#define POPUP_DELAY_MS      150

#define MAX_WIN_WIDTH(psp)     (MIN_CELL_SIZE * (psp)->NUM_PATTERN_COLUMNS)
#define MAX_WIN_HEIGHT(psp)    (MIN_CELL_SIZE * (psp)->NUM_PATTERN_ROWS)
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
static void pattern_select_refresh_callback  (GtkWidget *, gpointer);
static gint pattern_select_events            (GtkWidget *, GdkEvent *, PatternSelectP);
static gint pattern_select_resize            (GtkWidget *, GdkEvent *, PatternSelectP);
static void pattern_select_scroll_update     (GtkAdjustment *, gpointer);


/*  local variables  */

/*  List of active dialogs  */
GSList *pattern_active_dialogs = NULL;


/*  If title == NULL then it is the main pattern dialog  */
PatternSelectP
pattern_select_new (gchar *title,
		    gchar *initial_pattern)
{
  PatternSelectP psp;
  GPatternP active = NULL;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *sbar;
  GtkWidget *label_box;

  psp = g_new (_PatternSelect, 1);
  psp->preview = NULL;
  psp->old_col = psp->old_row = 0;
  psp->callback_name = NULL;
  psp->pattern_popup = NULL;
  psp->popup_timeout_tag = 0;
  psp->NUM_PATTERN_COLUMNS = STD_PATTERN_COLUMNS;
  psp->NUM_PATTERN_ROWS    = STD_PATTERN_COLUMNS;

  /*  The shell and main vbox  */
  if (title)
    {
      psp->shell =
	gimp_dialog_new (title, "pattern_selection",
			 gimp_standard_help_func,
			 "dialogs/pattern_selection.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("Close"), pattern_select_close_callback,
			 psp, NULL, TRUE, TRUE,

			 NULL);

      if (initial_pattern && strlen (initial_pattern))
	{
	  active = pattern_list_get_pattern (pattern_list, initial_pattern);
	}
    }
  else
    {
      psp->shell =
	gimp_dialog_new (_("Pattern Selection"), "pattern_selection",
			 gimp_standard_help_func,
			 "dialogs/pattern_selection.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("Refresh"), pattern_select_refresh_callback,
			 psp, NULL, FALSE, FALSE,
			 _("Close"), pattern_select_close_callback,
			 psp, NULL, TRUE, TRUE,

			 NULL);

      session_set_window_geometry (psp->shell, &pattern_select_session_info,
				   TRUE);
    }

  /*  update the active selection  */
  if (!active)
    active = get_active_pattern ();

  psp->pattern = active;

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), vbox);

  /*  Options box  */
  psp->options_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), psp->options_box, FALSE, FALSE, 0);

  /*  Create the active pattern label  */
  label_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (psp->options_box), label_box, FALSE, FALSE, 2);

  psp->pattern_name = gtk_label_new (_("Active"));
  gtk_box_pack_start (GTK_BOX (label_box), psp->pattern_name, FALSE, FALSE, 4);
  psp->pattern_size = gtk_label_new ("(0 X 0)");
  gtk_box_pack_start (GTK_BOX (label_box), psp->pattern_size, FALSE, FALSE, 2);

  gtk_widget_show (psp->pattern_name);
  gtk_widget_show (psp->pattern_size);
  gtk_widget_show (label_box);

  /*  The horizontal box containing preview & scrollbar  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  psp->sbar_data = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, MAX_WIN_HEIGHT(psp), 1, 1, MAX_WIN_HEIGHT(psp)));
  gtk_signal_connect (GTK_OBJECT (psp->sbar_data), "value_changed",
		      (GtkSignalFunc) pattern_select_scroll_update,
		      psp);
  sbar = gtk_vscrollbar_new (psp->sbar_data);
  gtk_box_pack_start (GTK_BOX (hbox), sbar, FALSE, FALSE, 0);

  /*  Create the pattern preview window and the underlying image  */

  /*  Get the initial pattern extents  */
  psp->cell_width = MIN_CELL_SIZE;
  psp->cell_height = MIN_CELL_SIZE;

  psp->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (psp->preview),
		    MAX_WIN_WIDTH (psp), MAX_WIN_HEIGHT (psp));
  gtk_preview_set_expand (GTK_PREVIEW (psp->preview), TRUE);
  gtk_widget_set_events (psp->preview, PATTERN_EVENT_MASK);

  gtk_signal_connect (GTK_OBJECT (psp->preview), "event",
		      (GtkSignalFunc) pattern_select_events,
		      psp);
  gtk_signal_connect (GTK_OBJECT (psp->preview), "size_allocate",
		      (GtkSignalFunc) pattern_select_resize,
		      psp);

  gtk_container_add (GTK_CONTAINER (frame), psp->preview);
  gtk_widget_show (psp->preview);

  gtk_widget_show (sbar);
  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (psp->options_box);
  gtk_widget_show (vbox);
  gtk_widget_show (psp->shell);

  if(no_data)   /* if patterns are already loaded, dont do it now... */
    patterns_init(FALSE);
  preview_calc_scrollbar (psp);
  display_patterns (psp);

  if (active)
    pattern_select_select (psp, active->index);

  return psp;
}

void
pattern_change_callbacks(PatternSelectP psp,
			 gint           closing)
{
  gchar * name;
  ProcRecord *prec = NULL;
  GPatternP pattern;
  int nreturn_vals;
  static int busy = 0;

  /* Any procs registered to callback? */
  Argument *return_vals; 
  
  if(!psp || !psp->callback_name || busy != 0)
    return;

  busy = 1;
  name = psp->callback_name;
  pattern = psp->pattern;

  /* If its still registered run it */
  prec = procedural_db_lookup(name);

  if(prec && pattern)
    {
      return_vals = procedural_db_run_proc (name,
					    &nreturn_vals,
					    PDB_STRING,pattern->name,
					    PDB_INT32,pattern->mask->width,
					    PDB_INT32,pattern->mask->height,
					    PDB_INT32,pattern->mask->bytes,
					    PDB_INT32,pattern->mask->bytes*pattern->mask->height*pattern->mask->width,
					    PDB_INT8ARRAY,temp_buf_data (pattern->mask),
					    PDB_INT32,closing,
					    PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_warning ("failed to run pattern callback function");
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = 0;
}

void
pattern_select_select (PatternSelectP psp,
		       int            index)
{
  int row, col;

  update_active_pattern_field (psp);
  row = index / psp->NUM_PATTERN_COLUMNS;
  col = index - row * psp->NUM_PATTERN_COLUMNS;

  pattern_select_show_selected (psp, row, col);
}

void
pattern_select_free (PatternSelectP psp)
{
  if (psp)
    {
      /* Only main one is saved */
      if (psp == pattern_select_dialog)
	session_get_window_info (psp->shell, &pattern_select_session_info);
      if (psp->pattern_popup != NULL)
	gtk_widget_destroy (psp->pattern_popup);
      if (psp->popup_timeout_tag != 0)
	gtk_timeout_remove (psp->popup_timeout_tag);

      if (psp->callback_name)
	g_free (psp->callback_name);

      /* remove from active list */
      pattern_active_dialogs = g_slist_remove (pattern_active_dialogs, psp);

      g_free (psp);
    }
}

/*
 *  Local functions
 */

typedef struct {
  PatternSelectP psp;
  int            x;
  int            y;
  GPatternP      pattern;
} popup_timeout_args_t;


static gboolean
pattern_popup_timeout (gpointer data)
{
  popup_timeout_args_t *args = data;
  PatternSelectP psp = args->psp;
  GPatternP pattern = args->pattern;
  gint x, y;
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;

  /* timeout has gone off so our tag is now invalid  */
  psp->popup_timeout_tag = 0;

  /* make sure the popup exists and is not visible */
  if (psp->pattern_popup == NULL)
    {
      GtkWidget *frame;
      psp->pattern_popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (psp->pattern_popup),
			     FALSE, FALSE, TRUE);
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
  x = x_org + args->x - pattern->mask->width * 0.5;
  y = y_org + args->y - pattern->mask->height * 0.5;
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + pattern->mask->width > scr_w) ? scr_w - pattern->mask->width : x;
  y = (y + pattern->mask->height > scr_h) ? scr_h - pattern->mask->height : y;
  gtk_preview_size (GTK_PREVIEW (psp->pattern_preview),
		    pattern->mask->width, pattern->mask->height);
  gtk_widget_popup (psp->pattern_popup, x, y);

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
      gtk_preview_draw_row (GTK_PREVIEW (psp->pattern_preview), (guchar *)buf, 0, y, pattern->mask->width);
      src += pattern->mask->width * pattern->mask->bytes;
    }
  g_free (buf);

  /*  Draw the pattern preview  */
  gtk_widget_draw (psp->pattern_preview, NULL);

  return FALSE;  /* don't repeat */
}


static void
pattern_popup_open (PatternSelectP psp,
		    int            x,
		    int            y,
		    GPatternP      pattern)
{
  static popup_timeout_args_t popup_timeout_args;

  /* if we've already got a timeout scheduled, then we complain */
  g_return_if_fail (psp->popup_timeout_tag == 0);

  popup_timeout_args.psp = psp;
  popup_timeout_args.x = x;
  popup_timeout_args.y = y;
  popup_timeout_args.pattern = pattern;
  psp->popup_timeout_tag = gtk_timeout_add (POPUP_DELAY_MS,
					    pattern_popup_timeout,
					    &popup_timeout_args);
}

static void
pattern_popup_close (PatternSelectP psp)
{
  if (psp->popup_timeout_tag != 0)
    gtk_timeout_remove (psp->popup_timeout_tag);
  psp->popup_timeout_tag = 0;

  if (psp->pattern_popup != NULL)
    gtk_widget_hide (psp->pattern_popup);
}

static void
display_pattern (PatternSelectP psp,
		 GPatternP      pattern,
		 int            col,
		 int            row)
{
  TempBuf * pattern_buf;
  unsigned char * src, *s;
  unsigned char * buf, *b;
  int cell_width, cell_height;
  int width, height;
  int rowstride;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i, j;

  buf = (unsigned char *) g_malloc (sizeof (char) * psp->cell_width * 3);

  pattern_buf = pattern->mask;

  /*  calculate the offset into the image  */
  cell_width = psp->cell_width - 2*MARGIN_WIDTH;
  cell_height = psp->cell_height - 2*MARGIN_HEIGHT;
  width = (pattern_buf->width > cell_width) ? cell_width :
    pattern_buf->width;
  height = (pattern_buf->height > cell_height) ? cell_height :
    pattern_buf->height;

  offset_x = col * psp->cell_width + ((cell_width - width) >> 1) + MARGIN_WIDTH;
  offset_y = row * psp->cell_height + ((cell_height - height) >> 1)
    - psp->scroll_offset + MARGIN_HEIGHT;

  ystart = BOUNDS (offset_y, 0, psp->preview->allocation.height);
  yend = BOUNDS (offset_y + height, 0, psp->preview->allocation.height);

  /*  Get the pointer into the pattern mask data  */
  rowstride = pattern_buf->width * pattern_buf->bytes;
  src = temp_buf_data (pattern_buf) + (ystart - offset_y) * rowstride;

  for (i = ystart; i < yend; i++)
    {
      s = src;
      b = buf;

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

      gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, offset_x, i, width);

      src += rowstride;
    }

  g_free (buf);
}

static void
display_setup (PatternSelectP psp)
{
  unsigned char * buf;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * psp->preview->allocation.width * 3);

  /*  Set the buffer to white  */
  memset (buf, 255, psp->preview->allocation.width * 3);

  /*  Set the image buffer to white  */
  for (i = 0; i < psp->preview->allocation.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, 0, i,
			  psp->preview->allocation.width);

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
      if (++col == psp->NUM_PATTERN_COLUMNS)
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
  GdkRectangle area;
  unsigned char * buf;
  int yend;
  int ystart;
  int offset_x, offset_y;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * psp->cell_width * 3);

  if (psp->old_col != col || psp->old_row != row)
    {
      /*  remove the old selection  */
      offset_x = psp->old_col * psp->cell_width;
      offset_y = psp->old_row * psp->cell_height - psp->scroll_offset;

      ystart = BOUNDS (offset_y , 0, psp->preview->allocation.height);
      yend = BOUNDS (offset_y + psp->cell_height, 0, psp->preview->allocation.height);

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

  ystart = BOUNDS (offset_y , 0, psp->preview->allocation.height);
  yend = BOUNDS (offset_y + psp->cell_height, 0, psp->preview->allocation.height);

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

  psp->old_row = row;
  psp->old_col = col;

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
  num_rows = (num_patterns + psp->NUM_PATTERN_COLUMNS - 1) / psp->NUM_PATTERN_COLUMNS;
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
  gchar buf[32];

  if(pattern_select_dialog == psp)
    pattern = get_active_pattern ();
  else
    pattern = psp->pattern;

  if (!pattern)
    return;

  /*  Set pattern name  */
  gtk_label_set_text (GTK_LABEL (psp->pattern_name), pattern->name);

  /*  Set pattern size  */
  g_snprintf (buf, sizeof (buf), "(%d X %d)",
	      pattern->mask->width, pattern->mask->height);
  gtk_label_set_text (GTK_LABEL (psp->pattern_size), buf);
}

static gint
pattern_select_resize (GtkWidget      *widget,
		       GdkEvent       *event,
		       PatternSelectP  psp)
{
  /*  calculate the best-fit approximation...  */  
  gint wid;
  gint now;
  gint cell_size;

  wid = widget->allocation.width;

  for(now = cell_size = MIN_CELL_SIZE;
      now < MAX_CELL_SIZE; ++now)
    {
      if ((wid % now) < (wid % cell_size)) cell_size = now;
      if ((wid % cell_size) == 0)
        break;
    }

  psp->NUM_PATTERN_COLUMNS =
    (gint) (wid / cell_size);
  psp->NUM_PATTERN_ROWS =
    (gint) ((num_patterns + psp->NUM_PATTERN_COLUMNS - 1) /
	    psp->NUM_PATTERN_COLUMNS);

  psp->cell_width = cell_size;
  psp->cell_height = cell_size;

  /*  recalculate scrollbar extents  */
  preview_calc_scrollbar (psp);

  /*  render the patterns into the newly created image structure  */
  display_patterns (psp);

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
	  index = row * psp->NUM_PATTERN_COLUMNS + col;

	  /*  Get the pattern and display the popup pattern preview  */
	  if ((pattern = get_pattern_by_index (index)))
	    {
	      gdk_pointer_grab (psp->preview->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	      if(pattern_select_dialog == psp)
		{
		  /*  Make this pattern the active pattern  */
		  select_pattern (pattern);
		}
	      else
		{
		  psp->pattern = pattern;
		  pattern_select_select (psp, pattern->index);
		}
	      /*  Show the pattern popup window if the pattern is too large  */
	      if (pattern->mask->width > psp->cell_width ||
		  pattern->mask->height > psp->cell_height)
		pattern_popup_open (psp, bevent->x, bevent->y, pattern);
	    }
	}

      /*  wheelmouse support  */
      else if (bevent->button == 4)
	{
	  GtkAdjustment *adj = psp->sbar_data;
	  gfloat new_value = adj->value - adj->page_increment / 2;
	  new_value =
	    CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	  gtk_adjustment_set_value (adj, new_value);
	}
      else if (bevent->button == 5)
	{
	  GtkAdjustment *adj = psp->sbar_data;
	  gfloat new_value = adj->value + adj->page_increment / 2;
	  new_value =
	    CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	  gtk_adjustment_set_value (adj, new_value);
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

	  /* Call any callbacks registered */
	  pattern_change_callbacks(psp,0);
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
pattern_select_close_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  PatternSelectP psp;

  psp = (PatternSelectP) client_data;

  if (GTK_WIDGET_VISIBLE (psp->shell))
    gtk_widget_hide (psp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if(psp != pattern_select_dialog)
    {
      /* Send data back */
      pattern_change_callbacks(psp,1);
      gtk_widget_destroy(psp->shell); 
      pattern_select_free(psp); 
    }
}

static void
pattern_select_refresh_callback (GtkWidget *widget,
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

      if(pattern_select_dialog == psp)
	active = get_active_pattern ();
      else
	active = psp->pattern;

      if (active)
	{
	  row = active->index / psp->NUM_PATTERN_COLUMNS;
	  col = active->index - row * psp->NUM_PATTERN_COLUMNS;
	  pattern_select_show_selected (psp, row, col);
	}

      draw_preview (psp);
    }
}


/* Close active dialogs that no longer have PDB registered for them */

void
patterns_check_dialogs (void)
{
  GSList *list;
  PatternSelectP psp;
  gchar * name;
  ProcRecord *prec = NULL;

  list = pattern_active_dialogs;

  while (list)
    {
      psp = (PatternSelectP) list->data;
      list = list->next;

      name = psp->callback_name;
      prec = procedural_db_lookup(name);

      if (!prec)
	{
	  pattern_active_dialogs = g_slist_remove (pattern_active_dialogs,psp);

	  /* Can alter pattern_active_dialogs list*/
	  pattern_select_close_callback (NULL, psp); 
	}
    }
}
