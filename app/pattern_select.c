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
#include "dialog_handler.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpui.h"
#include "patterns.h"
#include "pattern_select.h"
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
static void     pattern_change_callbacks        (PatternSelect *psp,
						 gboolean       closing);

static void     pattern_select_drop_pattern     (GtkWidget     *widget,
						 GPattern      *pattern,
						 gpointer       data);
static void     pattern_select_pattern_changed  (GimpContext   *context,
						 GPattern      *pattern,
						 PatternSelect *psp);
static void     pattern_select_select           (PatternSelect *psp,
						 gint           index);

static gboolean pattern_popup_timeout           (gpointer       data);
static void     pattern_popup_open              (PatternSelect *psp,
						 gint, gint, GPattern *);
static void     pattern_popup_close             (PatternSelect *);

static void     display_setup                   (PatternSelect *);
static void     display_pattern                 (PatternSelect *, GPattern *,
						 gint, gint);
static void     display_patterns                (PatternSelect *);

static void     pattern_select_show_selected    (PatternSelect *, gint, gint);
static void     update_active_pattern_field     (PatternSelect *);
static void     preview_calc_scrollbar          (PatternSelect *);

static gint     pattern_select_resize           (GtkWidget *, GdkEvent *,
						 PatternSelect *);
static gint     pattern_select_events           (GtkWidget *, GdkEvent *,
						 PatternSelect *);

static void     pattern_select_scroll_update    (GtkAdjustment *, gpointer);

static void     pattern_select_close_callback   (GtkWidget *, gpointer);
static void     pattern_select_refresh_callback (GtkWidget *, gpointer);

/*  dnd stuff  */
static GtkTargetEntry preview_target_table[] =
{
  GIMP_TARGET_PATTERN
};
static guint preview_n_targets = (sizeof (preview_target_table) /
                                  sizeof (preview_target_table[0]));

/*  The main pattern selection dialog  */
PatternSelect   *pattern_select_dialog = NULL;

/*  local variables  */

/*  List of active dialogs  */
GSList *pattern_active_dialogs = NULL;

void
pattern_dialog_create (void)
{
  if (! pattern_select_dialog)
    {
      pattern_select_dialog = pattern_select_new (NULL, NULL);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (pattern_select_dialog->shell))
	gtk_widget_show (pattern_select_dialog->shell);
      else
	gdk_window_raise (pattern_select_dialog->shell->window);
    }
}

void
pattern_dialog_free ()
{
  if (pattern_select_dialog)
    {
      session_get_window_info (pattern_select_dialog->shell,
			       &pattern_select_session_info);

      pattern_select_free (pattern_select_dialog);
      pattern_select_dialog = NULL;
    }
}

/*  If title == NULL then it is the main pattern dialog  */
PatternSelect *
pattern_select_new (gchar *title,
		    gchar *initial_pattern)
{
  PatternSelect *psp;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *sbar;
  GtkWidget *label_box;

  GPattern *active = NULL;

  static gboolean first_call = TRUE;

  psp = g_new (PatternSelect, 1);
  psp->preview             = NULL;
  psp->callback_name       = NULL;
  psp->pattern_popup       = NULL;
  psp->popup_timeout_tag   = 0;
  psp->old_col             = 0;
  psp->old_row             = 0;
  psp->NUM_PATTERN_COLUMNS = STD_PATTERN_COLUMNS;
  psp->NUM_PATTERN_ROWS    = STD_PATTERN_COLUMNS;

  /*  The shell  */
  psp->shell = 	gimp_dialog_new (title ? title : _("Pattern Selection"),
				 "pattern_selection",
				 gimp_standard_help_func,
				 "dialogs/pattern_selection.html",
				 GTK_WIN_POS_NONE,
				 FALSE, TRUE, FALSE,

				 _("Refresh"), pattern_select_refresh_callback,
				 psp, NULL, FALSE, FALSE,
				 _("Close"), pattern_select_close_callback,
				 psp, NULL, TRUE, TRUE,

				 NULL);

  if (title)
    {
      psp->context = gimp_context_new (title, NULL);
    }
  else
    {
      psp->context = gimp_context_get_user ();

      session_set_window_geometry (psp->shell, &pattern_select_session_info,
				   TRUE);
      dialog_register (psp->shell);
    }

  if (no_data && first_call)
    patterns_init (FALSE);

  first_call = FALSE;

  if (title && initial_pattern && strlen (initial_pattern))
    {
      active = pattern_list_get_pattern (pattern_list, initial_pattern);
    }
  else
    {
      active = gimp_context_get_pattern (gimp_context_get_user ());
    }

  if (!active)
    {
      active = gimp_context_get_pattern (gimp_context_get_standard ());
    }

  if (title)
    {
      gimp_context_set_pattern (psp->context, active);
    }

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
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  psp->sbar_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, MAX_WIN_HEIGHT(psp),
					1, 1, MAX_WIN_HEIGHT(psp)));
  sbar = gtk_vscrollbar_new (psp->sbar_data);
  gtk_signal_connect (GTK_OBJECT (psp->sbar_data), "value_changed",
		      GTK_SIGNAL_FUNC (pattern_select_scroll_update),
		      psp);
  gtk_box_pack_start (GTK_BOX (hbox), sbar, FALSE, FALSE, 0);

  /*  Create the pattern preview window and the underlying image  */

  /*  Get the initial pattern extents  */
  psp->cell_width  = MIN_CELL_SIZE;
  psp->cell_height = MIN_CELL_SIZE;

  psp->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (psp->preview),
		    MAX_WIN_WIDTH (psp), MAX_WIN_HEIGHT (psp));
  gtk_preview_set_expand (GTK_PREVIEW (psp->preview), TRUE);
  gtk_widget_set_events (psp->preview, PATTERN_EVENT_MASK);

  gtk_signal_connect (GTK_OBJECT (psp->preview), "event",
		      GTK_SIGNAL_FUNC (pattern_select_events),
		      psp);
  gtk_signal_connect (GTK_OBJECT (psp->preview), "size_allocate",
		      GTK_SIGNAL_FUNC (pattern_select_resize),
		      psp);

  /*  dnd stuff  */
  gtk_drag_dest_set (psp->preview,
                     GTK_DEST_DEFAULT_ALL,
                     preview_target_table, preview_n_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_pattern_dest_set (psp->preview, pattern_select_drop_pattern, psp);

  gtk_container_add (GTK_CONTAINER (frame), psp->preview);
  gtk_widget_show (psp->preview);

  gtk_widget_show (sbar);
  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (psp->options_box);
  gtk_widget_show (vbox);
  gtk_widget_show (psp->shell);

  preview_calc_scrollbar (psp);

  gtk_signal_connect (GTK_OBJECT (psp->context), "pattern_changed",
		      GTK_SIGNAL_FUNC (pattern_select_pattern_changed),
		      psp);

  if (active)
    pattern_select_select (psp, active->index);

  /*  Add to active pattern dialogs list  */
  pattern_active_dialogs = g_slist_append (pattern_active_dialogs, psp);

  return psp;
}

void
pattern_select_free (PatternSelect *psp)
{
  if (!psp)
    return;

  /* remove from active list */
  pattern_active_dialogs = g_slist_remove (pattern_active_dialogs, psp);

  gtk_signal_disconnect_by_data (GTK_OBJECT (psp->context), psp);

  if (psp->pattern_popup != NULL)
    gtk_widget_destroy (psp->pattern_popup);

  if (psp->popup_timeout_tag != 0)
    gtk_timeout_remove (psp->popup_timeout_tag);

  if (psp->callback_name)
    {
      g_free (psp->callback_name);
      gtk_object_unref (GTK_OBJECT (psp->context));
    }

  g_free (psp);
}

/*  Call this dialog's PDB callback  */

static void
pattern_change_callbacks (PatternSelect *psp,
			  gboolean      closing)
{
  gchar *name;
  ProcRecord *prec = NULL;
  GPatternP pattern;
  gint nreturn_vals;
  static gboolean busy = FALSE;

  /* Any procs registered to callback? */
  Argument *return_vals; 

  if (!psp || !psp->callback_name || busy)
    return;

  busy = TRUE;
  name = psp->callback_name;
  pattern = gimp_context_get_pattern (psp->context);

  /* If its still registered run it */
  prec = procedural_db_lookup (name);

  if (prec && pattern)
    {
      return_vals =
	procedural_db_run_proc (name,
				&nreturn_vals,
				PDB_STRING,    pattern->name,
				PDB_INT32,     pattern->mask->width,
				PDB_INT32,     pattern->mask->height,
				PDB_INT32,     pattern->mask->bytes,
				PDB_INT32,     pattern->mask->bytes*pattern->mask->height*pattern->mask->width,
				PDB_INT8ARRAY, temp_buf_data (pattern->mask),
				PDB_INT32,     (gint) closing,
				PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_warning ("failed to run pattern callback function");
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = FALSE;
}

/*  Close active dialogs that no longer have PDB registered for them  */

void
patterns_check_dialogs (void)
{
  PatternSelect *psp;
  GSList *list;
  gchar *name;
  ProcRecord *prec = NULL;

  list = pattern_active_dialogs;

  while (list)
    {
      psp = (PatternSelect *) list->data;
      list = g_slist_next (list);

      name = psp->callback_name;

      if (name)
	{
	  prec = procedural_db_lookup (name);

	  if (!prec)
	    {
	      /*  Can alter pattern_active_dialogs list  */
	      pattern_select_close_callback (NULL, psp); 
	    }
	}
    }
}

/*
 *  Local functions
 */

static void
pattern_select_drop_pattern (GtkWidget *widget,
			     GPattern  *pattern,
			     gpointer   data)
{
  PatternSelect *psp;

  psp = (PatternSelect *) data;

  gimp_context_set_pattern (psp->context, pattern);
}

static void
pattern_select_pattern_changed (GimpContext   *context,
				GPattern      *pattern,
				PatternSelect *psp)
{
  if (pattern)
    pattern_select_select (psp, pattern->index);
}

static void
pattern_select_select (PatternSelect *psp,
		       gint           index)
{
  gint row, col;
  gint scroll_offset = 0;

  update_active_pattern_field (psp);

  row = index / psp->NUM_PATTERN_COLUMNS;
  col = index - row * psp->NUM_PATTERN_COLUMNS;

  /*  check if the new active pattern is already in the preview  */
  if (((row + 1) * psp->cell_height) >
      (psp->preview->allocation.height + psp->scroll_offset))
    {
      scroll_offset = (((row + 1) * psp->cell_height) -
                       (psp->scroll_offset + psp->preview->allocation.height));
    }
  else if ((row * psp->cell_height) < psp->scroll_offset)
    {
      scroll_offset = (row * psp->cell_height) - psp->scroll_offset;
    }
  else
    {
      pattern_select_show_selected (psp, row, col);
    }

  gtk_adjustment_set_value (psp->sbar_data, psp->scroll_offset + scroll_offset);
}

typedef struct
{
  PatternSelect *psp;
  int            x;
  int            y;
  GPattern      *pattern;
} popup_timeout_args_t;


static gboolean
pattern_popup_timeout (gpointer data)
{
  popup_timeout_args_t *args = data;
  PatternSelect *psp = args->psp;
  GPattern *pattern = args->pattern;
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
pattern_popup_open (PatternSelect *psp,
		    gint           x,
		    gint           y,
		    GPattern      *pattern)
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
pattern_popup_close (PatternSelect *psp)
{
  if (psp->popup_timeout_tag != 0)
    gtk_timeout_remove (psp->popup_timeout_tag);
  psp->popup_timeout_tag = 0;

  if (psp->pattern_popup != NULL)
    gtk_widget_hide (psp->pattern_popup);
}

static void
display_setup (PatternSelect *psp)
{
  guchar * buf;
  gint i;

  buf = g_new (guchar, 3 * psp->preview->allocation.width);

  /*  Set the buffer to white  */
  memset (buf, 255, psp->preview->allocation.width * 3);

  /*  Set the image buffer to white  */
  for (i = 0; i < psp->preview->allocation.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf, 0, i,
			  psp->preview->allocation.width);

  g_free (buf);
}

static void
display_pattern (PatternSelect *psp,
		 GPattern      *pattern,
		 gint           col,
		 gint           row)
{
  TempBuf * pattern_buf;
  guchar * src, *s;
  guchar * buf, *b;
  gint cell_width, cell_height;
  gint width, height;
  gint rowstride;
  gint offset_x, offset_y;
  gint yend;
  gint ystart;
  gint i, j;

  buf = g_new (guchar, psp->cell_width * 3);

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
display_patterns (PatternSelect *psp)
{
  GSList *list = pattern_list;    /*  the global pattern list  */
  gint row, col;
  GPattern *pattern;

  if (list == NULL)
    {
      gtk_widget_set_sensitive (psp->options_box, FALSE);
      return;
    }
  else
    {
      gtk_widget_set_sensitive (psp->options_box, TRUE);
    }

  /*  setup the display area  */
  display_setup (psp);

  row = col = 0;
  for (; list; list = g_slist_next (list))
    {
      pattern = (GPattern *) list->data;

      /*  Display the pattern  */
      display_pattern (psp, pattern, col, row);

      /*  increment the counts  */
      if (++col == psp->NUM_PATTERN_COLUMNS)
	{
	  row ++;
	  col = 0;
	}
    }
}

static void
pattern_select_show_selected (PatternSelect *psp,
			      gint           row,
			      gint           col)
{
  GdkRectangle area;
  guchar * buf;
  gint yend;
  gint ystart;
  gint offset_x, offset_y;
  gint i;

  buf = g_new (guchar, 3 * psp->cell_width);

  if (psp->old_col != col || psp->old_row != row)
    {
      /*  remove the old selection  */
      offset_x = psp->old_col * psp->cell_width;
      offset_y = psp->old_row * psp->cell_height - psp->scroll_offset;

      ystart = BOUNDS (offset_y , 0, psp->preview->allocation.height);
      yend = BOUNDS (offset_y + psp->cell_height, 0, psp->preview->allocation.height);

      /*  set the buf to white  */
      memset (buf, 255, 3 * psp->cell_width);

      for (i = ystart; i < yend; i++)
	{
	  if (i == offset_y || i == (offset_y + psp->cell_height - 1))
	    gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf,
				  offset_x, i, psp->cell_width);
	  else
	    {
	      gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf,
				    offset_x, i, 1);
	      gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf,
				    offset_x + psp->cell_width - 1, i, 1);
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
	gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf,
			      offset_x, i, psp->cell_width);
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf,
				offset_x, i, 1);
	  gtk_preview_draw_row (GTK_PREVIEW (psp->preview), buf,
				offset_x + psp->cell_width - 1, i, 1);
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
update_active_pattern_field (PatternSelect *psp)
{
  GPattern *pattern;
  gchar buf[32];

  pattern = gimp_context_get_pattern (psp->context);

  if (!pattern)
    return;

  /*  Set pattern name  */
  gtk_label_set_text (GTK_LABEL (psp->pattern_name), pattern->name);

  /*  Set pattern size  */
  g_snprintf (buf, sizeof (buf), "(%d X %d)",
	      pattern->mask->width, pattern->mask->height);
  gtk_label_set_text (GTK_LABEL (psp->pattern_size), buf);
}

static void
preview_calc_scrollbar (PatternSelect *psp)
{
  gint num_rows;
  gint page_size;
  gint max;

  psp->scroll_offset = 0;
  num_rows = ((num_patterns + psp->NUM_PATTERN_COLUMNS - 1) /
	      psp->NUM_PATTERN_COLUMNS);
  max = num_rows * psp->cell_width;
  if (!num_rows)
    num_rows = 1;
  page_size = psp->preview->allocation.height;

  psp->sbar_data->value          = psp->scroll_offset;
  psp->sbar_data->upper          = max;
  psp->sbar_data->page_size      = (page_size < max) ? page_size : max;
  psp->sbar_data->page_increment = (page_size >> 1);
  psp->sbar_data->step_increment = psp->cell_width;

  gtk_signal_emit_by_name (GTK_OBJECT (psp->sbar_data), "changed");
  gtk_signal_emit_by_name (GTK_OBJECT (psp->sbar_data), "value_changed");
}

static gint
pattern_select_resize (GtkWidget     *widget,
		       GdkEvent      *event,
		       PatternSelect *psp)
{
  /*  calculate the best-fit approximation...  */  
  gint wid;
  gint now;
  gint cell_size;

  wid = widget->allocation.width;

  for (now = cell_size = MIN_CELL_SIZE;
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

  psp->cell_width  = cell_size;
  psp->cell_height = cell_size;

  /*  recalculate scrollbar extents  */
  preview_calc_scrollbar (psp);

  return FALSE;
}

static gint
pattern_select_events (GtkWidget     *widget,
		       GdkEvent      *event,
		       PatternSelect *psp)
{
  GdkEventButton *bevent;
  GPattern *pattern;
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
	  if ((pattern = pattern_list_get_pattern_by_index (pattern_list,
							    index)))
	    {
	      gdk_pointer_grab (psp->preview->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	      /*  Make this pattern the active pattern  */
	      gimp_context_set_pattern (psp->context, pattern);

	      /*  Show the pattern popup window if the pattern is too large  */
	      if (pattern->mask->width  > psp->cell_width ||
		  pattern->mask->height > psp->cell_height)
		{
		  pattern_popup_open (psp, bevent->x, bevent->y, pattern);
		}
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
	  pattern_change_callbacks (psp, FALSE);
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
pattern_select_scroll_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  PatternSelect *psp;
  GPattern *active;
  gint row, col, index;

  psp = (PatternSelect *) data;

  if (psp)
    {
      psp->scroll_offset = adjustment->value;
 
      display_patterns (psp);

      active = gimp_context_get_pattern (psp->context);

      if (active)
	{
	  index = active->index;
	  row = index / psp->NUM_PATTERN_COLUMNS;
	  col = index - row * psp->NUM_PATTERN_COLUMNS;

	  pattern_select_show_selected (psp, row, col);
	}

      gtk_widget_draw (psp->preview, NULL);
    }
}

static void
pattern_select_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PatternSelect *psp;

  psp = (PatternSelect *) data;

  if (GTK_WIDGET_VISIBLE (psp->shell))
    gtk_widget_hide (psp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if (psp != pattern_select_dialog)
    {
      /* Send data back */
      pattern_change_callbacks (psp, TRUE);
      gtk_widget_destroy (psp->shell); 
      pattern_select_free (psp); 
    }
}

static void
pattern_select_refresh_callback (GtkWidget *widget,
				 gpointer   data)
{
  PatternSelect *psp;
  GSList *list;

  /*  re-init the pattern list  */
  patterns_init (FALSE);

  for (list = pattern_active_dialogs; list; list = g_slist_next (list))
    {
      psp = (PatternSelect *) list->data;

      /*  recalculate scrollbar extents  */
      preview_calc_scrollbar (psp);
    }
}
